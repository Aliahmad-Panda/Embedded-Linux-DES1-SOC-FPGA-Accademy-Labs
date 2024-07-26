#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "/home/root/lab_tasks/address_map_arm.h"
#include "/home/root/lab_tasks/interrupt_ID.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Altera University Program");
MODULE_DESCRIPTION("Clock");

void * LW_virtual;         // Lightweight bridge base address
volatile int *LEDR_ptr;    // virtual address for the LEDR port
volatile int *KEY_ptr;     // virtual address for the KEY port
volatile int *HEX3_0_ptr;     // virtual address for the HEX3-0 (Seven Segment) port
volatile int *HEX5_4_ptr;
volatile int *TIMER0_ptr;

int hund_sec = 0;
int sec = 0;
int min = 0;


volatile int SevenSegment(volatile int number)
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

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    hund_sec++;
    if(hund_sec > 99)
    {
        hund_sec = 0;
        sec++;
        if(sec > 59)
        {
            sec = 0;
            min++;
            if (min > 59)
            {
                hund_sec = 0;
                sec = 0;
                min = 0;
            }
        }
    }

    if(*KEY_ptr & 0x2)
    printk(KERN_ERR "\e[2J\e[H%.2d:%.2d:%.2d", min, sec, hund_sec);

    *HEX5_4_ptr = ((SevenSegment(min / 10)) << 8) + (SevenSegment(min % 10));
    *HEX3_0_ptr = ((SevenSegment(sec / 10)) << 24) + ((SevenSegment(sec % 10)) << 16);
    *HEX3_0_ptr += ((SevenSegment(hund_sec / 10)) << 8) + (SevenSegment(hund_sec % 10));
    *TIMER0_ptr = *TIMER0_ptr & 0;

    return (irq_handler_t) IRQ_HANDLED;
}

static int __init initialize_timer(void)
{
    int value;
    // generate a virtual address for the FPGA lightweight bridge
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

    LEDR_ptr = LW_virtual + LEDR_BASE;  // init virtual address for LEDR port
    *LEDR_ptr = 0x201;   // turn on the leftmost light

    KEY_ptr = LW_virtual + KEY_BASE;    // init virtual address for KEY port
    // Clear the PIO edgecapture register (clear any pending interrupt)
    *(KEY_ptr + 3) = 0xF;

    // Enable IRQ generation for the 4 buttons
    *(KEY_ptr + 2) = 0xF;

    HEX3_0_ptr = LW_virtual + HEX3_HEX0_BASE; // init virtual address for HEX port
    HEX5_4_ptr = LW_virtual + HEX5_HEX4_BASE;
    *HEX3_0_ptr = 0;
    *HEX5_4_ptr = 0;


    TIMER0_ptr = LW_virtual + TIMER0_BASE;   // init virtual address for Timer0
    *(TIMER0_ptr + 1) = 0x3;    //Enabling CONT and ITO (Interrupt)
    *(TIMER0_ptr + 2) = 0x4240;
    *(TIMER0_ptr + 3) = 0xF;        //loading 1,000,000 in Hex to get 0.01s

    *(TIMER0_ptr + 1) += 0x4;   //starting Timer0


    // Register the interrupt handler.
    value = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED,
      "timer", (void *) (irq_handler));

    return value;
}


static void __exit cleanup_timer(void)
{
    *LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
    *HEX3_0_ptr = 0;
    *HEX5_4_ptr = 0;


    free_irq (TIMER0_IRQ, (void*) irq_handler);

}

module_init(initialize_timer);
module_exit(cleanup_timer);
