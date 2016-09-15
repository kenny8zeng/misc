#include <linux/kernel.h>
#include <linux/delay.h>

#include "fmtx.h"

#define ERR_RETURN(opt) \
	do{ \
		if( 0 != ( opt ) ) { \
			printk( KERN_DEBUG LOGTAG_FMTX "error: %s:"#opt"\n", __func__ ); \
			return -1; \
		} \
	} while(0)

/***************************************************************************** 
 * Function
 *****************************************************************************/

//----------------------------------------------------------------------------
typedef struct reg_bits_t
{
	addr_t 	addr;
	reg_t 	mask;
	reg_t	value;
} reg_bits_t;

//----------------------------------------------------------------------------
static const struct reg_bits_t bit_setting[] = 
{
	{ 0x01, 0xff, 0x00 },		//00: Tx channel index low  8bits of 10-bit, (76+CH*0.05)MHz
	{ 0x00, 0x03, 0x00 },		//01: Tx channel index hith 2bits of 10-bit, (76+CH*0.05)MHz
	{ 0x00, 0x08, 0x08 },		//02: Audio mute enable.
	{ 0x04, 0x73, 0x50 },		//03: TX input buffer gain index 5-bit (dB).
	{ 0x06, 0xff, 0x44 },		//04: CID2 read chip id for product id and major resision.
	{ 0x10, 0x7f, 0x7f },		//05: PA output power target: 0.62*PA+71dBu, valid 20~75.

	//apply setup
	{ 0x00, 0x20, 0x00 },		//06: enter idle mode.
	{ 0x00, 0x20, 0x20 },		//07: enter tx mode.

	//reset chip
	{ 0x00, 0x80, 0x80 },		//08: reset chip.

	{ 0, 0, 0 }
};

//----------------------------------------------------------------------------
static const struct reg_bits_t bit_init[] = 
{
	//chip init
	{ 0x03, 0xff, 0x10 },		//00: Crystal source selection.
	{ 0x04, 0xff, 0x32 },		//01: Crystal frequency selection.
	{ 0x00, 0xff, 0x41 },		//02: power up and calibration sequence.
	{ 0xff, 0x00, 50   },		//03: delay 30ms
	{ 0x00, 0xff, 0x01 },		//04: done power up and calibration sequence.
	{ 0x18, 0xff, 0xe4 },		//05: internal init.
	{ 0x1b, 0xff, 0xf0 },		//06: internal init.
	{ 0x01, 0xff, 0x00 },		//07: init tx frequency.
	{ 0x00, 0x03, 0x01 },		//08: init tx frequency.
	{ 0x10, 0x7f, 0x7f },		//09: PA output power target: 0.62*PA+71dBu, valid 20~75.
	{ 0x02, 0xff, 0xb9 },		//10: Selection of never for PA off when no audio.

	{ 0, 0, 0 }
};

//----------------------------------------------------------------------------
static struct reg_bits_t bit_curent[] = 
{
	{ 0x00, 0xff, 0x00 },		//00: register 00
	{ 0x01, 0xff, 0x00 },		//01: register 01
	{ 0x02, 0xff, 0x00 },		//02: register 02
	{ 0x03, 0xff, 0x00 },		//02: register 02
	{ 0x04, 0xff, 0x00 },		//03: register 04
	{ 0x10, 0x7f, 0x00 },		//04: register 10
	{ 0x18, 0xff, 0x00 },		//05: internal init.
	{ 0x1b, 0xff, 0x00 },		//06: internal init.

	{ 0, 0, 0 }
};

//----------------------------------------------------------------------------
typedef enum {
	emREG_IDX_UNKOWN,

	emREG_IDX_FREQ_L8        = 0,
	emREG_IDX_FREQ_H2        = 1,
	emREG_IDX_MUTE           = 2,
	emREG_IDX_TXGAIN         = 3,
	emREG_IDX_CHPID          = 4,
	emREG_IDX_TXPWR          = 5,

	emREG_IDX_APPLY_START    = 6,
	emREG_IDX_APPLY_DOME     = 7,

	emREG_IDX_RESET          = 8,

	emREG_IDX_INIT_START     = 0,
	emREG_IDX_INIT_LAST      = 11,
	emREG_IDX_INIT_END,

	emREG_IDX_REGISTER_START = 0,
	emREG_IDX_REGISTER_LAST  = 7,
	emREG_IDX_REGISTER_END,

	emREG_IDX_DELAY          = 0xff
} reg_ctrl_index_t;

//----------------------------------------------------------------------------
static const char gainmap[] = {
	0x03,	// -15
	0x13,	// -12
	0x23,	// -9
	0x33,	// -6
	0x43,	// -3
	0x53,	// 0
	0x00,	// +3
	0x10,	// +6
	0x20,	// +9
	0x30,	// +12
	0x40,	// +15
	0x50	// +18
};

//----------------------------------------------------------------------------
static int _fm_setting( struct fm_io_t * io, addr_t addr, reg_t mask, reg_t val )
{
	reg_bits_t *rbs = NULL;
	reg_t reg = mask & val;
	addr_t a;

	printk( KERN_DEBUG LOGTAG_FMTX "%s(addr:0x%x, mask:0x%x, val:0x%x)\n",
			__func__, (unsigned int)addr, (unsigned int)mask, (unsigned int)val );

	for( a = emREG_IDX_REGISTER_START; a < emREG_IDX_REGISTER_END; ++a )
	{
		rbs = &bit_curent[ a ];

		if( addr == rbs->addr )
		{
			reg = (rbs->mask & rbs->value & ~mask) | (mask & val);

			ERR_RETURN( io->write( io, addr, reg ) );

			if( rbs )
			{
				rbs->value = reg;
			}

			printk( KERN_DEBUG LOGTAG_FMTX "%s -> write reg 0x%x:0x%x\n", __func__, (unsigned int)addr, (unsigned int)reg );
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
static int _fm_apply( struct fm_io_t *io )
{
	const reg_bits_t * rbs = NULL;

	rbs = &bit_setting[ emREG_IDX_APPLY_START ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, rbs->value ) );
	rbs = &bit_setting[ emREG_IDX_APPLY_DOME ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, rbs->value ) );

	return 0;
}

//----------------------------------------------------------------------------
int fm_initial( struct fm_io_t * io )
{
	int idx = 0;
	reg_bits_t *r = NULL;
	const reg_bits_t *rbs = NULL;
	
	for( idx = emREG_IDX_REGISTER_START; idx < emREG_IDX_REGISTER_END; ++idx )
	{
		r = &bit_curent[ idx ];
		r->value = 0;
	}

	for( idx = emREG_IDX_INIT_START; idx < emREG_IDX_INIT_END; ++idx )
	{
		rbs = &bit_init[ idx ];

		if( rbs->addr == emREG_IDX_DELAY )
		{
			msleep( rbs->value );
		} else {
			ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, rbs->value ) );
		}
	}

	return _fm_apply( io );
}

//----------------------------------------------------------------------------
int fm_detect( struct fm_io_t * io )
{
	reg_t reg = 0;

	const reg_bits_t *rbs = &bit_setting[ emREG_IDX_CHPID ];

	ERR_RETURN( io->read( io, rbs->addr, &reg ) );

	printk( KERN_DEBUG LOGTAG_FMTX "%s result %x\n",
			__func__, (unsigned int)reg );

	return ( (reg & rbs->mask) == (rbs->value & rbs->mask) )? 0 : ( 0xaa000000 | (int)reg );
}

//----------------------------------------------------------------------------
int fm_register(struct fm_io_t * io, uint4_t val)
{
	addr_t addr = (addr_t)(val >> 16);
	reg_t  reg  = (reg_t )val;

	ERR_RETURN( io->write( io, addr, reg ) );

	return 0;
}

//----------------------------------------------------------------------------
int fm_volume(struct fm_io_t *io, uint4_t val)
{
	const reg_bits_t *rbs = &bit_setting[ emREG_IDX_TXGAIN ];
	uint4_t index = sizeof( gainmap ) / sizeof( reg_t );
	reg_t reg_volume = gainmap[ ( val < index )? val : (index - 1) ];
	reg_t reg_mute   = ( val == 0 )? rbs->value : 0;

	rbs = &bit_setting[ emREG_IDX_MUTE ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, reg_mute ) );

	rbs = &bit_setting[ emREG_IDX_TXGAIN ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, reg_volume ) );

	return _fm_apply( io );
}

//----------------------------------------------------------------------------
int fm_txpwr(struct fm_io_t *io, uint4_t val)
{
	const reg_bits_t *rbs = NULL;
	
	rbs = &bit_setting[ emREG_IDX_TXPWR ];

	if( val > rbs->mask ) val = rbs->mask;

	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, (reg_t)val ) );

	return _fm_apply( io );
} 

//----------------------------------------------------------------------------
int fm_freq(struct fm_io_t * io, uint4_t val)
{
	uint4_t index = 0;
	uint4_t fmax = 7600 + 5 * 0x3ff;
	const reg_bits_t *rbs = NULL;

	//freq = 76 + 0.05 * index
	if( val < 7600 ) val = 7600;
	if( val > fmax ) val = fmax;

	index = (val - 7600) / 5;

	rbs = &bit_setting[ emREG_IDX_FREQ_L8 ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, (reg_t)index ) );

	rbs = &bit_setting[ emREG_IDX_FREQ_H2 ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, (reg_t)(index >> 8) ) );

	return 0;
}

//----------------------------------------------------------------------------
int fm_mode(struct fm_io_t * io, uint4_t val)
{
	const reg_bits_t * rbs = NULL;

	rbs = &bit_setting[ val? emREG_IDX_APPLY_DOME : emREG_IDX_APPLY_START ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, rbs->value ) );

	return 0;
}

//----------------------------------------------------------------------------
int fm_reset(struct fm_io_t * io, uint4_t val)
{
	const reg_bits_t * rbs = NULL;

	rbs = &bit_setting[ emREG_IDX_RESET ];
	ERR_RETURN( _fm_setting( io, rbs->addr, rbs->mask, rbs->value ) );

	return fm_initial( io );
}

