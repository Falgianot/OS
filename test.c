#include <stdio.h>
#include <stdlib.h>
#include "my_pthread.c"
#include "my_pthread_t.h"

my_pthread_t * threadArray;
//comment
void *dummy2();
void* dummy3();
int total = 0;
my_pthread_mutex_t m;
void * dummy1(){
int j=0;
my_pthread_t *threadArray1= (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
for(j=0;j<12500;j++){
    
  
	}
    int a = 3;
    int * b = (int *)malloc(sizeof(int));
    b = &a;
    int i=0;
    //while(i<5){
      my_pthread_create(&threadArray1[i],NULL,dummy3,NULL);
        
      //  i++;
   // }
     i=0;
    // while(i<5){
      my_pthread_join(threadArray1[i],(void*)&b);
       printf("I am in dummy 1:the tid is: %d and the return val is: %d\n",threadArray1[i], *b);
     //  i++;
    //}
    
    
    my_pthread_exit(0);

}

void * dummy2(){
int j=0;
my_pthread_t *threadArray2 = (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
//sleep(1);
for(j=12500;j<12501;j++){
	}
    
    int c = 2;
    int * d = (int *)malloc(sizeof(int));
    d = &c;
    int i=0;
    while(i<5){
      my_pthread_create(&threadArray2[i],NULL,dummy1,NULL);
        
        i++;
    }
     i=0;
     while(i<5){
      my_pthread_join(threadArray2[i],NULL);
       printf("i am in dummy 2:the tid is: %d and the return val is: %d\n",threadArray2[i], 0);
       i++;
    }
    my_pthread_exit(d);
}

void * dummy3(){
    int i=0;
//while(i<500){
   // printf("hi\n");
    i++;
    int sum =0;
//}
    
    //sum=500;
    // my_pthread_mutex_lock(&m);
    while(i <= 250000000){
        sum+=1;
       // printf("%d\n",sum);
        i++;
    }
   //my_pthread_mutex_unlock(&m);
    total += sum;
    
    printf("%d\n",total);
    //my_pthread_mutex_lock(&m);
    my_pthread_exit(0);
    //my_pthread_exit((void*)2);
  //  return 0;
}

int main(int argc, char** arg){

	//getcontext(&uctx_main);
    threadArray = (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
    
    my_pthread_mutex_init(&m,NULL);
	int i = 0;
   //  while(i <5){
          //my_pthread_create(&threadArray[i],NULL,dummy1,NULL);
          my_pthread_create(&threadArray[0],NULL,dummy3,NULL);
           my_pthread_create(&threadArray[1],NULL,dummy3,NULL);
            my_pthread_create(&threadArray[2],NULL,dummy3,NULL);
       
       
           
          //  i++;
        
    // }
    /*printf("back in main\n");
    printf("pick up dummy2");
    
    printf("back in main\n");
    */
   // int *x = (int *)malloc(sizeof(int));
     int *y = (int *)malloc(sizeof(int));
     i = 0;
    /* while(i <5){
        my_pthread_join(threadArray[i],(void*)&x);
       // my_pthread_join(threadArray[i+5],(void*)&y);
      printf("the tid is: %d and the return val is: %d\n",threadArray[i],*x);
     // printf("the tid is: %d and the return val is: %d\n",threadArray[i+5],*y);
    i++;
    }*/
    //my_pthread_join(threadArray[1],(void*)&y);
    i = 0;
    // while(i <5){
     
     my_pthread_join(threadArray[0],(void*)&y);
      my_pthread_join(threadArray[1],(void*)&y);
      
       my_pthread_join(threadArray[2],(void*)&y);
     
     printf("the tid is: %d and the return val is: %d\n",threadArray[0],*y);
     printf("the tid is: %d and the return val is: %d\n",threadArray[1],*y);
     
     printf("the tid is: %d and the return val is: %d\n",threadArray[2],*y);
           my_pthread_mutex_destroy(&m);

     printf("%d",total);
  // i++;
    // }
    //printf("%d" , *y);
/*while(1){
   printf("hell\n");
}*/
     
      
	return 0;
}

