#include <stdio.h>
#include <stdlib.h>
#include "my_pthread.c"
#include "my_pthread_t.h"

my_pthread_t * threadArray;
my_pthread_mutex_t a;
my_pthread_mutex_t b;
void * dummy(){
int j=0;
for(j=0;j<10;j++){
    printf("%d\n",j);
	}
return 0;
}

int main(int argc, char** arg){

	//getcontext(&uctx_main);
    threadArray = (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
    int i;
   for(i = 0; i< 2; i++){
    //my_pthread_create(&threadArray[0],NULL,dummy,NULL);
	my_pthread_mutex_init(&a,NULL);
		my_pthread_mutex_init(&b,NULL);
   }
my_pthread_mutex_lock(&a);
    /*int j = 0;
    for(j = 0; j<5; j++){
        printf("tid:%d\n",threadArray[j]);
    }*/
	return 0;
}

