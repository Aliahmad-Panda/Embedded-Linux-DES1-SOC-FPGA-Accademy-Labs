#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "/home/root/lab_tasks/address_map_arm.h"

void *LW_virtual;
volatile int *KEY_ptr;
volatile int *SW_ptr;
volatile int *LEDR_ptr;

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t KEY_read (struct file *, char *, size_t, loff_t *);

static struct file_operations KEY_fops = {
    .owner = THIS_MODULE,
    .read = KEY_read,
    .open = device_open,
    .release = device_release
};

static ssize_t SW_read (struct file *, char *, size_t, loff_t *);

static struct file_operations SW_fops = {
    .owner = THIS_MODULE,
    .read = SW_read,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0
#define MAX_SIZE 256    // we assume that no message longer than this will be used

static struct miscdevice KEY = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "KEY",
    .fops = &KEY_fops,
    .mode = 0666
};

static struct miscdevice SW = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "SW",
    .fops = &SW_fops,
    .mode = 0666
};

static int KEY_registered = 0;
static int SW_registered = 0;

static char KEY_msg[MAX_SIZE];
static char SW_msg[MAX_SIZE];

static int __init init_drivers(void) {
    int err1 = misc_register (&KEY);
    if (err1 < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "KEY");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "KEY");
        KEY_registered = 1;
    }

    int err2 = misc_register (&SW);
    if (err2 < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "SW");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "SW");
        SW_registered = 1;
    }


    //initializing memory map
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

    KEY_ptr = LW_virtual + KEY_BASE;

    SW_ptr = LW_virtual + SW_BASE;

    LEDR_ptr = LW_virtual + LEDR_BASE;
    *LEDR_ptr = 0x300;
    *(KEY_ptr + 3) = 0xF; //Clearing Edgecapture

    int err = 0;
    if ((err1 + err2) < 0)
        err = -1;
    else
        err = 0;

    return err;
}

static void __exit exit_drivers(void) {
    if (KEY_registered) {
        misc_deregister (&KEY);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "KEY");
    }

    if (SW_registered) {
        misc_deregister (&SW);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "SW");
    }

    *LEDR_ptr = 0;
}


/* Called when a process opens a device */
static int device_open(struct inode *inode, struct file *file) {
    return SUCCESS;
}

/* Called when a process closes a device */
static int device_release(struct inode *inode, struct file *file) {
    return 0;
}



static ssize_t KEY_read(struct file *filp, char *buffer, size_t length, loff_t *offset) {
    size_t bytes;
    bytes = length;

    sprintf(KEY_msg, "%1x\n" , *(KEY_ptr + 3));

    *(KEY_ptr + 3) = 0xF; //Clearing Edgecapture

    bytes = strlen (KEY_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?

    if (bytes)
        if (copy_to_user (buffer, &KEY_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}

static ssize_t SW_read(struct file *filp, char *buffer, size_t length, loff_t *offset) {
    size_t bytes;
    bytes = length;

    sprintf(SW_msg, "%.3x\n" , *SW_ptr);

    bytes = strlen (SW_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?

    if (bytes)
        if (copy_to_user (buffer, &SW_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}


MODULE_LICENSE("GPL");
module_init (init_drivers);
module_exit (exit_drivers);
