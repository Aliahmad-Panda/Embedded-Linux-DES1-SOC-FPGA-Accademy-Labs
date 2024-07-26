#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/kernel.h>
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include <linux/interrupt.h>        // for interrupts
#include "/home/root/lab_tasks/address_map_arm.h"
#include "/home/root/lab_tasks/interrupt_ID.h"

void *LW_virtual;
volatile int *LEDR_ptr;
volatile int *TIMER0_ptr;
volatile int *HEX3_0_ptr;     // virtual address for the HEX3-0 (Seven Segment) port
volatile int *HEX5_4_ptr;


int min = 59;
int sec = 59;
int hund_sec = 99;


static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t stopwatch_read (struct file *, char *, size_t, loff_t *);

static struct file_operations stopwatch_fops = {
    .owner = THIS_MODULE,
    .read = stopwatch_read,
    .open = device_open,
    .release = device_release
};


#define SUCCESS 0
#define MAX_SIZE 256    // we assume that no message longer than this will be used

int SevenSegment(int number)
{										// 6 5 4  3 2 1 0
										// g f e  d c b a
	if (number == 0) return 0x3F ;		// 0 1 1  1 1 1 1
	else if (number == 1) return 0x06 ; // 0 0 0  0 1 1 0
	else if (number == 2) return 0x5B ; // 1 0 1  1 0 1 1
	else if (number == 3) return 0x4F ; // 1 0 0  1 1 1 1
	else if (number == 4) return 0x66 ;	// 1 1 0  0 1 1 0
	else if (number == 5) return 0x6D ; // 1 1 0  1 1 0 1
	else if (number == 6) return 0x7D ;	// 1 1 1  1 1 0 1
	else if (number == 7) return 0x07 ;	// 0 0 0  0 1 1 1
	else if (number == 8) return 0x7F ;	// 1 1 1  1 1 1 1
	else if (number == 9) return 0x6F ;	// 1 1 0  1 1 1 1
	else return 0x00;
}

static struct miscdevice stopwatch = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "stopwatch",
    .fops = &stopwatch_fops,
    .mode = 0666
};

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    hund_sec--;
    if(hund_sec < 0)
    {
        hund_sec = 99;
        sec--;
        if(sec < 0)
        {
            sec = 59;
            min--;
            if (min < 0)
            {
                min = 0;
                sec = 0;
                hund_sec = 0;
                *(TIMER0_ptr + 1) = 0;
            }
        }
    }


    *HEX5_4_ptr = ((SevenSegment(min / 10)) << 8) + (SevenSegment(min % 10));
    *HEX3_0_ptr = ((SevenSegment(sec / 10)) << 24) + ((SevenSegment(sec % 10)) << 16);
    *HEX3_0_ptr += ((SevenSegment(hund_sec / 10)) << 8) + (SevenSegment(hund_sec % 10));
    *TIMER0_ptr = *TIMER0_ptr & ~(1);

    return (irq_handler_t) IRQ_HANDLED;
}

static int stopwatch_registered = 0;

static char stopwatch_msg[MAX_SIZE];

static int __init init_drivers(void) {
    int value;
    int err = misc_register (&stopwatch);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "stopwatch");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "stopwatch");
        stopwatch_registered = 1;
    }

    //initializing memory map
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);


    LEDR_ptr = LW_virtual + LEDR_BASE;
    *LEDR_ptr = 0x300;

    HEX3_0_ptr = LW_virtual + HEX3_HEX0_BASE; // init virtual address for HEX port
    HEX5_4_ptr = LW_virtual + HEX5_HEX4_BASE;

    TIMER0_ptr = LW_virtual + TIMER0_BASE;   // init virtual address for Timer0
    *(TIMER0_ptr + 1) = 0x3;    //Enabling CONT and ITO (Interrupt)
    *(TIMER0_ptr + 2) = 0x4240;
    *(TIMER0_ptr + 3) = 0xF;        //loading 1,000,000 in Hex to get 0.01s

    *(TIMER0_ptr + 1) += 0x4;   //starting Timer0


    // Register the interrupt handler.
    value = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED,
      "stopwatch", (void *) (irq_handler));

    return err;
}

static void __exit exit_drivers(void) {
    if (stopwatch_registered) {
        misc_deregister (&stopwatch);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "stopwatch");
    }

    *LEDR_ptr = 0;
    *HEX3_0_ptr = 0;
    *HEX5_4_ptr = 0;

    free_irq (TIMER0_IRQ, (void*) irq_handler);
}


/* Called when a process opens a device */
static int device_open(struct inode *inode, struct file *file) {
    return SUCCESS;
}

/* Called when a process closes a device */
static int device_release(struct inode *inode, struct file *file) {
    return 0;
}



static ssize_t stopwatch_read(struct file *filp, char *buffer, size_t length, loff_t *offset) {
    size_t bytes;

    sprintf(stopwatch_msg, "%.2d:%.2d:%.2d\n", min, sec, hund_sec);

    bytes = strlen (stopwatch_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?

    if (bytes)
        if (copy_to_user (buffer, &stopwatch_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}


MODULE_LICENSE("GPL");
module_init (init_drivers);
module_exit (exit_drivers);
