#ifndef _FMTX_H_
#define  _FMTX_H_

//----------------------------------------------------------------------------
#ifdef FM_BK1085
#include "bk1085.h"
#endif

#ifdef FM_LPD6006
#include "lpd6006.h"
#endif

#ifdef FM_HS6760
#include "hs6760.h"
#endif

//----------------------------------------------------------------------------
#define LOGTAG_FMTX "[fmtx]"

//----------------------------------------------------------------------------
typedef unsigned char  byte_t;
typedef unsigned int   uint_t;
typedef unsigned short uint2_t;
typedef unsigned long  uint4_t;

//----------------------------------------------------------------------------
struct fm_io_t
{
	void *dev;
	int (*read) (struct fm_io_t*, addr_t, reg_t* );
	int (*write)(struct fm_io_t*, addr_t, reg_t );
};

//----------------------------------------------------------------------------
int fm_register(struct fm_io_t * io, uint4_t val);
int fm_volume(struct fm_io_t *io, uint4_t vol);
int fm_txpwr(struct fm_io_t *io, uint4_t txpwr);
int fm_freq(struct fm_io_t * io, uint4_t val);
int fm_mode(struct fm_io_t * io, uint4_t val);
int fm_reset(struct fm_io_t * io, uint4_t val);

int fm_initial( struct fm_io_t * io );
int fm_detect( struct fm_io_t * io );

//----------------------------------------------------------------------------
typedef int (*fm_func_t)(struct fm_io_t *, uint4_t);

#endif //_FMTX_H_

