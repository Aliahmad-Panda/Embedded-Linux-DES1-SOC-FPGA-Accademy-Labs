#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "physical.h"
#include "/home/root/lab_tasks/address_map_arm.h"
#include <time.h>


volatile int * LEDR_ptr;   // virtual address pointer to red LEDs
volatile int * HEX3_0_ptr; // virtual address pointer to HEX3-HEX0
volatile int * HEX5_4_ptr;  // virtual address pointer to HEX5-HEX4
volatile int * KEY_ptr;

int temp = 0;
unsigned char run = 1;
char text[] = "Intel SoC FPGA ";


unsigned char getHexNum(int);
void shiftHexValues();


int main(void)
{

    int fd = -1;               // used to open /dev/mem for access to physical addresses
    int n;
    void *LW_virtual;          // used to map physical addresses for the light-weight bridge
    struct timespec t1 = {0,200000000}, t2;


    // Create virtual memory access to the FPGA light-weight bridge
    if ((fd = open_physical (fd)) == -1)
      return (-1);
    if ((LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)) == NULL)
      return (-1);

    // Set virtual address pointer to I/O port
    LEDR_ptr = (unsigned int *) (LW_virtual + LEDR_BASE);
    *LEDR_ptr = 0x200;

    HEX3_0_ptr = LW_virtual + HEX3_HEX0_BASE;
    HEX5_4_ptr = LW_virtual + HEX5_HEX4_BASE;
    KEY_ptr = LW_virtual + KEY_BASE;

    *(KEY_ptr + 3) = 0xF;
    *HEX5_4_ptr = 0;
    *HEX3_0_ptr = 0;
    *LEDR_ptr = 0;

    printf("\e[2J\e[H ------\n|      |\n ------\n");

    while(1)
    {

        for (n = 0; n < 15;)
        {
            if(*(KEY_ptr + 3))
            {
                if(run)
                    run = 0;
                else
                    run = 1;

                *(KEY_ptr + 3) = 0xF;
            }

            if (run)
            {
                shiftHexValues();
                *HEX3_0_ptr = *HEX3_0_ptr + getHexNum(n);


                printf("\e[02;02H%c%c%c%c%c%c\n\e[?25l",text[n % 15], text[(n+1) % 15], text[(n+2) % 15], text[(n+3) % 15], text[(n+4) % 15], text[(n+5) % 15]);

                nanosleep(&t1, &t2);
                n++;
            }
        }

    }


    *HEX5_4_ptr = 0;
    *HEX3_0_ptr = 0;
    *LEDR_ptr = 0;
    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);   // release the physical-memory mapping
    close_physical (fd);   // close /dev/mem
    return 0;
}

unsigned char getHexNum(int n)
{
    if (n == 0) return 0x10;           //i
    else if (n == 1) return 0x54;      //n
    else if (n == 2) return 0x78;      //t
    else if (n == 3) return 0x79;      //E
    else if (n == 4) return 0x38;      //L
    else if (n == 5) return 0x00;      //
    else if (n == 6) return 0x6D;      //S
    else if (n == 7) return 0x5C;      //o
    else if (n == 8) return 0x39;      //C
    else if (n == 9) return 0x00;      //
    else if (n == 10) return 0x71;     //F
    else if (n == 11) return 0x73;     //P
    else if (n == 12) return 0x7D;     //G
    else if (n == 13) return 0x77;     //A
    else if (n == 14) return 0x00;     //
}

void shiftHexValues()
{
    temp = *HEX3_0_ptr & 0xFF000000;
    temp = temp >> 24;

    *HEX5_4_ptr = (*HEX5_4_ptr << 8) + temp;
    *HEX3_0_ptr = *HEX3_0_ptr << 8;
}

