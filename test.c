#include <stdio.h>
#include <stdlib.h>
#include "my_pthread.c"
#include "my_pthread_t.h"

my_pthread_t * threadArray;
//comment
void * dummy1(){
int j=0;

for(j=0;j<12500;j++){
    
    printf("%d\n",j);
  
	}
    int a = 1;
    int * b = (int *)malloc(sizeof(int));
    my_pthread_exit((void *)b);
   // return 0;
}

void * dummy2(){
int j=0;
//sleep(1);
for(j=12500;j<12501;j++){
    printf("%d\n",j);
	}
    
    int c = 2;
    int * d = (int *)malloc(sizeof(int));
    my_pthread_exit((void *)d);
  //  return 0;
}

int main(int argc, char** arg){

	//getcontext(&uctx_main);
    threadArray = (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
    
    
	
    my_pthread_create(&threadArray[0],NULL,dummy1,NULL);
    printf("back in main\n");
    printf("pick up dummy2");
    my_pthread_create(&threadArray[1],NULL,dummy2,NULL);
    printf("back in main\n");
    
    int i = 1000000000000000000;
while(i>0){
    i--;
}
     
      
	return 0;
}

