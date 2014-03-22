/* This is a mini timer library program
 * for simplicity purpose i am adding all
 * routines in the same file, which should
 * be modularized otherwise 
 * supports only second granularity and not
 * usecond*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define FALSE 0
#define TRUE 1
#define MAX_TIMER 15

typedef int (*timeout_handler)(int usr_ctx);

typedef struct lib_timer{
	struct timeval   expire_timer;
	timeout_handler  handler;
	int 			 usr_ctx;	
	//change usr_ctx to void* type to pass user define
	//parameters, for functionality purpose adding an intger
	//which should be the case in real scenario
	struct lib_timer *prev, *next;
}timer_lib_t;

pthread_mutex_t doubly_lock;
timer_lib_t *head = NULL; 

void print_currenttime()
{
	struct timeval now;	
	struct tm 	*ptm;
	char time_string[40];

	gettimeofday(&now,0);
	
	ptm = localtime (&now.tv_sec); 
    /* Format the date and time, down to a single second. */ 
	strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm); 
	printf("%s\n",time_string);
}

/* This function is junk handler - replace with custom*/
int temp_handler(int ctx)
{
	printf("\n function NO %d called  ; ",ctx);
	print_currenttime();
}

/* REMOVE ME : debugging the doubly. */
int timeout_print()
{
	timer_lib_t **timer_lib_temp = &head,**timer_prev=&head;
	printf("\n printing");
	while(*timer_lib_temp) {
		printf("\n  %d %d \n",
				(*timer_lib_temp)->expire_timer.tv_sec,
				(*timer_lib_temp)->expire_timer.tv_usec);
		timer_prev = timer_lib_temp;
		timer_lib_temp=&((*timer_lib_temp)->next);
	}
	timer_lib_temp=timer_prev;
	printf("\n reverse");
	while(*timer_lib_temp){
		printf("\n  %d %d \n",
				(*timer_lib_temp)->expire_timer.tv_sec,
				(*timer_lib_temp)->expire_timer.tv_usec);
		timer_lib_temp =&((*timer_lib_temp)->prev);
	}
}

int
timeval_subtract (struct timeval *x, struct timeval *y)
{
     struct timeval *result, temp;
	 result = &temp;

  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int timeout_register(int secs,timeout_handler handler,int usr_ctx)
{
	timer_lib_t 	*timer_lib_temp = NULL,**temp_head = &head,*temp_prev = NULL;
	struct timeval   *now;
	timer_lib_temp = malloc(sizeof(timer_lib_t));

	if(timer_lib_temp) {
		gettimeofday(&(timer_lib_temp->expire_timer),0);
		now = &(timer_lib_temp->expire_timer);
		now->tv_sec = now->tv_sec+secs;
	}
	else{
		printf("\n stack up more memory :(");
		return FALSE;
	}

	timer_lib_temp->handler = handler;
	timer_lib_temp->usr_ctx = usr_ctx;

	/* sort the doubly in ascending*/
	pthread_mutex_lock(&doubly_lock);

	while(*temp_head != NULL)
	{
		//difftime(a,b) ---> a-b
		if(timeval_subtract(now,&((*temp_head)->expire_timer)))
			break;
		temp_prev = *temp_head;	
		temp_head = &((*temp_head)->next);
	}

	timer_lib_temp->next = *temp_head;
	timer_lib_temp->prev = temp_prev;	
        if(*temp_head != NULL) {
		(*temp_head)->prev = timer_lib_temp;
	}
	*temp_head = timer_lib_temp;

	pthread_mutex_unlock(&doubly_lock);
}



int timeout_deregister(timer_lib_t **temp_head)
{
	 timer_lib_t *kill_me = *temp_head;;

	*temp_head = (*temp_head)->next;
	 if(*temp_head)
		 (*temp_head)->prev = NULL;
	free(kill_me);

}

void* fire_me()
{
		
	timer_lib_t **temp_head = &head;
	struct timeval now;

	/* I am infinite, i am god */
	while(TRUE){
 
	 
		pthread_mutex_lock(&doubly_lock);

		if(*temp_head){
			gettimeofday(&now,0);

			if(timeval_subtract(&now,&((*temp_head)->expire_timer)))
			{

				// give others a chance to take the lock
				pthread_mutex_unlock(&doubly_lock);
				continue;
			}

			/* Strinking the clock, calling the payload */

			(*temp_head)->handler((*temp_head)->usr_ctx);

			/* rework on the doubly, remove the expired*/

			timeout_deregister(temp_head);
		}
		// give others a chance to take the lock
		pthread_mutex_unlock(&doubly_lock);
	}

}

void* add_random_timer()
{
	int i=0,temp_timeout;
	struct timeval seed;
	
	gettimeofday(&seed,0);

	srandom(seed.tv_sec % 1000);
	
	while(TRUE){
		
		temp_timeout = random()%MAX_TIMER;
		i++;
		printf("\n function NO %d register timeout - %d  ",i ,temp_timeout);	
		print_currenttime();
		timeout_register(temp_timeout,&temp_handler,i);

		//Add timers slowly, lets not put too much in
		sleep(MAX_TIMER/2);

	}
}

int main()
{
	pthread_t thread1,thread2;
	int ithread1,ithread2;

	printf("\n Timer library starting :");
	print_currenttime();

 	ithread1 = pthread_create( &thread1, NULL, fire_me, NULL);
	ithread2 = pthread_create( &thread2, NULL, add_random_timer, NULL);

	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);
	//timeout_print();
}

