#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "/home/root/lab_tasks/address_map_arm.h"

void *LW_virtual;
volatile int *HEX3_0_ptr;
volatile int *HEX5_4_ptr;
volatile int *LEDR_ptr;

int SevenSegment(int number)
{										// 6 5 4  3 2 1 0
										// g f e  d c b a
	if (number == 0) return 0x3F ;		// 0 1 1  1 1 1 1
	else if (number == 1) return 0x06 ; // 0 0 0  0 1 1 0
	else if (number == 2) return 0x5B ; // 1 0 1  1 0 1 1
	else if (number == 3) return 0x4F ; // 1 0 0  1 1 1 1
	else if (number == 4) return 0x66;	// 1 1 0  0 1 1 0
	else if (number == 5) return 0x6D ; // 1 1 0  1 1 0 1
	else if (number == 6) return 0x7D ;	// 1 1 1  1 1 0 1
	else if (number == 7) return 0x07 ;	// 0 0 0  0 1 1 1
	else if (number == 8) return 0x7F ;	// 1 1 1  1 1 1 1
	else if (number == 9) return 0x6F;	// 1 1 0  1 1 1 1
	else return 0;
}

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t LEDR_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations LEDR_fops = {
    .owner = THIS_MODULE,
    .write = LEDR_write,
    .open = device_open,
    .release = device_release
};


static ssize_t HEX_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations HEX_fops = {
    .owner = THIS_MODULE,
    .write = HEX_write,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0
#define MAX_SIZE 256    // we assume that no message longer than this will be used

static struct miscdevice LEDR = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "LEDR",
    .fops = &LEDR_fops,
    .mode = 666
};

static struct miscdevice HEX = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "HEX",
    .fops = &HEX_fops,
    .mode = 666
};

static int LEDR_registered = 0;
static int HEX_registered = 0;

static char LEDR_msg[MAX_SIZE];
static char HEX_msg[MAX_SIZE];

static int __init init_drivers(void) {
    int err1 = misc_register (&LEDR);
    if (err1 < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "LEDR");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "LEDR");
        LEDR_registered = 1;
    }

    int err2 = misc_register (&HEX);
    if (err2 < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "HEX");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "HEX");
        HEX_registered = 1;
    }


    //initializing memory map
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

    HEX3_0_ptr = LW_virtual + HEX3_HEX0_BASE;
    HEX5_4_ptr = LW_virtual + HEX5_HEX4_BASE;

    *HEX3_0_ptr = 0x0F;
    *HEX5_4_ptr = 0x0F;


    LEDR_ptr = LW_virtual + LEDR_BASE;
    *LEDR_ptr = 0x0C0;

    int err = 0;
    if ((err1 + err2) < 0)
        err = -1;
    else
        err = 0;

    return err;
}

static void __exit exit_drivers(void) {
    if (LEDR_registered) {
        misc_deregister (&LEDR);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "LEDR");
    }

    if (HEX_registered) {
        misc_deregister (&HEX);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "HEX");
    }

    *LEDR_ptr = 0;
    *HEX3_0_ptr = 0;
    *HEX5_4_ptr = 0;
}


/* Called when a process opens a device */
static int device_open(struct inode *inode, struct file *file) {
    return SUCCESS;
}

/* Called when a process closes a device */
static int device_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t LEDR_write(struct file *filp, const char *buffer, size_t length, loff_t *offset) {
    size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (LEDR_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");

    sscanf(LEDR_msg, "%x", LEDR_ptr);

    LEDR_msg[bytes] = '\0';    // NULL terminate
    return bytes;
}

static ssize_t HEX_write(struct file *filp, const char *buffer, size_t length, loff_t *offset) {
    size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (HEX_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");

    volatile int hex_value;
    sscanf(HEX_msg, "%d", &hex_value);

    *HEX5_4_ptr = (SevenSegment((hex_value % 1000000) / 100000) << 8) + SevenSegment((hex_value % 100000) / 10000);
    *HEX3_0_ptr = (SevenSegment((hex_value % 10000) / 1000) << 24) + (SevenSegment((hex_value % 1000) / 100) << 16);
    *HEX3_0_ptr += (SevenSegment((hex_value % 100) / 10) << 8) + SevenSegment(hex_value % 10);


    HEX_msg[bytes] = '\0';    // NULL terminate
    return bytes;
}




MODULE_LICENSE("GPL");
module_init (init_drivers);
module_exit (exit_drivers);
