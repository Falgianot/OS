// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>


//defining global vars
//4096
#define STACK_SIZE 1024*64
#define MAX_THREADS 64
#define slice 25000
#define priorities 5
static ucontext_t uctx_main;
static ucontext_t uctx_handler;
int tid = 1;
int isInit = 0;
tcb * running_thread;
struct itimerval timer;
struct timeval interval;
struct itimerval thread_timer;
struct timeval thread_interval;


int pick [5]= {16, 8 ,6 ,4 ,2};
int times [5] = {15000, 30000,45000, 50000, 55000};

void pause_timer(){
	struct itimerval zero_timer = {0};
	setitimer(ITIMER_REAL, &zero_timer, &timer);
}

void resume_timer(){
	setitimer(ITIMER_REAL, &timer, NULL);
}





typedef struct Node{
	tcb * thread;
	struct Node * next;
	
}node;

typedef struct Queue{
	struct Node * front;
	struct Node * tail;
	int size;
}queue;

struct Queue *createQueue()
{
    struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->tail = NULL;
	q->size = 0;
    return q;
}

queue * ready_queue;
queue * waiting_queue;
queue * running_queue;

//priority array - scheduler
 queue** priority = NULL;
 node * current_thread;

 
 //for exit and join
 //done array - finished threads
 tcb * done;
 
 void enqueue_running(node * insert){
		
		
		if(running_queue->front == NULL){
			running_queue->front = insert;
			running_queue->tail = insert;
			
		}else{
			running_queue->tail->next = insert;
			running_queue->tail = insert;
		
		}
		running_queue->size = running_queue->size + 1;
}

struct Node * dequeue_running(){
	if(running_queue->front == NULL){
		return NULL;
	}
	
	node * delete = running_queue->front;
	running_queue->front = running_queue->front->next;
	
	if(running_queue->front == NULL){
		running_queue->tail = NULL;
	}
	//what do we want to do with it
	running_queue->size = running_queue->size - 1;
	return delete;
}
struct Node * dequeue(int p){
	queue * q = priority[p];
	
	if(q->front == NULL){
		return NULL;
	}
	
	node * delete = q->front;
	q->front = q->front->next;
	
	if(q->front == NULL){
		q->tail = NULL;
	}
	//what do we want to do with it
	q->size = q->size - 1;
	return delete;
	
}


 void print_running(){
	node * search = running_queue->front;
	while(search!=NULL){
		tcb * cb = search->thread;
		printf("tid:%i\n",cb->tid);
		search = search->next;
	}
}
 
 
 void contextSwap(){
	
 }
 
 
 
 
 //pick 16,8,4,2,1
void my_handler(int signum){
	int i = 0;
	//start inserting into running queue
	
	
	while(i<priorities){
		//Get number of threads we are picking at the priority level and enqueue into running queue
		//If there aren't enough threads in that level just go to the next
		int p = pick[i];
		queue * q = priority[i];
		int k = 0;
		while(priority[i]!=NULL&&k<p){
			node * node_leaving = dequeue(i);
			enqueue_running(node_leaving);
			k++;
		}
		i++;
	}
	
	
	if(running_thread->state == running){
		//Thread is not finished so put it back in the scheduler
		
	}else if(running_thread->state == terminate){
		//Thread called exit so put it in the done queue
	}
	
	
	//swapping contexts
	while(running_queue->size>0){
		node * temp = dequeue_running();
		running_thread = temp->thread;
		int running_priority = running_thread->priority;
		
		thread_interval.tv_sec = 0;
		thread_interval.tv_usec = times[running_priority];
			
		thread_timer.it_interval = thread_interval;
		thread_timer.it_value = thread_interval;
		
		
			
		setitimer(ITIMER_REAL,&thread_timer,NULL);
		
		swapcontext(&uctx_handler,&running_thread->cxt);
	
	}
	
	
}


void initialize(){
	if(isInit == 0){
		done = (tcb *) malloc(sizeof(tcb*)*MAX_THREADS);
		running_queue = createQueue();
		waiting_queue = createQueue();
		//ready_queue = createQueue();
		current_thread = (node *) malloc(sizeof(node));
		signal(SIGALRM, my_handler);
		priority = ( queue **)malloc(sizeof( queue *)*5);
		int i = 0;
		while(i<priorities){
			priority[i] = createQueue();
			i++;
		}
		
		if(getcontext(&uctx_main)==-1){
		perror("getcontext failed");
		exit(0);
	}
		
		if(getcontext(&uctx_handler)==-1){
		perror("getcontext failed");
		exit(0);
	}
		void * stack = malloc(STACK_SIZE);
		uctx_handler.uc_stack.ss_sp = stack;
		uctx_handler.uc_stack.ss_size = STACK_SIZE;
		uctx_handler.uc_link = 0;
		uctx_handler.uc_stack.ss_flags=0;
		makecontext(&uctx_handler,(void *)my_handler,1, SIGALRM);
		
		
		//setting itimer
		
		interval.tv_sec = 0;
		interval.tv_usec = slice;
		
		
		timer.it_interval = interval;
		timer.it_value = interval;
		
		setitimer(ITIMER_REAL,&timer,NULL);
		
		
		while(1){
			
		}
		isInit = 1;
	}else{
		return;
	}
}



void enqueue(int p, tcb * cb){
		node * insert = (node *) malloc(sizeof(node));
		cb->state = ready;
		insert->thread = cb;
		insert->next = NULL;
		
		queue * q = priority[p];
		
		if(q->front == NULL){
			q->front = insert;
			q->tail = insert;
			
		}else{
			q->tail->next = insert;
			q->tail = insert;
		
		}
		q->size = q->size + 1;
}




void print_schedule(){
	int i = 0;
	while(i<priorities){
		queue * q = priority[i];
		node * search = q->front;
		while(search!=NULL){
			tcb * cb = search->thread;
			printf("Priority:%i \t Size:%i tid:%i\n",i,q->size,cb->tid);
			search = search->next;
		}
		i++;
	}

}



/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	initialize();
	
	tcb *control_block = (tcb*)malloc(sizeof(tcb));
	control_block->tid = tid;
	tid++;
	control_block->state = embryo;
	
	
	ucontext_t c;
	
	if(getcontext(&c)==-1){
		perror("getcontext failed");
		exit(0);
	}
	
	
	
	
	void * stack = malloc(STACK_SIZE);
	c.uc_stack.ss_sp = stack;
	c.uc_stack.ss_size = STACK_SIZE;
	c.uc_link = &uctx_handler;
	c.uc_stack.ss_flags=0;
	control_block->stack = stack;
	control_block->next = NULL;
	thread[0]=control_block->tid;
	
	makecontext(&c,(void*)function,1,arg);
	control_block->cxt = c;
	

		//must add timesplice and priority later
		//count for error if insufficient stack space etc.
		
		enqueue(0, control_block);
		raise(SIGALRM);
		
		
		
		//my_pthread_yield();
	return 0;
	};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	raise(SIGALRM);
	
	return 0;
};

/* terminate a thread */
//make done array
//set tcb ret val to value ptr
//for join continue based on TID check, set value_ptr
//add to complete just in case thread calls join
void my_pthread_exit(void *value_ptr) {
	
};

/* wait for thread termination */

int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};



