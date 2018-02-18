// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server: 
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define pthread_create(X,Y,Z,A) my_pthread_create(X,Y,Z,A)
#define pthread_yield() my_pthread_yield()
#define pthread_join(X,Y) my_pthread_join(X,Y)
#define pthread_exit(X) my_pthread_exit(X)
#define pthread_mutex_init(X,Y) my_pthread_mutex_init(X,Y)
#define pthread_mutex_lock(X)   my_pthread_mutex_lock(X)
#define pthread_mutex_unlock(X) my_pthread_mutex_unlock(X)
#define pthread_mutex_destroy(X)   my_pthread_mutex_destroy(X)

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>

typedef uint my_pthread_t;
#define pthread_t my_pthread_t
typedef struct threadControlBlock {
    int tid;
    int waitid;
    enum states{ready, running, wait, terminate,embryo}state;
    void* return_val;
    ucontext_t * cxt;
    int isMain;
    void* stack;
    struct itimerval timesplice;
    int priority;
    struct threadControlBlock * next;
	
} tcb; 

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
    int locked;
    int isInit;
    struct threadControlBlock* wait;
} my_pthread_mutex_t;

#define pthread_t_mutex my_pthread_t_mutex_
/* define your data structures here: */

// Feel free to add your own auxiliary data structures


/* Function Declarations: */

void my_handler(int signum);

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif


