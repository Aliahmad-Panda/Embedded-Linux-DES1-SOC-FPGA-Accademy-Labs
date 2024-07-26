#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define BYTES 256                // max number of characters to read from /dev/chardev

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

/* This code uses the character device driver /dev/chardev. The code reads the default
 * message from the driver and then prints it. After this the code changes the message in
 * a loop by writing to the driver, and prints each new message. The program exits if it
 * receives a kill signal (for example, ^C typed on stdin). */
int main(int argc, char *argv[])
{
    int KEY_FD;                        // file descriptor for KEY
    int SW_FD;                         // file descriptor for SW
    int LEDR_FD;                       // file descriptor for LEDR
    int stopwatch_FD;                  // file descriptor for HEX
    char KEY_buffer[BYTES];        // buffer for KEY character data
    char SW_buffer[BYTES];        // buffer for SW character data
    char stopwatch_buffer[BYTES]; // buffer for stopwatch data
    int ret_val, chars_read;           // number of characters read
    int run = 1;                    //for stop and run
    int time_value = 0;
    struct timespec t1 = {0,50000000}, t2;


    // catch SIGINT from ctrl+c, instead of having it abruptly close this program
    signal(SIGINT, catchSIGINT);

    // Open the character device driver for read/write
    if ((KEY_FD = open("/dev/KEY", O_RDWR)) == -1) {
        printf("Error opening /dev/KEY: %s\n", strerror(errno));
        return -1;
    }

    if ((SW_FD = open("/dev/SW", O_RDWR)) == -1) {
        printf("Error opening /dev/SW: %s\n", strerror(errno));
        return -1;
    }

    if ((LEDR_FD = open("/dev/LEDR", O_RDWR)) == -1) {
        printf("Error opening /dev/LEDR: %s\n", strerror(errno));
        return -1;
    }

    if ((stopwatch_FD = open("/dev/stopwatch", O_RDWR)) == -1) {
        printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
        return -1;
    }

    while (!stop) {
        chars_read = 0;
        while ((ret_val = read (KEY_FD, KEY_buffer, BYTES)) != 0)
            chars_read += ret_val;    // read the driver until EOF
        KEY_buffer[chars_read] = '\0';    // NULL terminate
        printf ("%s", KEY_buffer);

        chars_read = 0;
        while ((ret_val = read (SW_FD, SW_buffer, BYTES)) != 0)
            chars_read += ret_val;    // read the driver until EOF
        SW_buffer[chars_read] = '\0';    // NULL terminate
        printf ("%s", SW_buffer);

        write (LEDR_FD, SW_buffer, strlen(SW_buffer));

        if (*KEY_buffer == '1')
        {
            if (run)
            {
                write (stopwatch_FD, "stop", strlen("stop"));
                run = 0;
            }
            else
            {
                write (stopwatch_FD, "run", strlen("run"));
                run = 1;
            }
        }

        else if (*KEY_buffer == '2')
        {
            sscanf(SW_buffer, "%x", &time_value);
            time_value = time_value % 100;
            sprintf(stopwatch_buffer, "::%d", time_value);
            write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
        }

        else if (*KEY_buffer == '4')
        {
            sscanf(SW_buffer, "%x", &time_value);
            time_value = time_value % 60;
            sprintf(stopwatch_buffer, ":%d:", time_value);
            write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
        }

        else if (*KEY_buffer == '8')
        {
            sscanf(SW_buffer, "%x", &time_value);
            time_value = time_value % 60;
            sprintf(stopwatch_buffer, "%d", time_value);
            write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
        }


        printf("run = %d\n\n", run);
        nanosleep(&t1, &t2);
    }
    close (KEY_FD);
    close (SW_FD);
    close (LEDR_FD);
    close (stopwatch_FD);
    return 0;
}
