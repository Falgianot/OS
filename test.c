#include <stdio.h>
#include <stdlib.h>
#include "my_pthread.c"
#include "my_pthread_t.h"

my_pthread_t * threadArray;
//comment
void * dummy1(){
int j=0;

for(j=0;j<12500;j++){
    
   // printf("%d\n",j);
  
	}
    int a = 3;
    int * b = (int *)malloc(sizeof(int));
    b = &a;
    my_pthread_exit(b);
   // return 0;
}

void * dummy2(){
int j=0;
//sleep(1);
for(j=12500;j<12501;j++){
    //printf("%d\n",j);
	}
    
    int c = 2;
    int * d = (int *)malloc(sizeof(int));
    d = &c;
    my_pthread_exit(d);
    //my_pthread_exit((void*)2);
  //  return 0;
}

int main(int argc, char** arg){

	//getcontext(&uctx_main);
    threadArray = (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
    
    
	int i = 0;
     while(i <5){
          my_pthread_create(&threadArray[i],NULL,dummy1,NULL);
          my_pthread_create(&threadArray[i+5],NULL,dummy2,NULL);
            i++;
     }
    /*printf("back in main\n");
    printf("pick up dummy2");
    
    printf("back in main\n");
    */
    int *x = (int *)malloc(sizeof(int));
     int *y = (int *)malloc(sizeof(int));
     i = 0;
     while(i <5){
        my_pthread_join(threadArray[i],(void*)&x);
        my_pthread_join(threadArray[i+5],(void*)&y);
      printf("the tid is: %d and the return val is: %d\n",threadArray[i],*x);
      printf("the tid is: %d and the return val is: %d\n",threadArray[i+5],*y);
    i++;
    }
    //my_pthread_join(threadArray[1],(void*)&y);
    
    //printf("%d" , *y);
/*while(1){
   printf("hell\n");
}*/
     
      
	return 0;
}

