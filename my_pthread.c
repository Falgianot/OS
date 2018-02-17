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
#define STACK_SIZE 1024*10
#define MAX_THREADS 64
#define slice 2500
#define priorities 5
 ucontext_t * uctx_main;
 ucontext_t * uctx_handler;
 ucontext_t * uctx_garbage;
int tid = 1;
int isInit = 0;
tcb * running_thread;
struct itimerval timer;
struct timeval interval;
int mainDone = 0;
int firstSwap = 0;
int timeCounter =-1;
int pick [5]= {16, 8 ,6 ,4 ,2};



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

queue * done_queue;
queue * waiting_queue;
queue * running_queue;

//priority array - scheduler
 queue** priority = NULL;
 tcb * current_thread;
 tcb * prev_thread;
 tcb * mainThread;

 
 
 
 //enqueue and dequeue for waiting, done, running
 void enqueue_other(node * insert, queue * q){
		
		
		if(q->front == NULL){
			q->front = insert;
			q->tail = insert;
			
		}else{
			q->tail->next = insert;
			q->tail = insert;
		
		}
		q->size = q->size + 1;
}

node * dequeue_other(queue * q){
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


//enqueue and dequeue for our scheduler
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

node * dequeue(int p){
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
 
 
 
 //pick 16,8,4,2,1
 //try to only call swap once during function call
void my_handler(int signum){
	//running thread
	//yield
	//maintenance
	//insert into running
	
	
	//If thread called pthread_exit. Insert into done queue
	if(firstSwap!=0&&running_thread!=NULL&&running_thread->state == terminate){
		node * toInsert = (node *)malloc(sizeof(node));
		toInsert->thread = running_thread;
		toInsert->next = NULL;
		enqueue_other(toInsert,done_queue);
		prev_thread = running_thread;
		free(running_thread->cxt->uc_stack.ss_sp);
		running_thread = NULL;
	}
	
	//If running thread is not null, that means the thread got interrupted
	if(running_thread!=NULL){
		
		//This is the case for if the thread did not finish running for its time slice(timeslice * priority)
		if(timeCounter != running_thread->priority){
			timeCounter++;
			return;
		}
		else{
			
		//These cases are for if the thread is not done so insert it back into scheduler down a priority unless its already 4 because that is the max
		printf("I am running for %d\n",timeCounter);
		if(running_thread->priority ==4){
			prev_thread = running_thread;
			enqueue(4,running_thread);
			timeCounter = -1;
		}
		else{
			running_thread->priority = running_thread->priority +1;
			prev_thread = running_thread;
			enqueue(running_thread->priority,running_thread);
			timeCounter =-1;
		}
		running_thread = NULL;
		}
		
	}

	//start inserting into running queue	
	if(running_queue->size==0){
	int i = 0;
	while(i<priorities){
		printf("inserting into running\n");
		//Get number of threads we are picking at the priority level and enqueue into running queue
		//If there aren't enough threads in that level just go to the next
		int p = pick[i];
		queue * q = priority[i];
		int k = 0;
		while(priority[i]->size>0&&k<p){
			node * node_leaving = dequeue(i);
			enqueue_other(node_leaving,running_queue);
			k++;
			}
		i++;
		}
	}

	
	 if(running_queue->size != 0){
	//run all threads in running queue
	//	while(running_queue->size>0 && firstswap == 0){
		
		//dequeue from running queue
		node * temp = dequeue_other(running_queue);
		running_thread = temp->thread;
		int running_priority = running_thread->priority;
	
		
		//Cases to account for when doing first swap.
		//Main thread will get dequeued first
		//Make it swap to another context
		if(firstSwap == 0 && running_thread->isMain == 1){
			printf("main dequeue\n");
			prev_thread = running_thread;
			enqueue(0,running_thread);
			firstSwap = 1;
			node * temp = dequeue_other(running_queue);
			running_thread = temp->thread;
			//might have to move around nodes instead of just freeing
			free(temp);
			int running_priority = running_thread->priority;
			timeCounter++;
			swapcontext(prev_thread->cxt, running_thread->cxt);
		

		}else{
			//GO THROUGH THE ENTIRE RUNNING QUEUE
			//if we encounter the same context, let it run again
			if(prev_thread->cxt == running_thread ->cxt){
				printf("swap same context\n");
				timeCounter++;
				return;
			}
			else{
			//Swap from current context to a new one
			printf("prev:%i   run:%i\n",prev_thread->tid,running_thread->tid);
			timeCounter++;
			swapcontext(prev_thread->cxt, running_thread->cxt);
			}
			
		}
		//Check entire waiting queue for mutexes and joins
		
	
		//}
	}
	printf("gone\n");

	
}

//switch back to main when everything is done
void garbage(){
	
	if(running_thread->isMain==1){
		//main is done
		printf("main done\n");
		mainDone = 1;
		
	}
	printf("garbage\n");
	prev_thread = running_thread;
	running_thread =  NULL;
	printf("garbage1\n");
	raise(SIGALRM);
}

void initialize(){
	if(isInit == 0){
		done_queue = createQueue();//for threads that are done, and might be waited on
		running_queue = createQueue(); // queue for threads that are ready to run
		waiting_queue = createQueue();// queue for threads waiting on a lock
		running_thread = NULL;
		//ready_queue = createQueue();
		current_thread = (tcb *) malloc(sizeof(tcb)); //thread we are currently executing for a certain time slice
		signal(SIGALRM, my_handler); //linking our handler to the OS
		priority = ( queue **)malloc(sizeof( queue *)*5); //our scheduler data structure. an array of queues
		int i = 0;
		
		//initialize our array of queues(scheduler)
		while(i<priorities){
			priority[i] = createQueue();
			i++;
		}
		
		
		
		//Create a context for our garbage collector. All threads will go here once done
		uctx_garbage = (ucontext_t *)malloc(sizeof(ucontext_t));
		if(getcontext(uctx_garbage)==-1){
		perror("getcontext failed");
		exit(0);
	}
		void * garstack = malloc(STACK_SIZE);
		uctx_garbage->uc_stack.ss_sp = garstack;
		uctx_garbage->uc_stack.ss_size = STACK_SIZE;
		uctx_garbage->uc_link = 0;
		uctx_garbage->uc_stack.ss_flags=0;	
		makecontext(uctx_garbage,(void *)&garbage,0);
		
		//get context of main
		mainThread = (tcb *) malloc(sizeof(tcb));
		uctx_main = (ucontext_t *)malloc(sizeof(ucontext_t));
		if(getcontext(uctx_main)==-1){
		perror("getcontext failed");
		exit(0);
	}
		uctx_main->uc_link = 0;
		uctx_main->uc_stack.ss_flags=0;
		
		mainThread->tid = 0;
		mainThread->state = running;
		mainThread->cxt = uctx_main;
		mainThread->isMain = 1;
		
		//Put main into our scheduler. Treat it like any other thread
		enqueue(0,mainThread);
		
		
		//set up context of my_handler. Do we need this?
	
		uctx_handler = (ucontext_t *)malloc(sizeof(ucontext_t));
		if(getcontext(uctx_handler)==-1){
		perror("getcontext failed");
		exit(0);
	}
		void * stack = malloc(STACK_SIZE);
		uctx_handler->uc_stack.ss_sp = stack;
		uctx_handler->uc_stack.ss_size = STACK_SIZE;
		uctx_handler->uc_link = 0;
		uctx_handler->uc_stack.ss_flags=0;	
		makecontext(uctx_handler,(void *)&my_handler,1, SIGALRM);
		
		
		//setting itimer
		
		interval.tv_sec = 0;
		interval.tv_usec = slice;
		
		timer.it_interval = interval;
		timer.it_value = interval;
		//?? this timer may or may not act within pthread create time
		setitimer(ITIMER_REAL,&timer,NULL);
		
		
		isInit = 1;
	}else{
		return;
	}
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

void wrapper(void *(*function)(void*), void* arg){
	void* retval = function(arg);
	my_pthread_exit(retval);
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	initialize();
	
	//Start setting up the control block for the thread
	tcb *control_block = (tcb*)malloc(sizeof(tcb));
	control_block->tid = tid;
	tid++;
	control_block->state = embryo;
	
	//Make a context for the thread
	ucontext_t * c = (ucontext_t *)malloc(sizeof(ucontext_t));
	
	if(getcontext(c)==-1){
		perror("getcontext failed");
		exit(0);
	}
	
	
	void * stack = malloc(STACK_SIZE);
	c->uc_stack.ss_sp = stack;
	c->uc_stack.ss_size = STACK_SIZE;
	c->uc_link = 0;
	c->uc_stack.ss_flags=0;
	control_block->stack = stack;
	control_block->next = NULL;
	thread[0]=control_block->tid;
	
	
	//wrap function user passes so that it call pthread_exit
	
	
	
	//makecontext(c,(void*)function,1,arg);
	makecontext(c,(void *)&wrapper,2,function,arg);
	control_block->cxt = c;
	control_block->isMain = 0;
	

	//must add timesplice and priority later
	//count for error if insufficient stack space etc.
	
	
		
	//Put thread into our scheduler
	enqueue(0, control_block);	
		
	//Yield so scheduler can do stuff
	my_pthread_yield();
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
	
	running_thread->state = terminate;
	running_thread->return_val = (void *) value_ptr;
	
	my_pthread_yield();
};

/* wait for thread termination */

int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	//Upon call, search done queue
	//If its there, check value_ptr and set accordingly and return
	//If not there, move to waiting queue,
	if(done_queue->size==0){
		//not there, so wait
		//running_thread
	}	
	else  {
		//search
		
		
		
	}
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


