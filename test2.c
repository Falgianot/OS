#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include "my_pthread_t.h"
#include "my_pthread.c"




my_pthread_t* threadArray;
my_pthread_t* threadArray1;
//comment
void *dummy2();
void* dummy3();
int total = 0;
my_pthread_mutex_t m;
void * dummy1(){
    int i = 0;
    int* temp;
      temp = (int*)myallocate(5,"test2.c",76,1);
       int* temp2 = (int*)myallocate(10,"test2.c",76,1);
      i++;
    
    *temp = 8;
    printf("%d\n",*temp);
 printf("Addr:%x\n",temp);
    my_pthread_exit(0);
    
    
    
    
    
/*int j=0;
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
    
    
    my_pthread_exit(0);*/

}

void * dummy2(){
    
    
    int i = 0;
    int * temp = NULL;
    int * temp2 = NULL;
    int * temp3 = NULL;
  //  while(i<10){
      temp = (int*)myallocate(4000,"test2.c",76,1);
      temp2 = (int*)myallocate(4097,"test2.c",76,1);
      //temp3 = (int *)myallocate(8,"test2.c",76,1);
      mydeallocate(temp2,"test2.c",76,1);
       mydeallocate(temp,"test2.c",76,1);
     //i++;
   // }
    
   
     
   // mydeallocate(temp2,"test2.c",76,30);
     
     
   //  int* temp3 = (int*)myallocate(4500,"test2.c",76,30);
     *temp = 8;
   // *temp2 = 10;
     
     /*
     int* temp2= (int*)myallocate(10000,"test2.c",76,30);
    *temp = 8;
    *temp2 = 10;
    int i= 0 ;
    while(i < 5){
      int* temp3 = (int*)myallocate(4500,"test2.c",76,30);
      i++;
    }
   while(1){
      
    }*/
     
    // mydeallocate(temp2,"test2.c",76,30);
    printf("%d\n",*temp);
 printf("Addr:%x\n",temp);
 //printf("%d\n",*temp2);
 //printf("Addr:%x\n",temp2);
 

    my_pthread_exit(0);
    
    
    
    
    
    
/*int j=0;
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
    my_pthread_exit(d);*/
}

void * dummy3(){
    int* temp;
    int* temp2;
   
   int  i = 0;
         temp = (int*)myallocate(3000,"test2.c",76,1);
         temp2 = (int*)myallocate(9724,"test.c",767,1);
       int* temp3 = (int*)myallocate(2,"test.c",767,1);
   
   
 //  mydeallocate(temp2,__FILE__,__LINE__,1);
  //int* temp4 = (int*)myallocate(1031,"test.c",767,1);
     *temp = 5;
    printf("%d\n",*temp);
 printf("Addr:%x\n",temp);
    my_pthread_exit(0);
}

int main(int argc, char** arg){

	//getcontext(&uctx_main);
  //threadArray1= (my_pthread_t *)myallocate(sizeof(my_pthread_t)*20,"test2.c", 76,1);
  threadArray1= (my_pthread_t *)malloc(sizeof(my_pthread_t)*20);
  
  //threadArray = (my_pthread_t *)malloc(sizeof(my_pthread_t)*55);
    int i = 1;
	
    
   // threadArray[0] = (my_pthread_t *)myallocate(sizeof(my_pthread_t),"test2.c", 76,1);
  // my_pthread_create(&threadArray[i],NULL,dummy3,NULL);
    
       
       

         //my_pthread_create(&threadArray1[0],NULL,dummy1,NULL);

        // my_pthread_create(&threadArray1[1],NULL,dummy3,NULL);
         i++;
        
            my_pthread_create(&threadArray1[2],NULL,dummy2,NULL);
          //  i++;
        
    // }
    /*printf("back in main\n");
    printf("pick up dummy2");
    
    printf("back in main\n");
    */
   // int *x = (int *)malloc(sizeof(int));
     int *y = (int *)malloc(sizeof(int));
     
    

   // my_pthread_join(threadArray[0],(void*)&y);
    i = 1;
	
  // my_pthread_create(&threadArray[i],NULL,dummy3,NULL);
    
        // my_pthread_create(&threadArray[0],NULL,dummy2,NULL);
       

        //  my_pthread_create(&threadArray1[0],NULL,dummy1,NULL);
 //my_pthread_join(threadArray1[0],(void*)&y);
 //my_pthread_join(threadArray1[1],(void*)&y);
 my_pthread_join(threadArray1[2],(void*)&y);
      /*  while(i < 20){
       my_pthread_join(threadArray[i],(void*)&y);
         i++;
        }*/

     printf("the tid is: %d and the return val is: %d\n",threadArray1[1],*y);
     

    
 
     
      
	return 0;
}

