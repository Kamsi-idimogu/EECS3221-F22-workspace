

/*
 * alarm_cond.c
 *
 * This is an enhancement to the alarm_mutex.c program, which
 * used only a mutex to synchronize access to the shared alarm
 * list. This version adds a condition variable. The alarm
 * thread waits on this condition variable, with a timeout that
 * corresponds to the earliest timer request. If the main thread
 * enters an earlier timeout, it signals the condition variable
 * so that the alarm thread will wake up and process the earlier
 * timeout first, requeueing the later request.
 */
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "errors.h"
#include <semaphore.h>
#include <string.h>

#define MAX_BUFFER_SIZE 4
#define MAX_MESSAGE_LENGTH 128




typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[64];
    int                 message_number;
    int                 type;   // type a -> 0, type b -> 1
} alarm_t;

alarm_t buffer[MAX_BUFFER_SIZE];
int head;
int tail;


pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBuffer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_cond = PTHREAD_COND_INITIALIZER;
alarm_t *alarm_list = NULL;
alarm_t *alarm_display_list = NULL;
time_t current_alarm = 0;

sem_t empty;
sem_t full;
sem_t mutex;







void insert_into_buffer(alarm_t *element)
{

    // add to buffer

    sem_wait(&empty);
    sem_wait(&mutex);

    buffer[tail] = *element;
    tail = (tail + 1) % MAX_BUFFER_SIZE;

    sem_post(&mutex);
    sem_post(&full);

}

alarm_t retrieve_from_buffer()
{
    alarm_t *element;


    // remove from buffer
    sem_wait(&full);
    sem_wait(&mutex);

    element = &buffer[head];
    head = (head + 1) % MAX_BUFFER_SIZE;

    sem_post(&mutex);
    sem_post(&empty);

      alarm_t **last, *next;

        last = &alarm_display_list;
        next = *last;
        alarm_t *alarmToInsert;

    
   
                alarmToInsert = (alarm_t*) malloc(sizeof(alarm_t));
                ToInsert->time = element->time;
                ToInsert->seconds = element->seconds;
                //ToInsert->message = element->message;
                strcpy(ToInsert->message, element->message);
                ToInsert->type = element->type;
                ToInsert->message_number= element->message_number;

                //Sort Array by request number

        while (next != NULL) {
            if (next->message_number >= ToInsert->message_number) {
                ToInsert->link = next;
                *last = ToInsert;
                break;
            }
            last = &next->link;
            next = next->link;
        }
   
        if (next == NULL) {
            *last = ToInsert;
            ToInsert->link = NULL;
      
            
        }
     
    return *ToInsert;

}






/*
 * Insert element entry on alarm list, in order.
 */
void alarm_insert (alarm_t *alarm)
{
    int status;
    alarm_t **last, *next;

    /*
     * LOCKING PROTOCOL:
     *
     * This routine requires that the caller have locked the
     * alarm_mutex!
     */
    last = &alarm_list;
    next = *last;
    while (next != NULL) {
        if (next->time >= alarm->time) {
            alarm->link = next;
            *last = alarm;
            break;
        }
        last = &next->link;
        next = next->link;
    }
    /*
     * If we reached the end of the list, insert the new alarm
     * there.  ("next" is NULL, and "last" points to the link
     * field of the last item, or to the list header.)
     */
    if (next == NULL) {
        *last = alarm;
        alarm->link = NULL;
    }
    /*
     * Wake the alarm thread if it is not busy (that is, if
     * current_alarm is 0, signifying that it's waiting for
     * work), or if the new alarm comes before the one on
     * which the alarm thread is waiting.
     */
    if (current_alarm == 0 || alarm->time < current_alarm) {
        current_alarm = alarm->time;
        status = pthread_cond_signal (&alarm_cond);
        if (status != 0)
            err_abort (status, "Signal cond");
    }
}




void remove_from_display_list(int message_number)
{
    alarm_t *previous, *current;



    if (alarm_display_list == NULL) {
        return;
    }

    if (alarm_display_list->message_number == message_number) {
        alarm_display_list = alarm_display_list->link;
        return;
    }

    

    previous = alarm_display_list;
    current = alarm_display_list->link;
    while (current != NULL && current->message_number != message_number) {
        previous = current;
        current = current->link;
    }
    if (current != NULL) {
        previous->link = current->link;
    }
}

/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    struct timespec cond_time;
    time_t now;
    int status, expired;



    status = pthread_mutex_lock (&alarm_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");
    while (1) {
        /*
         * If the alarm list is empty, wait until an alarm is
         * added. Setting current_alarm to 0 informs the insert
         * routine that the thread is not busy.
         */
         
        current_alarm = 0;
        while (alarm_list == NULL) {
            status = pthread_cond_wait (&alarm_cond, &alarm_mutex);
            if (status != 0)
                err_abort (status, "Wait on cond");
        }
        

        alarm = alarm_list;
        alarm_list = alarm->link;
        if(alarm->type == 0){ // type a
            insert_into_buffer(alarm);                  // insert type-a alarm into circular_buffer
            now = time (NULL);
            expired = 0;
            if (alarm->time > now) {

                cond_time.tv_sec = alarm->time;
                cond_time.tv_nsec = 0;
                current_alarm = alarm->time;
                while (current_alarm == alarm->time) {
                    status = pthread_cond_timedwait (
                            &alarm_cond, &alarm_mutex, &cond_time);
                    if (status == ETIMEDOUT) {
                        expired = 1;
                        break;
                    }
                    if (status != 0)
                        err_abort (status, "Cond timedwait");
                }
                if (!expired)
                    alarm_insert (alarm);
            } else
                expired = 1;
            if (expired) {
                printf ("(%d) %s\n", alarm->seconds, alarm->message);


                free (alarm);
            }
        }else{ // type b
            printf("Cancel: Message(%d)\n",alarm->message_number);
            remove_from_display_list(alarm->message_number); // remove from alarm display list
            free (alarm);
        }

    }
}

void *consumer (void *arg)
{
    while(1){
        alarm_t retrieved_alarm = retrieve_from_buffer();

    }

}


void *display (void *arg)
{
 



    int newLine = 0;
 
    while(1){
          sleep(3); // sleep for 3 seconds between display periods
          alarm_t * current = alarm_display_list;
    
       

        while (current != NULL && current->link != NULL) {
            // print alarm data
            printf("Seconds: %d, Type-a Alarm, Message: %s, Time-inserted: %d, Message Number: %d\n", current->seconds, current->message, current->time, current->message_number);
         printf("Seconds: %d, Type-a Alarm, Message: %s, Time-inserted: %d, Message Number: %d\n", current->link->seconds, current->link->message, current->link->time, current->link->message_number);
            printf("\n");
            sleep(2);   // sleep for 2 seconds after printing
            current = current->link->link;
        }

         if (current != NULL) {
            printf("Seconds: %d, Type-a Alarm, Message: %s, Time-inserted: %d, Message Number: %d\n", current->seconds, current->message, current->time, current->message_number);
            newLine = 1;
        }


        if(newLine == 1) printf("\n");

      
    }
}



void receiveAlert(){
    int status;
    char line[128];
    alarm_t *alarm;
    char *input;
    int number;
    int errorFlag; // 1-> bad command 0-> valid command

    while (1) {
        printf ("Alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL) errno_abort ("Allocate alarm");

        if (sscanf (line, "%d Message(%d) %64[^\n]", &alarm->seconds, &alarm->message_number, alarm->message) < 3){
            if (sscanf (line, "Cancel: Message(%d)", &alarm->message_number) < 1)
            {
                fprintf (stderr, "Bad command\n");
                free (alarm);
                errorFlag = 1;
            }
            else
            {
                alarm->seconds = 0;
                strcpy(alarm->message,"Cancel command");
                alarm->type = 1;
                errorFlag = 1;
            }
        }
        else{
            alarm->type = 0;
        }

        if (errorFlag == 0)
        {

            status = pthread_mutex_lock (&alarm_mutex);

            if (status != 0)  err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;

            alarm_insert (alarm);    //  Insert the new alarm into the alarm list, sorted by alarm number.

            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0) err_abort (status, "Unlock mutex");

        }



    }
}





int main (int argc, char *argv[])
{
    int alarm_status, consumer_thread_status, periodic_display_thread_status;

    sem_init(&empty, 0, 4);
    sem_init(&full, 0, 0);
    sem_init(&mutex,0,1);

// Thread declarations

pthread_t thread;
pthread_t consumer_thread;
pthread_t  periodic_display_thread;




    alarm_status = pthread_create (&thread, NULL, alarm_thread, NULL);
    consumer_thread_status = pthread_create (&consumer_thread, NULL, consumer, NULL);
    periodic_display_thread_status = pthread_create (&periodic_display_thread, NULL, display, NULL);



    // Errors if any thread fails to be created

    if (alarm_status != 0)
        err_abort (alarm_status, "Create alarm thread");

    if (consumer_thread_status != 0)
        err_abort (consumer_thread_status, "Create consumer thread");

    if (periodic_display_thread_status != 0)
        err_abort (periodic_display_thread_status, "Create display thread");



    // Receives alert typed in from user
    receiveAlert();

    // Join statements to make main thread wait for other threads to finish
    if (pthread_join(thread, NULL) != 0)
    {
        return 1;
    }

    if (pthread_join(consumer_thread, NULL) != 0)
    {
        return 2;
    }

    if (pthread_join(periodic_display_thread, NULL) != 0)
    {
        return 3;
    }

    pthread_mutex_destroy(&mutexBuffer);
    sem_destroy(&empty);
    sem_destroy(&full);

}