//Game in which user is given mathematical questions and has to answer them in a given time.


#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


#define BYTES 256                // max number of characters to read from /dev/chardev

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

int KEY_FD;                    // file descriptor
int SW_FD;
int LEDR_FD;
int HEX_FD;
int stopwatch_FD;

char KEY_buffer[BYTES];        // buffer for chardev character data
char SW_buffer[BYTES];
char HEX_buffer[BYTES];
char stopwatch_buffer[BYTES];

int ret_val, chars_read, run = 0;          // number of characters read from chardev

int SW_msg = 0;
int tim = 0;
int SW_values = 0;
struct timespec t1 = {0,50000000} , t2;



void gettime(void)
{
    chars_read = 0;
    while ((ret_val = read (stopwatch_FD, stopwatch_buffer, BYTES)) != 0)
        chars_read += ret_val;    // read the driver until EOF
    stopwatch_buffer[chars_read] = '\0';    // NULL terminate
}

float gettime_in_sec(void)
{
    int min = 0;
    int sec = 0;
    int hund_sec = 0;
    float time_in_sec = 0.0;

    gettime();

    sscanf(stopwatch_buffer, "%d:%d:%d", &min, &sec, &hund_sec);
    time_in_sec = (min*60) + sec + (hund_sec/100.0);
    return time_in_sec;
}





/* This code uses the character device driver /dev/chardev. The code reads the default
 * message from the driver and then prints it. After this the code changes the message in
 * a loop by writing to the driver, and prints each new message. The program exits if it
 * receives a kill signal (for example, ^C typed on stdin). */

int main(int argc, char *argv[])
{
    srand(time(NULL)); // seed the random number generator
    // catch SIGINT from ctrl+c, instead of having it abruptly close this program
    signal(SIGINT, catchSIGINT);

    float time_per_question = 0.0;
    int score;
    int game_over = 0;
    int time_qs_hund = 0;
    int time_qs_sec = 0;
    int time_qs_min = 0;
    int total_time = 0;


    // Open the character device driver for read/write
    if ((KEY_FD = open("/dev/KEY", O_RDWR)) == -1) {
        printf("Error opening /dev/KEY: %s\n", strerror(errno));
        return -1;
    }
    if ((SW_FD = open("/dev/SW", O_RDWR)) == -1) {
        printf("Error opening /dev/SW: %s\n", strerror(errno));
        return -1;
    }
    if ((stopwatch_FD = open("/dev/stopwatch", O_RDWR)) == -1) {
        printf("Error opening /dev/HEX: %s\n", strerror(errno));
        return -1;
    }

    

    write(stopwatch_FD, "stop" , strlen("stop"));

    while (!stop) {
        chars_read = 0;
        while ((ret_val = read (KEY_FD, KEY_buffer, BYTES)) != 0)
            chars_read += ret_val;    // read the driver until EOF
        KEY_buffer[chars_read] = '\0';    // NULL terminate
        //printf ("%s", KEY_buffer);

        chars_read = 0;
            while ((ret_val = read (SW_FD, SW_buffer, BYTES)) != 0)
                chars_read += ret_val;    // read the driver until EOF
        SW_buffer[chars_read] = '\0';    // NULL terminate
        printf ("%s", SW_buffer);


        if (*KEY_buffer == '1'){
            //run = 1 ^ run;
            time_per_question = gettime_in_sec();
            write(stopwatch_FD, "run" , strlen("run"));
            break;
        }

        if (*KEY_buffer == '2'){
            sscanf(SW_buffer, "%x", &tim);
            sprintf(stopwatch_buffer, "::%d", (tim%100));
            time_qs_hund = tim%100;
            write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
        }
        else if (*KEY_buffer == '4'){
            sscanf(SW_buffer, "%x", &tim);
            sprintf(stopwatch_buffer, ":%d", (tim%60));
            time_qs_sec = tim%60;
            write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
        }
        else if (*KEY_buffer == '8'){
            sscanf(SW_buffer, "%x", &tim);
            sprintf(stopwatch_buffer, "%d", (tim%60));
            time_qs_min = tim%60;
            write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
        }

        nanosleep(&t1, &t2);
    }


    while (!game_over)
    {
        int num1 = rand() % (5+(score*score));
        int num2 = rand() % (5+(score*score));
        int answer = num1 + num2;
        int user_answer = 0;
        printf("%d + %d = ", num1, num2);
        scanf("%d", &user_answer);
        while(1){
            float time_left = gettime_in_sec();
            if (!time_left)
            {   
                game_over = 1;
                break;
            }
            
            if (user_answer == answer)
            {
                score++;
                printf("Correct!\n");
                sprintf(stopwatch_buffer, "%d:%d:%d", time_qs_min, time_qs_sec, time_qs_hund);
                write(stopwatch_FD, stopwatch_buffer, strlen(stopwatch_buffer));
                total_time +=  time_per_question - time_left;
                break;
            }
            else
            {
                printf("Try again: ");
                scanf("%d", &user_answer);
            }
        }
    }
    printf("Game over! Your score is %d\n", score);
    printf("Average Time taken is: %.2f\n", total_time/(score*1.0));
    
    write(stopwatch_FD, "stop", strlen("stop"));
    write(stopwatch_FD, "nodisp", strlen("nodisp"));
    write(LEDR_FD, "000", strlen("000"));

    close (KEY_FD);
    close (LEDR_FD);
    close (HEX_FD);
    close (SW_FD);

    return 0;
}
