#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/platform_device.h>

static struct input_dev rc_input_dev;
static struct platform_device *rc_dev;

static ssize_t set_rc( struct device *dev, struct device_attribute *attr, const char *buffer, size_t count )
{
	return count;
}

DEVICE_ATTR(rc, 0644, NULL, set_rc );

static struct attribute *attrs[] = { &dev_attr_rc.attr, NULL };
static struct attribute_group rc_attr_group = { .attrs = attrs, };

int __init rc_init(void)
{
	rc_dev = platform_device_register_simple( "hudplay_rc", -1, NULL, 0 );
	if( IS_ERR( rc_dev ) )
	{
		return PTR_ERR( rc_dev );
	}

	sysfs_create_group( &rc_dev->dev.kobj, &rc_attr_group );

	rc_input_dev = input_allocate_device();

	if( !rc_input_dev )
	{
		return -1;
	}

	//set_bit( EV_KEY, rc_input_dev->evbit );
	
	input_register_device( rc_input_dev );

	return 0;
}

void __exit rc_exit(void)
{
	input_unregister_device( rc_input_dev );

	sysfs_remove_group( &rc_dev->dev.kobj, &rc_attr_group );

	platform_device_unregister( rc_dev );
}

module_init( rc_init );
module_exit( rc_exit );

MODULE_AUTHOR("HudPlay");
MODULE_DESCRIPTION("HudPlay remote controler");
MODULE_LICENSE("GPL");

