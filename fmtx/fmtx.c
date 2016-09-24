/*******************************************************************************
 * Filename:
 * ---------
 *  fmtx.c
 *
 * Project:
 * --------
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *  Hudplay coder.
 * 
 *******************************************************************************/
/***************************************************************************** 
 * Include
 *****************************************************************************/ 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>

#include <cust_gpio_usage.h>
#include <cust_eint.h>

#include <mach/mt_gpio.h>
#include <mach/eint.h>

#include <linux/interrupt.h>
#include <linux/delay.h>

#include "fmtx.h"

/***************************************************************************** 
 * Define
 *****************************************************************************/

#define DEVICE_NAME "fmtx"
#define DEVICE_CLASS_NAME "fm_tx"
#define DEVICE_I2C_BUSNUM  2
#define MAX_BUFFER_SIZE	255
#define I2C_FREQUENCY 100

/***************************************************************************** 
 * GLobal Variable
 *****************************************************************************/ 
static struct i2c_board_info __initdata fmtx_board_info = {
	I2C_BOARD_INFO(DEVICE_NAME, FMTX_I2C_ID)
};

/***************************************************************************** 
 * Function Prototype
 *****************************************************************************/ 
static int fmtx_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int fmtx_remove(struct i2c_client *client);

static int fmtx_dev_open(struct inode *inode, struct file *filp);
static int fmtx_dev_release(struct inode *inode, struct file *filp);

static long fmtx_dev_unlocked_ioctl(struct file *filp,unsigned int cmd, unsigned long arg);

/***************************************************************************** 
 * Data Structure
 *****************************************************************************/
//----------------------------------------------------------------------------
typedef enum {
	emFMTX_FN_BASE = 0x00240000,

	emFMTX_FN_REGISTER = emFMTX_FN_BASE,
	emFMTX_FN_TXPWR,
	emFMTX_FN_CHANNEL,
	emFMTX_FN_VOLUME,
	emFMTX_FN_MODE,
	emFMTX_FN_RESET,

	emFMTX_FN_COUNT
} fmtx_ioctrl_cmd_t;

//----------------------------------------------------------------------------
typedef enum {
	emTFR_READ,
	emTFR_WRITE
} i2c_tfr_dir_t;

//----------------------------------------------------------------------------
typedef struct fmtx_dev_t
{
	struct miscdevice	device;
	struct i2c_client	*client;
	struct class		*class;
	struct device 		*kobj;
	dev_t				devno;
	struct fm_io_t		io;
} fmtx_dev_t;

//----------------------------------------------------------------------------
struct fm_state_t {
	uint4_t reg;
	fm_func_t func;
};

//----------------------------------------------------------------------------
static struct fmtx_dev_t *gdev = NULL;

//----------------------------------------------------------------------------
static struct fm_state_t fm_state[ emFMTX_FN_COUNT ] = {
	{ 0, 	fm_register },
	{ 0, 	fm_txpwr	},
	{ 0, 	fm_freq		},
	{ 0, 	fm_volume 	},
	{ 0, 	fm_mode 	},
	{ 0, 	fm_reset	}
};

//----------------------------------------------------------------------------
static const struct i2c_device_id fmtx_id[] = {
	{ DEVICE_NAME, 0 },
	{ }
};

//----------------------------------------------------------------------------
static struct i2c_driver fmtx_dev_driver = {
	.id_table	= fmtx_id,
	.probe		= fmtx_probe,
	.remove		= fmtx_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "fmtx",
	},

};

//----------------------------------------------------------------------------
static const struct file_operations fmtx_dev_fops = {
	.owner			= THIS_MODULE,
	.llseek			= no_llseek,
	.open			= fmtx_dev_open,
	.release		= fmtx_dev_release,
	.unlocked_ioctl = fmtx_dev_unlocked_ioctl,
};

//----------------------------------------------------------------------------
#pragma pack(1)
struct _i2c_buffer_t { addr_t r; reg_t d; };
#pragma pack()

//----------------------------------------------------------------------------
static int fmtx_dev_transfer( struct fmtx_dev_t *dev, i2c_tfr_dir_t dir,
		addr_t reg, reg_t *data )
{
	int mc, ret = 0;
	struct _i2c_buffer_t send;

	printk( KERN_DEBUG LOGTAG_FMTX "%s( dev:0x%x, dir:%d, addr:0x%x, reg:0x%x, data:0x%x\n"
			, __func__, (uint_t)dev, dir, (uint_t)dev->client->addr,
			(uint_t)reg, (uint_t)*data );

	send.r = reg;

	if( dir == emTFR_WRITE )
	{
		send.d = *data;
	}

	do
	{
		struct i2c_msg msgs[] = {
			{
				.addr	= dev->client->addr,
				.flags	= 0,
				.len	= ((dir == emTFR_WRITE)? (sizeof(addr_t) + sizeof(reg_t)) : sizeof(reg_t)),
				.buf	= (byte_t*)&send,
				.timing = I2C_FREQUENCY,
			},
			{
				.addr	= dev->client->addr,
				.flags	= ( dir == emTFR_WRITE )? 0 : I2C_M_RD,
				.len	= sizeof(reg_t),
				.buf	= data,
				.timing = I2C_FREQUENCY,
			}
		};

		mc = ( dir == emTFR_WRITE )? 1 : (sizeof(msgs) / sizeof(struct i2c_msg));

		ret = i2c_transfer( dev->client->adapter, msgs, mc );

	} while( 0 );

	if( ret != mc )
	{
		printk( KERN_ERR LOGTAG_FMTX "%s err: %d\n", __func__, ret );

		return -1;
	}

	if( dir == emTFR_READ )
	{
		printk( KERN_DEBUG LOGTAG_FMTX "%s: read result: 0x%x\n", __func__, (uint_t)*data );
	}

	return 0;
}

//----------------------------------------------------------------------------
static int fmtx_fm_read(struct fm_io_t* io, addr_t addr, reg_t *val )
{
	return fmtx_dev_transfer( io->dev, emTFR_READ,  addr, val );
}

//----------------------------------------------------------------------------
static int fmtx_fm_write(struct fm_io_t* io, addr_t addr, reg_t  val )
{
	return fmtx_dev_transfer( io->dev, emTFR_WRITE, addr, &val );
}

//----------------------------------------------------------------------------
static ssize_t set_config_reset(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count )
{
	int ret = 0;

	ret = simple_strtol( buf, NULL, 0 );

	if( 0 != fm_reset( &(gdev->io), ret ) )
	{
		printk( KERN_ERR LOGTAG_FMTX "%s error fm_reset\n", __func__ );

		return 0;
	}

	return count;
}

//----------------------------------------------------------------------------
static ssize_t show_config_register(struct device *dev, struct device_attribute *attr, char *buff )
{
	int ret = fm_detect( &(gdev->io) );

	if( ret )
	{
		ret = snprintf( buff, 32, "fm chip cid2: 0x%x\n", (uint_t)ret );
	} else {
		ret = snprintf( buff, 32, "fm chip detect ok.\n" );
	}

	return ret;
}

//----------------------------------------------------------------------------
static ssize_t set_config_register(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count )
{
	int ret = 0;

	ret = simple_strtol( buf, NULL, 0 );

	printk( KERN_DEBUG LOGTAG_FMTX "%s strtol 0x%x \n", __func__, ret );

	if( 0 != ( ret = fm_register( &(gdev->io), ret ) ) )
	{
		printk( KERN_ERR LOGTAG_FMTX "%s error %x\n", __func__, ret );

		return 0;
	} else {
		return count;
	}
}

//----------------------------------------------------------------------------
static ssize_t show_config(struct fmtx_dev_t *dev, struct device_attribute *attr,
		char *buff, const char* format, uint_t opa )
{
	struct fm_state_t *state = fm_state + (opa - emFMTX_FN_BASE);
	return snprintf( buff, 16, format, (uint_t)(state->reg) );
}

//----------------------------------------------------------------------------
static ssize_t set_config(struct fmtx_dev_t *dev, struct device_attribute *attr,
		const char *buf, size_t count, uint4_t format, uint_t opa )
{
	int ret = 0;
	struct fm_state_t *state = fm_state + (opa - emFMTX_FN_BASE);
	state->reg = simple_strtol( buf, NULL, format );

	printk( KERN_DEBUG LOGTAG_FMTX "%s strtol %u \n", __func__, (uint_t)(state->reg) );

	if( 0 != ( ret = state->func( &(dev->io), state->reg ) ) )
	{
		printk( KERN_ERR LOGTAG_FMTX "%s error %x\n", __func__, (uint_t)ret );

		return 0;
	} else {
		return count;
	}
}

//----------------------------------------------------------------------------
static ssize_t show_config_f(struct device *dev, struct device_attribute *attr, char *buff)
{ return show_config( gdev, attr, buff, "%u", emFMTX_FN_CHANNEL ); }
static ssize_t set_config_f(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{ return set_config( gdev, attr, buf, count, 0, emFMTX_FN_CHANNEL ); }

//----------------------------------------------------------------------------
static ssize_t show_config_v(struct device *dev, struct device_attribute *attr, char *buff)
{ return show_config( gdev, attr, buff, "%u", emFMTX_FN_VOLUME ); }
static ssize_t set_config_v(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{ return set_config( gdev, attr, buf, count, 0, emFMTX_FN_VOLUME ); }

//----------------------------------------------------------------------------
static ssize_t show_config_t(struct device *dev, struct device_attribute *attr, char *buff)
{ return show_config( gdev, attr, buff, "%u", emFMTX_FN_TXPWR ); }
static ssize_t set_config_t(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{ return set_config( gdev, attr, buf, count, 0, emFMTX_FN_TXPWR ); }

//----------------------------------------------------------------------------
static ssize_t show_config_m(struct device *dev, struct device_attribute *attr, char *buff)
{ return show_config( gdev, attr, buff, "%u", emFMTX_FN_MODE ); }
static ssize_t set_config_m(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{ return set_config( gdev, attr, buf, count, 0, emFMTX_FN_MODE ); }

//----------------------------------------------------------------------------
static DEVICE_ATTR(reset, 		S_IWUSR, 	NULL,			 		set_config_reset);
static DEVICE_ATTR(registor,    660, 		show_config_register, 	set_config_register);
static DEVICE_ATTR(freq,   		660, 		show_config_f, 			set_config_f);
static DEVICE_ATTR(volume,  	660, 		show_config_v, 			set_config_v);
static DEVICE_ATTR(txpower, 	660, 		show_config_t, 			set_config_t);
static DEVICE_ATTR(mode, 	 	660, 		show_config_m, 			set_config_m);

//----------------------------------------------------------------------------
static int fmtx_fs_create( struct fmtx_dev_t * dev )
{
	int ret = 0;

	struct device* device = NULL;

	printk( KERN_DEBUG LOGTAG_FMTX LOGTAG_FMTX "%s\n", __func__ );

	dev->class = class_create( THIS_MODULE, DEVICE_CLASS_NAME );
	if( IS_ERR( dev->class ) )
	{
		ret = PTR_ERR( dev->class );
		printk( KERN_ERR LOGTAG_FMTX "%s: create class failed.\n", __func__ );
		return ret;
	}

	device = device_create( dev->class, NULL, dev->devno, "%s", DEVICE_NAME );
	if( IS_ERR( device ) )
	{
		ret = PTR_ERR( device );
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device failed.\n", __func__ );
		goto dev_fail;
	}

	ret = device_create_file(device, &dev_attr_reset);
	if( ret < 0 )
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device attr file fm reset failed.\n", __func__ );
		goto dev_attr_reset;
	}

	ret = device_create_file(device, &dev_attr_registor);
	if( ret < 0 )
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device attr file fm registor failed.\n", __func__ );
		goto dev_attr_registor;
	}

	ret = device_create_file(device, &dev_attr_freq);
	if( ret < 0 )
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device attr file fm freq failed.\n", __func__ );
		goto dev_attr_freq;
	}

	ret = device_create_file(device, &dev_attr_volume);
	if( ret < 0 )
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device attr file fm volume failed.\n", __func__ );
		goto dev_attr_volume;
	}

	ret = device_create_file(device, &dev_attr_txpower);
	if( ret < 0 )
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device attr file fm txpower failed.\n", __func__ );
		goto dev_attr_txpower;
	}

	ret = device_create_file(device, &dev_attr_mode);
	if( ret < 0 )
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s: create device attr file fm mode failed.\n", __func__ );
		goto dev_attr_mode;
	}

	dev->kobj = device;

	return ret;

dev_attr_mode:
	device_remove_file( device, &dev_attr_txpower );
dev_attr_txpower:
	device_remove_file( device, &dev_attr_volume );
dev_attr_volume:
	device_remove_file( device, &dev_attr_freq );
dev_attr_freq:
	device_remove_file( device, &dev_attr_registor );
dev_attr_registor:
	device_remove_file( dev->kobj, &dev_attr_reset );
dev_attr_reset:
	device_destroy( dev->class, dev->devno );
dev_fail:
	class_destroy( dev->class );

	return ret;
}

//----------------------------------------------------------------------------
static void fmtx_fs_remove( struct fmtx_dev_t *dev )
{
	printk( KERN_DEBUG LOGTAG_FMTX "%s\n", __func__ );

	device_remove_file( dev->kobj, &dev_attr_mode );
	device_remove_file( dev->kobj, &dev_attr_txpower );
	device_remove_file( dev->kobj, &dev_attr_volume );
	device_remove_file( dev->kobj, &dev_attr_freq );
	device_remove_file( dev->kobj, &dev_attr_registor );
	device_remove_file( dev->kobj, &dev_attr_reset );

	if( dev->class )
	{
		device_destroy( dev->class, dev->devno );
		class_destroy( dev->class );
	}
}

//----------------------------------------------------------------------------
static int fmtx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	struct fmtx_dev_t *dev = NULL;

	printk( KERN_DEBUG LOGTAG_FMTX "%s\n", __func__ );

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	dev = kmalloc(sizeof(struct fmtx_dev_t), GFP_KERNEL);
	if( dev == NULL ) {
		printk( KERN_ERR  LOGTAG_FMTX "Failed to alloc dev memory.\n");  
		return ret;  
	}

	printk( KERN_DEBUG LOGTAG_FMTX "%s alloc dev addr %x\n",
			__func__, (uint_t)(void*)dev );

	memset( dev, 0, sizeof( struct fmtx_dev_t ) );

	ret = alloc_chrdev_region(&dev->devno, 0, 1, DEVICE_NAME);  
	if(ret < 0) {  
		printk( KERN_ERR  LOGTAG_FMTX "Failed to alloc char dev region.\n");  
		goto err_alloc_devno;  
	}  

	dev->client   = client;

	dev->device.minor = MINOR( dev->devno );
	dev->device.name = DEVICE_NAME;
	dev->device.fops = &fmtx_dev_fops;

	dev->io.dev   = (void*)dev;
	dev->io.read  = &fmtx_fm_read;
	dev->io.write = &fmtx_fm_write;

	ret = misc_register(&dev->device);
	if (ret) 
	{
		printk( KERN_ERR  LOGTAG_FMTX "%s : misc_register failed\n", __func__);
		goto err_dev_register;
	}

	i2c_set_clientdata(client, dev);

	ret = fmtx_fs_create( dev );
	if( ret ) {
		printk( KERN_ERR  LOGTAG_FMTX "%s : fmtx_fs_create failed\n", __func__);
		goto err_fs;
	}

	ret = fm_detect( &(dev->io) );
	if( ret != 0 ) {
		printk( KERN_ERR  LOGTAG_FMTX "%s : fm_detect detect failed, ret:%x\n", __func__, ret );
		goto err_fm_detect;
	}

	gdev = dev;

	return 0;

err_fm_detect:
	fmtx_fs_remove( dev );
err_fs:
	misc_deregister(&dev->device);
err_dev_register:
	unregister_chrdev_region(dev->devno, 1);  
err_alloc_devno:
	kfree(dev);

	return ret;      
}

//----------------------------------------------------------------------------
static int fmtx_remove(struct i2c_client *client)
{	
	struct fmtx_dev_t *dev;

	dev = (struct fmtx_dev_t*)i2c_get_clientdata(client);

	printk( KERN_DEBUG LOGTAG_FMTX "%s\n", __func__ );

	fmtx_fs_remove( dev );

	misc_deregister(&dev->device);

	unregister_chrdev_region(dev->devno, 1);  

	kfree(dev);
	
	return 0;
}

//----------------------------------------------------------------------------
static int fmtx_dev_open(struct inode *inode, struct file *filp)
{
	struct fmtx_dev_t *dev = container_of(filp->private_data, struct fmtx_dev_t, device);

	filp->private_data = dev;

	printk(KERN_DEBUG LOGTAG_FMTX "%s : major:%d, minor:%d, use dev addr:%p\n",
			__func__, imajor(inode), iminor(inode), dev);

	return 0;
}

//----------------------------------------------------------------------------
static int fmtx_dev_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG LOGTAG_FMTX "%s\n", __func__);

	return 0;
}

//----------------------------------------------------------------------------
static long fmtx_dev_unlocked_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	struct fmtx_dev_t *dev = filp->private_data;
	struct fm_state_t* state = NULL;
	int result = 0;

	printk( KERN_DEBUG LOGTAG_FMTX "%s( %x, %x )\n", __func__, cmd, (uint_t)arg );

	if( cmd > emFMTX_FN_BASE && cmd < emFMTX_FN_COUNT )
	{
		state = fm_state + ( cmd - emFMTX_FN_BASE );
		result = state->func( &(dev->io), arg );

		if( result < 0 )
		{
			printk( KERN_ERR  LOGTAG_FMTX "%s( cmd:%d, arg:%lu ): error ioctrl operation.\n",
					__func__, cmd, arg);
		} else {
			state->reg = arg;
		}
	} else {
		printk( KERN_ERR  LOGTAG_FMTX "%s( cmd:%d, arg:%lu ): error unkown ioctrl operation.\n",
				__func__, cmd, arg);
	}

	return result;
}

//----------------------------------------------------------------------------
/*
 * module load/unload record keeping
 */
static int __init fmtx_dev_init(void)
{
	printk(KERN_DEBUG LOGTAG_FMTX "i2c_register_board_info\n");
	i2c_register_board_info(DEVICE_I2C_BUSNUM, &fmtx_board_info, 1);

	printk(KERN_DEBUG LOGTAG_FMTX "i2c_add_driver\n");
	i2c_add_driver(&fmtx_dev_driver);

	printk(KERN_DEBUG LOGTAG_FMTX "fmtx_dev_init success\n");	
	return 0;
}

//----------------------------------------------------------------------------
static void __exit fmtx_dev_exit(void)
{
	printk(KERN_DEBUG LOGTAG_FMTX "fmtx_dev_exit\n");
	i2c_del_driver(&fmtx_dev_driver);
}

//----------------------------------------------------------------------------
module_init(fmtx_dev_init);
module_exit(fmtx_dev_exit);

//----------------------------------------------------------------------------
MODULE_AUTHOR("HudPlay");
MODULE_DESCRIPTION("HudPlay H1 driver");
MODULE_LICENSE("GPL");

