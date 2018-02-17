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

    //my_pthread_exit(0);
   // return 0;
}

void * dummy2(){
int j=0;
//sleep(1);
for(j=12500;j<19000;j++){
    printf("%d\n",j);
	}
   //my_pthread_exit(0);
  //  return 0;
}

int main(int argc, char** arg){

	//getcontext(&uctx_main);
    threadArray = (my_pthread_t *)malloc(sizeof(my_pthread_t)*10);
    int i;
	
    my_pthread_create(&threadArray[0],NULL,dummy1,NULL);
    printf("back in main\n");
    printf("pick up dummy2");
    my_pthread_create(&threadArray[1],NULL,dummy2,NULL);
    printf("back in main\n");
    

 /* i = 10;
while(i >0){
   i--;
  //printf("main\n");
    
}*/
     
	return 0;
}

