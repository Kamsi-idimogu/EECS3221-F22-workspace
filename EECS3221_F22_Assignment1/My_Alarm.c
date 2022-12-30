/*
 * alarm_mutex.c
 *
 * This is an enhancement to the alarm_thread.c program, which
/*
 * alarm_mutex.c
 *
 * This is an enhancement to the alarm_thread.c program, which
 * created an "alarm thread" for each alarm command. This new
 * version uses a single alarm thread, which reads the next
 * entry in a list. The main thread places new requests onto the
 * list, in order of request number. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */

int seconds_temp;
time_t time_temp; /* seconds from EPOCH */
char message_temp[64];
int alarm_counter = 0;

// Mutex initialization
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;


// Thread declarations
pthread_t thread;
pthread_t display;

// Type: odd -> 1, even -> 0
int type;

// Conditional variable declaration
pthread_cond_t display_cond;


typedef struct alarm_tag
{
    struct alarm_tag *link;
    int seconds;
    time_t time; /* seconds from EPOCH */
    char message[64];
    int Alarm_Request_Number;
} alarm_t;

alarm_t *alarm_list = NULL;
alarm_t *display_1_List = NULL;
alarm_t *display_2_List = NULL;



void *alarm_thread()
{
    alarm_t *alarm;
    int sleep_time;
    int status;

    while (1)
    {
        status = pthread_mutex_lock(&display_mutex); // locking alarm thread

        if (status != 0)
            err_abort(status, "Lock mutex");

        alarm = alarm_list; //current alarm

        if (alarm == NULL)
            sleep_time = 1;
        else
        {
            if (alarm->Alarm_Request_Number % 2 == 0) // Even request number
            {
                type = 0;
                printf("Alarm Thread Passed on Alarm Request to Display Thread 2 Alarm Request Number: (%d) Alarm Request: %d %s\n", alarm->Alarm_Request_Number, alarm->seconds, alarm->message);
                display_2_List = alarm;
                pthread_cond_signal(&display_cond);
            }
            if(alarm->Alarm_Request_Number % 2 != 0)   // Odd request number
            {
                type = 1;
                printf("Alarm Thread Passed on Alarm Request to Display Thread 1 Alarm Request Number: (%d) Alarm Request: %d %s\n", alarm->Alarm_Request_Number, alarm->seconds, alarm->message);
                display_1_List = alarm;
                pthread_cond_signal(&display_cond);
            }
            alarm_list = alarm_list->link;
        }


        status = pthread_mutex_unlock(&display_mutex);

        if (status != 0)
            err_abort(status, "Unlock mutex");

        if (sleep_time > 0)
            sleep(sleep_time);
        else
            sched_yield();
    }
}

void *display_thread()
{
    alarm_t *alarm;
    time_t now;
    int sleep_time, seconds_left, thread_number, t;

    sleep_time = 0;
    while (1)
    {
        pthread_mutex_lock(&display_mutex);
        now = time(NULL);
        sleep(1);
        pthread_cond_wait(&display_cond, &display_mutex); // here

        if(type == 1) {
            thread_number = 1;
            alarm = display_1_List;
        }else{
            thread_number = 2;
            alarm = display_2_List;
        }


        printf("Display Thread %d: Received Alarm Request Number:(%d) Alarm Request: %d %s\n", thread_number,
               alarm->Alarm_Request_Number, alarm->seconds, alarm->message);

        if (alarm->time <= now)
            sleep_time = 0;
        else
            sleep_time = alarm->time - now;

        seconds_left = alarm->seconds;

        if(sleep_time > 0) {
            while (seconds_left > 0) {

                if (alarm->time - now > 0) {
                    printf("Display Thread: %d Number of Seconds Left: %d Alarm Request Number: %d Alarm Request: %d %s \n",
                           thread_number,
                           seconds_left, alarm->Alarm_Request_Number, alarm->seconds,alarm->message);
                    seconds_left = seconds_left-2;
                    sleep(2);

                }
            }
        }
        printf("Display Thread: %d Alarm Expired at: %d Alarm Request Number: %d Alarm Request: %d %s \n", thread_number , time(NULL), alarm->Alarm_Request_Number, alarm->seconds, alarm->message);

        pthread_mutex_unlock(&display_mutex);
    }
}



void receiveAlert(){
    char line[128];
    alarm_t *alarm, **last, *next;
    int status;
    int wait = -1;

    while (1)
    {
        if(wait == -1) //First request
            printf("alarm> ");
        if(wait != -1) //rest of requests
        {
            sleep(wait + 2);
            printf("alarm> ");
        }

        if (fgets(line, sizeof(line), stdin) == NULL)
            exit(0);

        if (strlen(line) <= 1)
            continue;

        alarm = (alarm_t *)malloc(sizeof(alarm_t));

        if (alarm == NULL)
            errno_abort("Allocate alarm");

        // Parse input line into seconds (%d) and a message (%64[^\n]), consisting of up to 64 characters separated from the seconds by whitespace.

        if (sscanf(line, "%d %64[^\n]",
                   &alarm->seconds, alarm->message) < 2)
        {

            fprintf(stderr, "Bad command\n");
            free(alarm);
        }
        else
        {
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            alarm->Alarm_Request_Number = alarm_counter + 1;

            printf("Main Thread Received Alarm Request Number:(%d) Alarm Request: %d %s \n", alarm->Alarm_Request_Number, alarm->seconds, alarm->message);
            alarm_counter++;

            alarm->time = time(NULL) + alarm->seconds;
            wait = alarm->seconds;

            // Inserts the new alarm into the list of alarms
            last = &alarm_list;
            next = *last;

            while (next != NULL)
            {
                last = &next->link;
                next = next->link;
            }

            if (next == NULL)
            {
                *last = alarm;
                alarm->link = NULL;
            }

#ifdef DEBUG
            printf("[list: ");
            for (next = alarm_list; next != NULL; next = next->link)
                printf("%d(%d)[\"%s\"] ", next->time,
                       next->time - time(NULL), next->message);
            printf("]\n");
#endif
            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock mutex");
        }
    }
}



int main(int argc, char *argv[])
{
    // Thread status declarations
    int alarm_status, display_status;

    // Thread status Initializations
    alarm_status = pthread_create(&thread, NULL, &alarm_thread, NULL);
    display_status = pthread_create(&display, NULL, &display_thread, NULL);


    // Initialize conditional variable
    pthread_cond_init(&display_cond, NULL);


    // Errors if any thread fails to be created
    if (alarm_status != 0)
        err_abort(alarm_status, "Create alarm thread failed.");

    if (display_status != 0)
        err_abort(display_status, "Create display 1 thread failed.");



    // Receives alert typed in from user
    receiveAlert();


    // Join statements to make main thread wait for other threads to finish
    if (pthread_join(thread, NULL) != 0)
    {
        return 1;
    }

    if (pthread_join(display, NULL) != 0)
    {
        return 2;
    }



}


