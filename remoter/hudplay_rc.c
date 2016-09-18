#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/platform_device.h>

//----------------------------------------------------------------------------
#define LOGTAG "[hp-rc]"

//----------------------------------------------------------------------------------------
static struct input_dev *rc_input_dev;
static struct platform_device *rc_dev;

//----------------------------------------------------------------------------------------
static ssize_t set_rc( struct device *dev, struct device_attribute *attr,
		const char *buffer, size_t count )
{
	unsigned long evt = 0;
	int active, scan, key;

	evt = simple_strtoul( buffer, NULL, 0 );

	if( evt )
	{
		active = (evt & 0xff000000)? 1 : 0;
		scan   = ( evt >> 00 )& 0xfff;
		key    = ( evt >> 12 )& 0xfff;

		input_event( rc_input_dev, EV_MSC, MSC_SCAN, scan );
		input_event( rc_input_dev, EV_KEY, key, active );
		input_sync( rc_input_dev );

		printk( KERN_DEBUG LOGTAG "%s: key %s %x\n", __func__,
				(active)? "down" : "up", scan );
	}

	return count;
}

//----------------------------------------------------------------------------------------
DEVICE_ATTR(rc, 0644, NULL, set_rc );

static struct attribute *attrs[] = { &dev_attr_rc.attr, NULL };
static struct attribute_group rc_attr_group = { .attrs = attrs, };

//----------------------------------------------------------------------------------------
int __init rc_init(void)
{
	int ret = 0;
	unsigned long code;

	rc_dev = platform_device_register_simple( "hudplay_rc", -1, NULL, 0 );
	if( IS_ERR( rc_dev ) )
	{
		return PTR_ERR( rc_dev );
	}

	ret = sysfs_create_group( &rc_dev->dev.kobj, &rc_attr_group );
	if( ret ) goto ERR_FS;

	rc_input_dev = input_allocate_device();
	if( !rc_input_dev ) goto ERR_ALLOC;

	rc_input_dev->name = "hudplay_rc";

	set_bit( EV_KEY, 		rc_input_dev->evbit );
	set_bit( EV_MSC, 		rc_input_dev->evbit );

	set_bit( MSC_SCAN, 		rc_input_dev->mscbit );

	for( code = 0; code < 0x200; ++code )
	{
		set_bit( code, rc_input_dev->keybit );
	}
	
	ret = input_register_device( rc_input_dev );
	if( ret ) goto ERR_INPUT;

	return 0;

ERR_INPUT:
	input_free_device( rc_input_dev );

ERR_ALLOC:
	sysfs_remove_group( &rc_dev->dev.kobj, &rc_attr_group );

ERR_FS:
	platform_device_unregister( rc_dev );

	return -1;
}

//----------------------------------------------------------------------------------------
void __exit rc_exit(void)
{
	input_unregister_device( rc_input_dev );

	input_free_device( rc_input_dev );

	sysfs_remove_group( &rc_dev->dev.kobj, &rc_attr_group );

	platform_device_unregister( rc_dev );
}

//----------------------------------------------------------------------------------------
module_init( rc_init );
module_exit( rc_exit );

//----------------------------------------------------------------------------------------
MODULE_AUTHOR("HudPlay");
MODULE_DESCRIPTION("HudPlay remote controler");
MODULE_LICENSE("GPL");

