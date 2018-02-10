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

typedef struct Node{
	tcb * thread;
	struct Node * next;
	
}node;

//priority array - scheduler
 node** priority = NULL;
 node * current_thread;
 node * running_queue;
 
 //for exit and join
 //done array - finished threads
 tcb * done;
 
void my_handler(int signum){
	int i = 0;
	while(i<priorities){
		if(priority[i] == NULL){
			continue;
		}
		node * search = priority[i];
		while(search->next != NULL){
			node * head = search;
			head = head->next;
			running_queue = search;
		}
	}
	
}


void initialize(){
	if(isInit == 0){
		done = (tcb *) malloc(sizeof(tcb*)*MAX_THREADS);
		running_queue = (node *) malloc(sizeof(node*)*MAX_THREADS);
		current_thread = (node *) malloc(sizeof(node));
		signal(SIGALRM, my_handler);
		priority = ( node **)malloc(sizeof( node *)*5);
		int i = 0;
		while(i<priorities){
			priority[i] = NULL;
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
		struct itimerval timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = slice;
		
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = slice;
		
		setitimer(ITIMER_REAL,&timer,NULL);
		
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
		if(priority[p] == NULL){
			priority[p] = insert;
			
			
		}else{
		
		node *ptr = priority[p];
		node * prev = ptr;
		
		while(ptr!=NULL){
			prev = ptr;
			ptr=ptr->next;
		}
		prev->next = insert;
		}
}

void enqueue_running(){
	
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



