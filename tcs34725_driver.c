#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DRIVER_NAME "tcs34725_driver"
#define CLASS_NAME "tcs34725"
#define DEVICE_NAME "tcs34725"

// TCS34725 register addresses
#define TCS34725_COMMAND_BIT      0x80
#define TCS34725_ENABLE           0x00
#define TCS34725_ENABLE_PON       0x01
#define TCS34725_ENABLE_AEN       0x02
#define TCS34725_CDATAL           0x14
#define TCS34725_RDATAL           0x16
#define TCS34725_GDATAL           0x18
#define TCS34725_BDATAL           0x1A

// IOCTL commands
#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_READ_CLEAR _IOR(TCS34725_IOCTL_MAGIC, 1, int)
#define TCS34725_IOCTL_READ_RED   _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 3, int)
#define TCS34725_IOCTL_READ_BLUE  _IOR(TCS34725_IOCTL_MAGIC, 4, int)
#define TCS34725_IOCTL_READ_RGB_NORMALIZED _IOR(TCS34725_IOCTL_MAGIC, 5, struct tcs34725_rgb)

struct tcs34725_rgb {
    __u8 r;
    __u8 g;
    __u8 b;
};

static struct i2c_client *tcs34725_client;
static struct class* tcs34725_class = NULL;
static struct device* tcs34725_device = NULL;
static int major_number;

static int tcs34725_read_channel(struct i2c_client *client, u8 reg)
{
    s32 lo = i2c_smbus_read_byte_data(client, TCS34725_COMMAND_BIT | reg);
    s32 hi = i2c_smbus_read_byte_data(client, TCS34725_COMMAND_BIT | (reg + 1));
    if (lo < 0 || hi < 0) {
        printk(KERN_ERR "Failed to read from TCS34725\n");
        return -EIO;
    }
    return (hi << 8) | lo;
}

static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int data;
    struct tcs34725_rgb rgb;

    switch (cmd) {
        case TCS34725_IOCTL_READ_CLEAR:
            data = tcs34725_read_channel(tcs34725_client, TCS34725_CDATAL);
            if (copy_to_user((int __user *)arg, &data, sizeof(data)))
                return -EFAULT;
            break;
        case TCS34725_IOCTL_READ_RED:
            data = tcs34725_read_channel(tcs34725_client, TCS34725_RDATAL);
            if (copy_to_user((int __user *)arg, &data, sizeof(data)))
                return -EFAULT;
            break;
        case TCS34725_IOCTL_READ_GREEN:
            data = tcs34725_read_channel(tcs34725_client, TCS34725_GDATAL);
            if (copy_to_user((int __user *)arg, &data, sizeof(data)))
                return -EFAULT;
            break;
        case TCS34725_IOCTL_READ_BLUE:
            data = tcs34725_read_channel(tcs34725_client, TCS34725_BDATAL);
            if (copy_to_user((int __user *)arg, &data, sizeof(data)))
                return -EFAULT;
            break;
        case TCS34725_IOCTL_READ_RGB_NORMALIZED: {
            int r = tcs34725_read_channel(tcs34725_client, TCS34725_RDATAL);
            int g = tcs34725_read_channel(tcs34725_client, TCS34725_GDATAL);
            int b = tcs34725_read_channel(tcs34725_client, TCS34725_BDATAL);
            int c = tcs34725_read_channel(tcs34725_client, TCS34725_CDATAL);
            if (c <= 0) c = 1;

            rgb.r = min(255, (r * 255) / c);
            rgb.g = min(255, (g * 255) / c);
            rgb.b = min(255, (b * 255) / c);

            if (copy_to_user((struct tcs34725_rgb __user *)arg, &rgb, sizeof(rgb))) {
                return -EFAULT;
            }
            break;
        }
        default:
            return -EINVAL;
    }

    return 0;
}

static int tcs34725_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device opened\n");
    return 0;
}

static int tcs34725_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = tcs34725_open,
    .owner = THIS_MODULE,
    .unlocked_ioctl = tcs34725_ioctl,
    .release = tcs34725_release,
};

static int tcs34725_probe(struct i2c_client *client)
{
    int ret;

    tcs34725_client = client;

    // Power ON and enable ADC
    ret = i2c_smbus_write_byte_data(client, TCS34725_COMMAND_BIT | TCS34725_ENABLE, TCS34725_ENABLE_PON);
    msleep(3);
    ret |= i2c_smbus_write_byte_data(client, TCS34725_COMMAND_BIT | TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);

    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize TCS34725\n");
        return ret;
    }

    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
        return major_number;

    tcs34725_class = class_create(CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(tcs34725_class);
    }

    tcs34725_device = device_create(tcs34725_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(tcs34725_device);
    }

    printk(KERN_INFO "TCS34725 driver installed\n");
    return 0;
}

static void tcs34725_remove(struct i2c_client *client)
{
    device_destroy(tcs34725_class, MKDEV(major_number, 0));
    class_unregister(tcs34725_class);
    class_destroy(tcs34725_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "TCS34725 driver removed\n");
}

static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "taos,tcs34725", },
    { },
};
MODULE_DEVICE_TABLE(of, tcs34725_of_match);

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .of_match_table = tcs34725_of_match,
        .owner = THIS_MODULE,
    },
    .probe  = tcs34725_probe,
    .remove = tcs34725_remove,
};

static int __init tcs34725_init(void)
{
    printk(KERN_INFO "Initializing TCS34725 driver\n");
    return i2c_add_driver(&tcs34725_driver);
}

static void __exit tcs34725_exit(void)
{
    printk(KERN_INFO "Exiting TCS34725 driver\n");
    i2c_del_driver(&tcs34725_driver);
}

module_init(tcs34725_init);
module_exit(tcs34725_exit);

MODULE_AUTHOR("Huynh Hoan Hai");
MODULE_DESCRIPTION("TCS34725 I2C Client Driver with IOCTL Interface");
MODULE_LICENSE("GPL");