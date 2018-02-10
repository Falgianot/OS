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
#define splice 25
static ucontext_t uctx_main;
int tid = 1;



typedef struct Node{
	tcb * thread;
	struct Node * next;
	
}node;

 node** priority = NULL;
 
int isInit = 0;

//initializeScheduler
void initialize(){
	if(isInit == 0){
		priority = ( node **)malloc(sizeof( node *)*5);
		int i = 0;
		
		
		while(i<5){
			priority[i] = NULL;
			i++;
		}
		
		
		isInit = 1;
	}else{
		return;
	}
	
}

//handles signals
//signals used if time runs out or if pthread yield called.
void my_handler(int signum){
	if(signum==SIGUSR1){
		//scheduler stuff
		
		
	}
}

/*inserts into scheduler
Params: p - priority level. cb - thread
*/
void enqueue(int p, tcb * cb){
		node * insert = (node *) malloc(sizeof(node));
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

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	
	signal(SIGALRM, my_handler);
	
	tcb *control_block = (tcb*)malloc(sizeof(tcb));
	control_block->tid = tid;
	tid++;
	control_block->state = embryo;
	
	if(getcontext(&control_block->cxt)==-1){
		perror("getcontext failed");
		exit(0);
	}
	
	ucontext_t c = control_block->cxt;
	void * stack = malloc(STACK_SIZE);
	c.uc_stack.ss_sp = stack;
	c.uc_stack.ss_size = STACK_SIZE;
	c.uc_link = &uctx_main;
	c.uc_stack.ss_flags=0;
	control_block->stack = stack;
	control_block->next = NULL;
	thread[0]=control_block->tid;
	
	makecontext(&c,(void*)function,1,arg);
	swapcontext(&uctx_main,&c);
	//swapcontext(&c,&uctx_main);

		//must add timesplice and priority later
		//count for error if insufficient stack space etc.
		initialize();
		enqueue(0, control_block);
		
		
		
		my_pthread_yield();
	return 0;
	};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	raise(SIGALRM);
	
	return 0;
};

/* terminate a thread */
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



