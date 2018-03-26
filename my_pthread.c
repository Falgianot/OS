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
#include <string.h>
#include <fcntl.h>


//defining global vars
//4096
#define STACK_SIZE 1024*8
#define MAX_THREADS 64
#define slice 20000
#define priorities 5
#define mutexSize 5
 ucontext_t * uctx_main;
 ucontext_t * uctx_handler;
 ucontext_t * uctx_garbage;
int tid = 1;
int isInit = 0;
int queueInit =0;
int waitBool =0;
tcb * running_thread;
struct itimerval timer;
struct timeval interval;
int mainDone = 0;
int firstSwap = 0;
int timeCounter =-1;
int pick [5]= {16, 8 ,6 ,4 ,2};
int maintenancePick[5] = {0, 1, 3, 7, 10};
int totalSwap = 0;
int test = 0;
int pagesLeft = 2048;
int specialStop = 0;
int OSLandTotal = 4194304;
int swapfd = -1;
my_pthread_mutex_t* current_mutex;

 static void* pages[2048];//8MB block of memory to malloc from

typedef struct mementry{
    int size;//our entry can have max 2^12 size
    struct mementry * next; //useful for coalascing
    struct mementry * prev;//useful for coalescing
    int free; //flag to tell if entry is free or not
}mementry;

typedef struct memBook{
	tcb * threadOwner;
	//int pageNum;
	int swapLoc;
}memBook;

typedef struct freeNode{
		
		void * freePage;
		struct freeNode * next;
	
	
} freeNode;

freeNode * head;
struct memBook membook[2044];




void swap(void* destination,void* source, pageEntry * sourcePage, int dest){
		void * temp = pages[2043];
		
		int i = 0;
		pageEntry * destPage = NULL;
		
		//Loading in pages case
		if(sourcePage!=NULL){
			while(i<128){
				//destination page is not occupied by a thread. Possibly because of free
				if( membook[sourcePage->initSpot].threadOwner==NULL){
					membook[sourcePage->initSpot].threadOwner = running_thread;
					membook[sourcePage->physicalMemLoc].threadOwner = NULL;
					
					//Update source pages location. Moving it to initial spot. And copy data
					sourcePage->physicalMemLoc = sourcePage->initSpot;
					
					memcpy(temp, destination,sysconf( _SC_PAGE_SIZE));
					memcpy(destination,source,sysconf( _SC_PAGE_SIZE));
					memcpy(source,temp, sysconf( _SC_PAGE_SIZE));
					return;
				}
				
				destPage = membook[sourcePage->initSpot].threadOwner->dir->table[i];
				if(destPage->physicalMemLoc == sourcePage->initSpot){
					break;
				}
				i++;
			}
			
			//update page table locations and membooks
			printf("sourcePage swapping to %d\n",destPage->physicalMemLoc);
			printf("destPage swapping to %d\n",sourcePage->physicalMemLoc);

			destPage->physicalMemLoc = sourcePage->physicalMemLoc;
			sourcePage->physicalMemLoc = sourcePage->initSpot;
			tcb * threadTemp = membook[sourcePage->physicalMemLoc].threadOwner;
			membook[sourcePage->physicalMemLoc].threadOwner = membook[destPage->physicalMemLoc].threadOwner;
			membook[destPage->physicalMemLoc].threadOwner = threadTemp;
			
			
			//Initial swaps. Such as mallocing 1 page a bunch of times
		}else{
		//Find the correct physcial location of source
			int j = 1024;
			while(j<2044){
				if(pages[j]==source){
					break;
				}
				j++;
			}
			
			
			if(membook[dest].threadOwner== 0){
				memcpy(temp, destination,sysconf( _SC_PAGE_SIZE));
				
				memcpy(destination,source,sysconf( _SC_PAGE_SIZE));
				memcpy(source,temp, sysconf( _SC_PAGE_SIZE));
			}
				
		membook[j].threadOwner = membook[dest].threadOwner;
		int k = 0;

		if(source == destination){
			return;
			
		}
		
		if(membook[dest].threadOwner!= 0){
		//Find the page entry of destination so we can update its physicalMemLoc
		while(k<128){
				destPage = membook[dest].threadOwner->dir->table[k];
				if(destPage!= NULL && destPage->physicalMemLoc == dest){
					destPage->physicalMemLoc = j;
					break;
				}
				k++;
			}
		
		}
		}
		
		
		/*int p = 0;
		while (p < 4096){
			memcpy(temp+p, destination+p,1);
	p++;
		}
		
		p = 0;
		while (p < 4096){
			memcpy(destination+p, source+p,1);
	p++;
		}
		p = 0;
		while (p < 4096){
			memcpy(source+p, temp+p,1);
	p++;
		}
		*/
		//Swap the actual data my mem copying
		//bzero(temp, sysconf( _SC_PAGE_SIZE));
		memcpy(temp, destination,sysconf( _SC_PAGE_SIZE));
		//bzero(destination,sysconf( _SC_PAGE_SIZE));
		//memset(destination,'0',sysconf( _SC_PAGE_SIZE));
		memcpy(destination,source,sysconf( _SC_PAGE_SIZE));
		//bzero(source,sysconf( _SC_PAGE_SIZE));
		memcpy(source,temp, sysconf( _SC_PAGE_SIZE));
		//bzero(temp, sysconf( _SC_PAGE_SIZE));
}


//Before context switch load pages of that thread in the right spot
void loadPages(){
	printf("Loading tid:%i\n",running_thread->tid);
	pageTable * currTable = running_thread->dir;
	//if current table is empty nothing to swap so return
	if(currTable->isEmpty == 0){
		return;
	}
	int i = 0;
	while(i<128){
		pageEntry * pe = currTable->table[i];
		if(pe!=NULL){
			//put page in correct spot in memory swap from physical loc to initial spot. Only if they are different pages
			if(pages[pe->initSpot]!=pages[pe->physicalMemLoc]){
			swap(pages[pe->initSpot],pages[pe->physicalMemLoc],pe,0);
			}
		}
		i++;
	}
	
	
}
/*finds curr_page# 0-2047 
*/
int getpage(void* addr){
	int curr_page;
	curr_page = addr - pages[0];
	curr_page = curr_page / 4096;
	return curr_page;
}





void mydeallocate(void * x, char file[], int line, int req){
		waitBool=1;
	
	if(x==NULL){
		waitBool=0;
		return;
	}

	

//search for the metadata that user is trying to free
	void * a = pages[1024];
	mementry * meta = (mementry *)a;
	int found = 0;
	while(meta!=NULL){
		mementry * m = (mementry*)x;
		if(meta==m-1){
			found = 1;
			break;
		}
		meta = meta->next;
	}
	if(found == 1){
		printf("found it\n");
	}else{
		printf("Addr DNE\n");
		return;
	}
	//update membook,pagetable, set mementry at beginning of fresh page, insert into free list
	//meta->free = 1;
	//after that check if we can coalesce
	int start_page = getpage(x);//get frame of address trying to free, is also init spot
	mementry* to_free = x - sizeof(mementry);//get the metadata
	to_free->free=1;
	
	int i = 0;
	pageEntry * pe = NULL;
	//get the page entry where the address is located. Helps us find see if there are dependencies
	while(i<128){
		pe = running_thread->dir->table[i];
		if(pe!=NULL&&pe->initSpot==start_page){
			break;
		}
		i++;
	}
	pe->largestAvailSize = pe->largestAvailSize+meta->size;//this gets updated again later on in code depending if the malloc spills over pages
	pageEntry * temp = pe;

	
	//assists in finding free space
	void* alloc_end;
	void *prev_alloc_end;
	mementry* nextNode = to_free->next;
	mementry* prevNode = to_free->prev;
	int end_page;
		
	
	alloc_end = (void*)to_free + sizeof(mementry) + to_free->size;//could be 0
	if(prevNode == NULL)
		prev_alloc_end = to_free;//nothing to add before
	else
		prev_alloc_end = (void*)prevNode + sizeof(mementry) + prevNode->size;//could be 0
	
	pageEntry* endPageEntry = NULL;
	i=0;

	int free_space_after;
	end_page = getpage(alloc_end);
	//if sliver is in page before the next metanode's page
	if((void*)to_free->next >= pages[end_page+1]){
		free_space_after = (void*)(to_free->next) - alloc_end;
		while(i<128){
			endPageEntry = running_thread->dir->table[i];
			if(endPageEntry!=NULL&&endPageEntry->initSpot==end_page){
				break;
			}
			i++;
		}
		endPageEntry->largestAvailSize += free_space_after; 
		end_page++;
	}
	//sliver at the very end of all pages
	else if(to_free->next==NULL){
		free_space_after = (void*)pages[end_page+1] - alloc_end;
	}
	//sliver before metanode in metanodes page
	else{
		free_space_after = (void*)(to_free->next) - alloc_end; 
	}


	int pagesBetweenF = end_page - start_page-1;
	i=0;
	while(i<128){
		endPageEntry = running_thread->dir->table[i];
		if(endPageEntry!=NULL&&endPageEntry->initSpot==end_page){
			break;
		}
		i++;
	}
	endPageEntry->largestAvailSize += free_space_after;


	
	//insert into free list, update threads page table, membook, and update metasize and make new metadata
	i = 0;
	//should have two pages in the end.
	while(i<pagesBetweenF){
		temp = temp->next;//pe to delete from page table
		//Create meta for fresh page
		mementry* m = (mementry*) pages[start_page+i+1];
		m->next=NULL;
		m->prev= NULL;
		m->size=4064;
		m->free=1;
		
		//Insert back into free list
		freeNode * toAdd = (freeNode *)malloc(sizeof(freeNode));
		toAdd->freePage = pages[temp->initSpot];
		toAdd->next = NULL;
		freeNode * search = head;
		toAdd->next = head;
		head=toAdd;
		/*
		while(search->next!=NULL){
			search=search->next;
		}
		search->next=toAdd;
		*/
		//update metasize
		//to_free->size -= 4096;
		pe->largestAvailSize = pe->largestAvailSize-4096;
		//to_free->size +=4096;

		//update membook
		membook[temp->initSpot].threadOwner = NULL;
		
		//update page table
		int j=0;
		while(j<128){
			pageEntry * temp2 = running_thread->dir->table[j];
			if(temp==temp2){
				if(temp2->next!=NULL){
					pe->next=temp2->next;
					temp2 = temp2->next;
					temp2->prev = pe;
				}
				//freeing page
				temp = pe;
				running_thread->dir->table[j] = NULL;
			}
			j++;
		}
		i++;
	}
	

	int spill=0;
	int spillover=0;
	if(start_page != end_page){
		spill=1;//boolean
		spillover = alloc_end- pages[end_page];
	}
	to_free->free=1;
	//After above loop. Two pages are next to each other. Add the spillover and sliver to largestAvailSize of last page.
	//endPageEntry->largestAvailSize += free_space_after;
	if(spill==1){
		pe->largestAvailSize -= spillover;
		//endPageEntry->largestAvailSize += free_space_after;//sliver
		endPageEntry->largestAvailSize += spillover;
		if(nextNode!=NULL){
			//more coalescing
			if(nextNode->free ==1){
				to_free->size += free_space_after;//sliver
				to_free->size += sizeof(mementry);//metadata
				to_free->size += nextNode->size;//next size
				
				//No freeing page
				if(nextNode->next!=NULL){
				//	endPageEntry->largestAvailSize += sizeof(mementry);
					nextNode->next->prev = to_free;
					to_free->next = nextNode->next;
				}
				//page needs to be freed
				else{
					//have to insert last page back into free list. So update tofree to get rid of remainder.
					int numInserted = pagesBetweenF +1;
					
					to_free->size = to_free->size-(numInserted*4096);
					to_free->next = NULL;
					
					temp = temp->next;//pe to delete from page table
					//Create meta for fresh page
					mementry* m = (mementry*) pages[end_page];
					m->next=NULL;
					m->prev= NULL;
					m->size=4064;
					m->free=1;
					
					//Insert back into free list
					freeNode * toAdd = (freeNode *)malloc(sizeof(freeNode));
					toAdd->freePage = pages[temp->initSpot];
					toAdd->next = NULL;
					freeNode * search = head;
					while(search->next!=NULL){
						search=search->next;
					}
					search->next=toAdd;
					
					//update metasize
					//to_free->size -= 4096;
					
					//update membook
					membook[temp->initSpot].threadOwner = NULL;
					
					//update page table
					int j=0;
					while(j<128){
						pageEntry * temp2 = running_thread->dir->table[j];
						if(temp==temp2){
							if(temp2->next!=NULL){
								pe->next=temp2->next;
								temp2 = temp2->next;
								temp2->prev = pe;
							}
							//freeing page
							running_thread->dir->table[j] = NULL;
							break;
						}
						j++;
					}
				}
			}else{
				//next node exists and not free
				to_free->size += free_space_after;
				//to_free->size += pages[start_page+1]-(void*)to_free-sizeof(mementry);
			}
		}
		//nextNode points to NULL which is end of cross over page.
		//Page needs to be freed
		else{
			int numInserted = pagesBetweenF +1;
			to_free->size = to_free->size-(numInserted*4096);
			to_free->next = NULL;
			to_free->size += free_space_after;
			
			temp = temp->next;//pe to delete from page table
			//Create meta for fresh page
			mementry* m = (mementry*) pages[end_page];
			m->next=NULL;
			m->prev= NULL;
			m->size=4064;
			m->free=1;
			
			//Insert back into free list
			freeNode * toAdd = (freeNode *)malloc(sizeof(freeNode));
			toAdd->freePage = pages[temp->initSpot];
			toAdd->next = NULL;
			freeNode * search = head;
			while(search->next!=NULL){
				search=search->next;
			}
			search->next=toAdd;
			
			//update metasize
			//to_free->size -= 4096;
			
			//update membook
			membook[temp->initSpot].threadOwner = NULL;
			
			//update page table
			int j=0;
			while(j<128){
				pageEntry * temp2 = running_thread->dir->table[j];
				if(temp==temp2){
					if(temp2->next!=NULL){
						pe->next=temp2->next;
						temp2 = temp2->next;
						temp2->prev = pe;
					}
					//freeing page
					running_thread->dir->table[j] = NULL;
					break;
				}
				j++;
			}
			//to_free->size = pages[start_page+1]-(void*)to_free-sizeof(mementry);
			//to_free->next = NULL;
		}
	}
	//no spill
	
	else{
		to_free->size += free_space_after;
		//pe->largestAvailSize += free_space_after;
		if(nextNode!=NULL){
			if(nextNode->free == 1){
				pe->largestAvailSize += sizeof(mementry);
				to_free->size += nextNode->size;
				to_free->size += sizeof(mementry);
				to_free->next = nextNode->next;
				if(nextNode->next!=NULL){
					nextNode = nextNode->next;
					nextNode->prev = to_free;
				}
				else{
					nextNode = NULL;
				}
			}
			////MIGHT WANT TO PUT BACK IN PAGE ENTRY IF LARGESTAVAILSIZE IF 4064
		}
	}


	if(prevNode==NULL){
		if(to_free->next==NULL){
			//free page pe. 
		}
		else{
			//do nothing
		}
	}
	//there is a node behind 
	else{
		int back_page = getpage(prevNode);
		int end_back_page = getpage(prev_alloc_end);
		


		pageEntry* backPageEntry = NULL;
		i=0;
		while(i<128){
			backPageEntry = running_thread->dir->table[i];
			if(backPageEntry!=NULL&&backPageEntry->initSpot==back_page){
				break;
			}
			i++;
		}


		//Start previous node checking case
		i = 0;
		temp = backPageEntry;
		int pagesBetweenB=0;
		while(i<pagesBetweenB){
			temp = temp->next;//pe to delete from page table
			//Create meta for fresh page
			mementry* m = (mementry*) pages[back_page+i+1];
			m->next=NULL;
			m->prev= NULL;
			m->size=4064;
			m->free=1;
			
			//Insert back into free list
			freeNode * toAdd = (freeNode *)malloc(sizeof(freeNode));
			toAdd->freePage = pages[temp->initSpot];
			toAdd->next = NULL;
			freeNode * search = head;
			while(search->next!=NULL){
				search=search->next;
			}
			search->next=toAdd;
			
			//update metasize
			//to_free->size -= 4096;
			//backPageEntry->largestAvailSize = backPageEntry->largestAvailSize-4096;
			//to_free->size +=4096;

			//update membook
			membook[temp->initSpot].threadOwner = NULL;
			
			//update page table
			int j=0;
			while(j<128){
				pageEntry * temp2 = running_thread->dir->table[j];
				if(temp==temp2){
					if(temp2->next!=NULL){
						backPageEntry->next=temp2->next;
						temp2 = temp2->next;
						temp2->prev = backPageEntry;
					}
					//freeing page
					temp = backPageEntry;
					running_thread->dir->table[j] = NULL;
				}
				j++;
			}
			i++;
		}


		int spillback=0;
		int spilloverback=0;
		if(start_page != back_page){
			spillback=1;
			spilloverback = prev_alloc_end - pages[back_page+1];
		}


		/*
		if(spill==1){
		//note: to_free can never be memmoved so that metadata exists between pages
		//more checks will be needed than nonacross page to satisfy request
			
		}
		//no spillback, all in same page
		else{
			//not start of page
			if(prevNode!=NULL){
				to_free->size += free_space_before;
				pe->largestAvailSize += free_space_before;
				//previous is free and needs merging
				if(prevNode->free==1){
					prevNode->size += to_free->size;
					prevNode->size += sizeof(mementry);
					prevNode->next = to_free->next;
					pe->largestAvailSize += sizeof(mementry);
					if(nextNode!=NULL)
						nextNode->prev = prevNode;
					//next is null and current or backpage could be freed after this
					else{
						//free current page
						if(pe->largestAvailSize == 4064){

						}

						//free page prev
						if(backPageEntry->largestAvailSize == 4064){

							int numInserted = pagesBetweenB +1;
							to_free->size = to_free->size-(numInserted*4096);
							to_free->next = NULL;
							
							
							temp = temp->next;//pe to delete from page table
							//Create meta for fresh page
							mementry* m = (mementry*) pages[end_page];
							m->next=NULL;
							m->prev= NULL;
							m->size=4064;
							m->free=1;
							
							//Insert back into free list
							freeNode * toAdd = (freeNode *)malloc(sizeof(freeNode));
							toAdd->freePage = pages[temp->initSpot];
							toAdd->next = NULL;
							freeNode * search = head;
							while(search->next!=NULL){
								search=search->next;
							}
							search->next=toAdd;
							
							//update metasize
							//to_free->size -= 4096;
							
							//update membook
							membook[temp->initSpot].threadOwner = NULL;
							
							//update page table
							int j=0;
							while(j<128){
								pageEntry * temp2 = running_thread->dir->table[j];
								if(temp==temp2){
									if(temp2->next!=NULL){
										pe->next=temp2->next;
										temp2 = temp2->next;
										temp2->prev = pe;
									}
									//freeing page
									running_thread->dir->table[j] = NULL;
									break;
								}
								j++;
							}
						}
					}

				}
				//previous not free and metadata needs to be moved
				else{
					to_free = memmove( (void*)to_free - free_space_before, to_free, sizeof(mementry) );
					prevNode->next = to_free;
					if(nextNode!=NULL)
						nextNode->prev = to_free;
				}
			}
			//start of allocated block
			else{
				//need to free page if availsize is 4064
				//if(pe->largestAvailSize == 4064)

			}
		}*/
	}
	
	
	
	
	/*
	int start_page = getpage(x);
	mementry* to_free = x - sizeof(mementry);
	to_free->free=1;

	//assists in finding free space
	void* alloc_end;
	void *prev_alloc_end;
	alloc_end = (void*)to_free + sizeof(mementry) + to_free->size;
	mementry* back = to_free -> prev;
	if(back == NULL)
		prev_alloc_end = to_free;
	else
		prev_alloc_end = (void*)back + sizeof(mementry) + back->size;

	/*********************************************************************
	will need to check this for a single malloc that go beyond pages and not in shalloc range.
	Would need to recreate free node in pages that are reclaimed when a thread claims more than one page.
	
	int free_space_before, free_space_after;
	if(to_free->next==NULL){
		int curr_page = getpage(alloc_end);
		free_space_after = (void*)pages[curr_page+1] - alloc_end;
	}
	else{
		free_space_after = (void*)(to_free->next) - alloc_end; 
	}
	free_space_before = (void*)to_free - prev_alloc_end;

    //void* start = pages[2044];
    //void* end = pages[2048];
    //deallocation in shared pages
    if ( start_page>=2044 && start_page<2048 ){
    	mementry* nextNode = to_free->next;
    	mementry* prevNode = to_free->prev;
		
		to_free->size += free_space_after;
    	if(nextNode!=NULL){
    		if(nextNode->free ==1){
				to_free->size += nextNode->size;
				to_free->next = nextNode->next;
				if(nextNode->next!=NULL){
					nextNode = nextNode->next;
					nextNode->prev = to_free;
				}
				else
					nextNode = NULL;
			}
    	}

    	if(prevNode!=NULL){
    		to_free->size += free_space_before;
    		if(prevNode->free==1){
				prevNode->size += to_free->size;
				prevNode->next = to_free->next;
				if(nextNode!=NULL)
					nextNode->prev = prevNode;
			}
			else{
				to_free = memcpy( (void*)to_free - free_space_before, to_free, sizeof(mementry) );
				prevNode->next = to_free;
				if(nextNode!=NULL)
					nextNode->prev = to_free;
			}
			
    	}
    	*/
		
    
    
    
    
    waitBool=0;
//}

}



/*should test lel?*/
void* shalloc(size_t size){
	waitBool=1;
	void* toReturn;
	mementry* startShare = pages[2044];
	mementry* prev = NULL;

	//finds if available node
	while( startShare!=NULL){
		if(startShare->free==1 && startShare->size > size )
			break;
		prev = startShare;
		startShare= startShare->next;
	}

	if(startShare==NULL) //no available node
		toReturn = NULL;
	else{
		//check if space to create new metanode
		toReturn = (void*)startShare + sizeof(mementry);
		startShare->free = 0;
		void* free_end = (void*)startShare + sizeof(mementry) + startShare->size;
		void* alloc_end = (void*)startShare + sizeof(mementry) + size;
		int space = free_end - alloc_end;
		startShare->size = size;
		//enough space for metanode
		if(space>sizeof(mementry)){
			mementry* newNode = alloc_end;
			newNode ->free = 1;
			newNode ->size = space;
			newNode->prev = startShare;
			newNode->next = startShare->next;
			startShare->next = newNode; 		
		}
		//not enough space
		else{
		}
	}

	waitBool=0;
	return toReturn;
}






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
    struct Queue *q = (struct Queue*)myallocate(sizeof(struct Queue),__FILE__,__LINE__,0);
    q->front = q->tail = NULL;
	q->size = 0;
    return q;
}

queue * done_queue;
my_pthread_mutex_t ** wait_queue;
queue * running_queue;

//priority array - scheduler
 queue** priority = NULL;
 tcb * current_thread;
 tcb * prev_thread;
 tcb * mainThread;

 
 
 
 //enqueue and dequeue for waiting, done, running and individual prioirity level
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
	
	node * delete1 = q->front;
	q->front = q->front->next;
	delete1->next = NULL; 
	if(q->front == NULL){
		q->tail = NULL;
	}
	//what do we want to do with it
	q->size = q->size - 1;
	return delete1;
}


//enqueue and dequeue for our scheduler
void enqueue(int p, tcb * cb){
		node * insert = (node *) myallocate(sizeof(node),__FILE__,__LINE__,0);
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
void waiting_queue_init(my_pthread_mutex_t *mutex){
	int i = 0;
	mutex->locked = 0;    //unlocked at start
	mutex->isInit = 1;    //Initiliazed the lock with boolean 1
	if(queueInit == 0){
		wait_queue = ( my_pthread_mutex_t **)myallocate(sizeof( my_pthread_mutex_t *)*mutexSize,__FILE__,__LINE__,0); //malloc size of max amount of locks	
		while(i<5){
			wait_queue[i] = NULL;
			i++;
		}
		i=0;
		while(i<5){
			if(wait_queue[i]==NULL){   //initializing lock 
				wait_queue[i] = mutex; 
				break;
				
			}
			i++;
		}	
		
		queueInit = 1;
	}
	else{
		i=0;
		while(i<5){             //initializing lock 
			if(wait_queue[i] == NULL){
				wait_queue[i] = mutex; 
				break;
				
			}
		i++;
		}		
	}
}



node * dequeue(int p){
	queue * q = priority[p];
	
	if(q->front == NULL){
		return NULL;
	}
	
	node * delete2 = q->front;
	q->front = q->front->next;
	delete2->next=NULL;
	
	if(q->front == NULL){
		q->tail = NULL;
	}
	//what do we want to do with it
	q->size = q->size - 1;
	return delete2;
	
}


 //pick 16,8,4,2,1
 //try to only call swap once during function call
void my_handler(int signum){
	
	//printf("in scheduler\n");

	
	if(isInit==0){
		return;
	}
	if(waitBool==1){
		
		return;
	}
	
	//If thread called pthread_exit. Insert into done queue
	if(firstSwap!=0&&running_thread!=NULL&&running_thread->state == terminate){
		node * toInsert = (node *)myallocate(sizeof(node),__FILE__,__LINE__,0);
		toInsert->thread = running_thread;
		toInsert->next = NULL;
		enqueue_other(toInsert,done_queue);
		prev_thread = running_thread;
		//free(running_thread->cxt->uc_stack.ss_sp);
		running_thread = NULL;
		timeCounter =-1;
	}
	
	//If running thread is not null, that means the thread got interrupted
	if(running_thread!=NULL){
		
		//This is the case for if the thread did not finish running for its time slice(timeslice * priority)
		if(timeCounter != running_thread->priority){
			timeCounter++;
			//printf("%d\n",timeCounter);
			return;
		}
		else{
			
		//These cases are for if the thread is not done so insert it back into scheduler down a priority unless its already 4 because that is the max
	//	printf("I am running for %d\n",timeCounter);
		if(running_thread->priority ==4){
			prev_thread = running_thread;
			enqueue(4,running_thread);
			timeCounter = -1;
		}
		else{
			running_thread->priority = running_thread->priority +1;
			prev_thread = running_thread;
			if(running_thread->state == wait){
				
			}else{
			enqueue(running_thread->priority,running_thread);
			}
			timeCounter =-1;
		}
		running_thread = NULL;
		}
		
	}

		
	if(running_queue->size==0){
		if(totalSwap % 4 ==0){
		//	maintenanceCycle();
			//printf("totalswap num is %d\n",totalSwap);
			int t = 1;
			while(t<priorities){
			//printf("inserting into running\n");
			//Get number of threads we are picking at the priority level and enqueue into running queue
			//If there aren't enough threads in that level just go to the next
			int p = maintenancePick[t];
			int k = 0;
			while(priority[t]->size>0&&k<p){
				node * node_leaving1 = dequeue(t);
				//printf("inserting thread %d in level 0 from level: %d\n",node_leaving1 ->thread ->tid, node_leaving1->thread->priority);
				node_leaving1->thread->priority =0;
		
				enqueue_other(node_leaving1,priority[0]);
				k++;
				}
			t++;
			}
		
		
		}
		//start inserting into running queue
		int i = 0;
		while(i<priorities){
		//printf("inserting into running\n");
		//Get number of threads we are picking at the priority level and enqueue into running queue
		//If there aren't enough threads in that level just go to the next
		int p = pick[i];
		//queue * q = priority[i];
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
		//int running_priority = running_thread->priority;
	
		
		//Cases to account for when doing first swap.
		//Main thread will get dequeued first
		//Make it swap to another context
		if(running_thread->isMain == 1 && specialStop ==1){
			specialStop++;
			enqueue(0,running_thread);
			return;
		}
		
		if(firstSwap == 0 && running_thread->isMain == 1){
			//printf("main dequeue\n");
			prev_thread = running_thread;
			enqueue(0,running_thread);
			firstSwap = 1;
			node * temp = dequeue_other(running_queue);
			running_thread = temp->thread;
			//might have to move around nodes instead of just freeing
			//free(temp);
			//int running_priority = running_thread->priority;
			timeCounter++;
			totalSwap++;
			loadPages();
			swapcontext(prev_thread->cxt, running_thread->cxt);
		

		}else{
			//GO THROUGH THE ENTIRE RUNNING QUEUE
			//if we encounter the same context, let it run again
			if(prev_thread->cxt == running_thread ->cxt){
			//printf("swap same context\n");
				
				timeCounter++;
				return;
			}
			else{
			//Swap from current context to a new one
			//printf("prev:%i   run:%i\n",prev_thread->tid,running_thread->tid);
			timeCounter++;
			totalSwap++;
			loadPages();
			swapcontext(prev_thread->cxt, running_thread->cxt);
			}
			
		}
		//Check entire waiting queue for mutexes and joins
		
	
		//}
	
	}
	//printf("gone\n");

	
}

//switch back to main when everything is done
void garbage(){
	
	if(running_thread->isMain==1){
		//main is done
	//	printf("main done\n");
		mainDone = 1;
		
	}
	//printf("garbage\n");
	prev_thread = running_thread;
	running_thread =  NULL;
	//printf("garbage1\n");
	raise(SIGVTALRM);
}

void initialize(){
	if(isInit == 0){
		
		waitBool=1;
		//Allocate our 8MB physical memory in one go
		void * a = (void *)memalign(sysconf( _SC_PAGE_SIZE),4096 * 2048);
		memset(a,0,(4096*2048));
		void * search = a;
		int i=0;
		mementry * meta;
		meta = (mementry *)search;
		OSLandTotal -= 32;
		meta->size = 4194272;
		meta->next = NULL;
		meta->prev = NULL;
		meta->free = 1;
		pages[i] = search;
		search = (void *)((char *)(meta +1)+4064);
		i++;
		while(i<1024){
			pages[i]=search;
			search = search+4096;
			i++;
		}
		search = a;
		search = (void *)((char *)(meta +1)+4194272);
		i = 1024;
		//loop through our 4MB storing the address of each page in our void **. Each entry in void *pages refers to the address of a page(multiple of 4096)
		while(i<2045){
			mementry * meta;
			meta = (mementry *)search;
			if(i==2044)
				meta->size = 4096*4-32;
			else
				meta->size = 4064;

			meta->next = NULL;
			meta->prev = NULL;
			meta->free = 1;
			//printf("page address: %x\n" , search);
			pages[i] = search;
			search = (void *)((char *)(meta +1)+4064);
			i++;
		}
		swapfd = open("./swap.txt",O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
		lseek(swapfd,0,SEEK_SET);
		//static char buff[16777216];
		
		lseek(swapfd,4096*4096-1,SEEK_SET);
		write(swapfd,"0",1);
		lseek(swapfd,0,SEEK_SET);
		close(swapfd);
		isInit = 1;
		done_queue = createQueue();//for threads that are done, and might be waited on
		running_queue = createQueue(); // queue for threads that are ready to run
		running_thread = NULL;
		//ready_queue = createQueue();
		
		current_thread = (tcb *) myallocate(sizeof(tcb),__FILE__,__LINE__,0);
		//current_thread = (tcb *) malloc(sizeof(tcb)); //thread we are currently executing for a certain time slice
		signal(SIGVTALRM, my_handler); //linking our handler to the OS
		priority = ( queue **)myallocate(sizeof( queue *)*5,__FILE__,__LINE__,0); //our scheduler data structure. an array of queues
		i = 0;
		current_mutex = NULL;
		//initialize our array of queues(scheduler)
		while(i<priorities){
			priority[i] = createQueue();
			i++;
		}
		
		
		
		//Create a context for our garbage collector. All threads will go here once done
		uctx_garbage = (ucontext_t *)myallocate(sizeof(ucontext_t),__FILE__,__LINE__,0);
		if(getcontext(uctx_garbage)==-1){
		perror("getcontext failed");
		exit(0);
	}
		void * garstack = myallocate(STACK_SIZE,__FILE__,__LINE__,0);
		uctx_garbage->uc_stack.ss_sp = garstack;
		uctx_garbage->uc_stack.ss_size = STACK_SIZE;
		uctx_garbage->uc_link = 0;
		uctx_garbage->uc_stack.ss_flags=0;	
		makecontext(uctx_garbage,(void *)&garbage,0);
		
		//get context of main
		mainThread = (tcb *) myallocate(sizeof(tcb),__FILE__,__LINE__,0);
		uctx_main = (ucontext_t *)myallocate(sizeof(ucontext_t),__FILE__,__LINE__,0);
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
		mainThread->dir= (pageTable*) myallocate(sizeof(pageTable),__FILE__,__LINE__,0);
	
		i = 0;
		while(i<128){
			mainThread->dir->table[i]=NULL;
			i++;
		}
		mainThread->dir->isEmpty = 0;
		
		
		
		//Put main into our scheduler. Treat it like any other thread
		enqueue(0,mainThread);
		
		
		//set up context of my_handler. Do we need this?
	
		uctx_handler = (ucontext_t *)myallocate(sizeof(ucontext_t),__FILE__,__LINE__,0);
		if(getcontext(uctx_handler)==-1){
		perror("getcontext failed");
		exit(0);
	}
		void * stack = myallocate(STACK_SIZE,__FILE__,__LINE__,0);
		uctx_handler->uc_stack.ss_sp = stack;
		uctx_handler->uc_stack.ss_size = STACK_SIZE;
		uctx_handler->uc_link = 0;
		uctx_handler->uc_stack.ss_flags=0;	
		makecontext(uctx_handler,(void *)&my_handler,1, SIGVTALRM);
		
		
		//setting itimer
		
		interval.tv_sec = 0;
		interval.tv_usec = slice;
		
		timer.it_interval = interval;
		timer.it_value = interval;
		//?? this timer may or may not act within pthread create time
		setitimer(ITIMER_VIRTUAL,&timer,NULL);
		
		
		

		
		//Construct free list. List of free pages
		i =1024;
		while (i < 2044){
			freeNode* fn= (freeNode*)myallocate(sizeof(freeNode),__FILE__,__LINE__,0);
			fn-> freePage = pages[i];
			if(i == 1024){
				head = fn;
				i++;
				continue;
			}
			else{
				freeNode * prev = head;
				freeNode * ptr = head;
				while(ptr!=NULL){
						prev = ptr;
						ptr = ptr->next;
				}
				prev->next = fn;
			}
			i++;
		}
		//my_pthread_yield();
		waitBool = 0;
		isInit = 1;
	}else{
		return;
	}
}

void swapfile(){
	swapfd = open("./swap.txt",O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
	lseek(swapfd,0,SEEK_SET);
	
	int evictPages = 0;
	
	int i = 1024;
	while(i < 2044){ //find next spot to insert the page
				if(membook[i].threadOwner == 0){
					break;
				}
			
				if(running_thread->tid != membook[i].threadOwner->tid && membook[i].threadOwner!=NULL){
					break;
				}
				i++;
			}
	
	
}

void * myallocate(int x,char file[], int line,  int req){
    
    //Load/get all the pages for that thread.
	if(isInit==0){
		initialize();
		specialStop = 1;
		//enqueue(0, control_block);
		my_pthread_yield();
	}
    
    //1 page, look through array, find first free page,
	//either thread has empty page table or it does not
	waitBool = 1;

	
	void * a = NULL;
	void * b = NULL;
	mementry * meta = NULL;
	mementry * metaExtra = NULL;

	int size = x;
	int extraPages = 0;

	if(req ==1){	//thread malloc,put 1 for the macro
			pageTable * currTable = running_thread->dir;
		
		
		
		
	//if empty page table, check if thread needs 1 page, or multiple pages
	if(currTable->isEmpty==0){
		
		//If the user only needs one page and empty
		//MAKE SURE TO ADD META

		int sizeLeft = x-4064;
		extraPages = (sizeLeft +4096 -1)/4096;//Additional pages needed for request. total pages during this malloc is extraPages+1
		pageEntry * lastPageAdded = NULL;

		 
		
		
			a = pages[1024]; //start at 1024
			meta = (mementry *)a;
			 //int sizeLeft = x-4064;
			// int extraPages = (sizeLeft +4096 -1)/4096;//Additional pages needed for request. total pages during this malloc is extraPages+1
			
			//case where on first malloc thread needs multiple pages
			if(extraPages>0){
				
				//first check if we have enough pages left for thread to malloc
				
					
					//check if first page is free. if not swap.
					if(meta->free == 0){
						//swap first page out
						freeNode * swapPage = head;
						freeNode * holder = head->next;
						swapPage->next = NULL;
						head = holder;

						pagesLeft--;

						//Find location of free page in physical memory so that we know where to swap
						int j = 0;
						while(j<2044){
							if(pages[j]==swapPage->freePage){
								break;
							}
							j++;
						}
						membook[j].threadOwner = running_thread;
						swap(pages[1024],swapPage->freePage,NULL,1024);
					}
					else{
						
						head = head->next;

						pagesLeft--;
					}
					
					a = pages[1024];//ensure we are using the right metadata struct
					meta = (mementry *)a;
					meta->size = x;
					meta->prev = NULL;
					meta->free = 0;
					
					//first page
					pageEntry * pe=(pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
					pe->isValid = 1;
					pe->physicalMemLoc = 1024;//this should be frame number change later
					pe->swapLoc = -1;
					pe->initSpot = 1024;
					pe->next = NULL;
					pe->prev = NULL;
					pe->largestAvailSize = 0;
				
				
					//get page from free list.
					//put first page in immediately
					currTable->table[0] = pe;
					currTable->isEmpty = 1;
					membook[pe->initSpot].threadOwner = running_thread;//duplicate
					
					int counter = 1; //to assign correct spots in memory
					//keep taking pages out of the free list. make sure these pages are dependent.
					while(extraPages>0){
						freeNode * swapPage = head;
						freeNode * holder = head->next;
						swapPage->next = NULL; 
						head = holder;

						pagesLeft--;

						//Find location of free page in physical memory so that we know where to swap
						int j = 0;
						while(j<2044){
							if(pages[j]==swapPage->freePage){
								break;
							}
							j++;
						}
						membook[j].threadOwner = running_thread;
						//Dont want to swap the same page. No point
						if(pages[counter] != swapPage->freePage){
							swap(pages[counter],swapPage->freePage,NULL,counter);
						}
						
						memset(pages[counter],'0',4096);
						pageEntry * extraPE = (pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
						extraPE->isValid = 1;
						extraPE->physicalMemLoc = counter;
						extraPE->swapLoc = -1;
						extraPE->initSpot = counter;
						
						membook[extraPE->initSpot].threadOwner = running_thread;
						extraPE->next = NULL;
						extraPE->prev = currTable->table[counter-1];
						if(extraPages>1){
							extraPE->largestAvailSize = 0;
						}
						currTable->table[counter-1]->next = extraPE;
						currTable->table[counter] = extraPE;
						counter++;
						lastPageAdded = extraPE;
						sizeLeft = sizeLeft-4096;
						extraPages--;
					}
					
					//check if we can fit a meta data. multiple page case
					sizeLeft = sizeLeft*-1;
					lastPageAdded->largestAvailSize =sizeLeft-sizeof(mementry);
					if(sizeLeft>33){
								
								mementry * newMeta;
								newMeta = (mementry *)((char *)(meta +1)+x);
								newMeta->size = sizeLeft-sizeof(mementry);

								newMeta->free = 1;

								newMeta->prev = meta;
								newMeta->next = NULL;
								meta->next =newMeta;
								
							}
							
					//put page in threads page table and update free list
					//currTable->table[0] = pe;
					//currTable->isEmpty = 1;
					//head = head->next;
					
					//return meta+1;
				
				
				//If a thread only needs one page
			}else{
					//swapping pages if the first page is already taken
					if(meta->free == 0){
						freeNode * swapPage= NULL;
						if(head == NULL){
							//No more pages so return null
							swapfile();
							printf("ran out of pages\n");
							return NULL;
						}else{
							swapPage = head;
							freeNode * holder = head->next;
							swapPage->next = NULL;
							head = holder;
						}
						//find the right physical location of the freePage
						int j = 1024;
						while(j<2044){
							if(pages[j]==swapPage->freePage){
								break;
							}
							j++;
						}
						membook[j].threadOwner = running_thread;
						swap(pages[1024],swapPage->freePage,NULL,1024);
						
					}
					else{
						head = head->next;
					}
					
					pageEntry * pe=(pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
					a = pages[1024]; //ensure we are using the right meta data struct
					meta = (mementry *)a;
					meta->size = x;
					meta->prev = NULL;
					meta->free = 0;
					pe->isValid = 1;
					pe->physicalMemLoc = 1024;//make sure these are consistent
					pe->swapLoc = -1;
					pe->initSpot = 1024;
					membook[pe->initSpot].threadOwner = running_thread;
					
					pe->next = NULL;
					pe->prev = NULL;
					pagesLeft--;//Decrement number pages left
					//checking if we can fit a new metadata
					if(4064-x>33){
							pe->largestAvailSize = 4064-x - sizeof(mementry);
							mementry * newMeta;
							newMeta = (mementry *)((char *)(meta +1)+x);
							newMeta->size = 4064-x-sizeof(mementry);

							newMeta->free = 1;//change this one for testing

							newMeta->prev = meta;
							newMeta->next = NULL;
							meta->next =newMeta;
							
						}
						else{
								pe->largestAvailSize = 4064-x;
						}
					currTable->table[0]  = pe;
					currTable->isEmpty = 1;
					//return meta+1;
					
				}
			
			
			
	}else{
	//else load pages and try to find space in the page traverse
	a = pages[1024]; //start at 1024
	meta = (mementry *)a;
	
	//((mementry*)pages[0])->free = 1;
	int sizeLeft = x-4064;
	extraPages = (sizeLeft +4096 -1)/4096;//Additional pages needed for request. total pages during this malloc is extraPages+1
	pageEntry * lastPageAdded = NULL;
	
	//traverse the meta datas and find the first place you can fit the malloc. If you reach the end we can spill over to next pages
	while(meta!=NULL){
		if(meta->size>=x&& meta->free == 1){
			break;
		}
		meta = meta->next;
	}
	
	if(meta==NULL){
		meta = (mementry *)a;
		
		while(meta!=NULL){
		if(meta->next==NULL){
			break;
		}
		meta = meta->next;
		}
	}
	
	//Now we have the right metadata
	if(meta->next==NULL){
		//At the end
		//Check if we can fit it in
		if(meta->size>=x&&meta->free==1){
			//Good enough to put it in
			int remaining = meta->size-x;
			meta->size = x;
			meta->free = 0;
			//make new mementry
			int pageFinder = getpage(meta);
			int l =0;
				while(l<128){
					if(currTable->table[l]!=NULL &&currTable->table[l]->physicalMemLoc==pageFinder){
					break;
					}
					
				l++;
			}
				
			printf(" the remainder %d\n",remaining);
			if(remaining>33){
				//update largestAvailSize here
				
				mementry * newMeta;
				currTable->table[l]->largestAvailSize = currTable->table[l]->largestAvailSize - x - sizeof(mementry); //dont know if the sizeof is necessary
				newMeta = (mementry *)((char *)(meta +1)+x);
				newMeta->size = remaining - sizeof(mementry);
				newMeta->free = 1;
				newMeta->prev = meta;
				newMeta->next = NULL;
				meta->next =newMeta;
			}
			else{
				currTable->table[l]->largestAvailSize = currTable->table[l]->largestAvailSize - x;
				
			}
			
		}else{
				//Need more pages
				
				if(meta->free ==0){
					
				int sizeLeft = x-4064;
				extraPages = (sizeLeft +4096 -1)/4096;//Additional pages needed for request. total pages during this malloc is extraPages+1
				pageEntry * lastPageAdded = NULL;
				int i=1024;
				//loadPages();
			while(i < 2044){ //find next spot to insert the page
					if(membook[i].threadOwner == 0){
						break;
					}
				
					if(running_thread->tid != membook[i].threadOwner->tid && membook[i].threadOwner!=NULL){
						break;
					}
					i++;
				}
					
				a = pages[i];
				freeNode * swapPage;
				
				if(head == NULL){
					//No more pages so return null
					swapfile();
					printf("ran out of pages\n");
					return NULL;
					}else{
					
					swapPage = head;
					freeNode * holder = head->next;
					swapPage->next = NULL;
					head = holder;
					}
					
					
						//find the right physical location of the freePage
					int j = 1024;
					while(j<2044){
						if(pages[j]==swapPage->freePage){
							break;
						}
						j++;
					}
					membook[j].threadOwner = running_thread;
					if(pages[i] != swapPage->freePage){
						swap(pages[i],swapPage->freePage,NULL,i);
					}
					//i++;
					
					pageEntry * pe=(pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
					pe->isValid = 1;
					pe->physicalMemLoc = i;//make sure these are consistent
					pe->swapLoc = -1;
					pe->initSpot = i;
					pe->next = NULL;
					pe->prev = NULL;
					
					mementry * me = (mementry*) a;
					me->size = x;
					me->prev=meta;
					me->next = NULL;
					me->free = 0;
					meta->next = me;
					
					//Just needed on extra page
					if(extraPages == 0){
						sizeLeft = 4096-x-sizeof(mementry);
						
						if(sizeLeft>33){
									pe->largestAvailSize =sizeLeft-sizeof(mementry);
									mementry * newMeta;
									newMeta = (mementry *)((char *)(me +1)+x);
									newMeta->size = sizeLeft-sizeof(mementry);
									newMeta->free = 1;
									newMeta->prev = me;
									newMeta->next = NULL;
									me->next =newMeta;
									membook[i].threadOwner = running_thread;
									int l =0;
									 while(l<128){
										if(currTable->table[l]==NULL){
										currTable->table[l] = pe;
										break;
										}
									l++;
									}
									
								}else{
									pe->largestAvailSize =sizeLeft;
								}
								waitBool = 0;
								return me+1;
					}
					
					pageEntry * prevPage = pe;
					
					int l = 0;
					while(l<128){
						if(currTable->table[l]==NULL){
							currTable->table[l] = pe;
							break;
						}
						l++;
					}
					i++;
				while(extraPages>0){
					
					
				if(head == NULL){
					//No more pages so return null
					swapfile();
					printf("ran out of pages\n");
					return NULL;
					}
					
					else{
					freeNode * swapPage = head;
					freeNode * holder = head->next;
					swapPage->next = NULL; 
					head = holder;
					pagesLeft--;
					}
					
					//Find location of free page in physical memory so that we know where to swap
					int j = 1024;
					while(j<2044){
						if(pages[j]==swapPage->freePage){
							break;
						}
						j++;
					}
					membook[j].threadOwner = running_thread;
					//Dont want to swap the same page. No point
					if(pages[i] != swapPage->freePage){
						swap(pages[i],swapPage->freePage,NULL,i);
					}
					
					memset(pages[i],'0',4096);
					pageEntry * extraPE = (pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
					extraPE->isValid = 1;
					extraPE->physicalMemLoc = i;
					extraPE->swapLoc = -1;
					extraPE->initSpot = i;
					
					membook[extraPE->initSpot].threadOwner = running_thread;
					
					extraPE->next = NULL;
					extraPE->prev = prevPage;
					prevPage->next = extraPE;
					prevPage = extraPE;
					if(extraPages>1){
						extraPE->largestAvailSize = 0;
					}
					l = 0;
					while(l<128){
						if(currTable->table[l]==NULL){
							currTable->table[l] = extraPE;
							break;
						
						}
						l++;
					}
					//currTable->table[i-1]->next = extraPE;
					//currTable->table[i] = extraPE;
					i++;
					lastPageAdded = extraPE;
					sizeLeft = sizeLeft-4096;
					extraPages--;
					
					
				}
				
				sizeLeft = sizeLeft*-1;
				
				if(sizeLeft>33){
							lastPageAdded->largestAvailSize =sizeLeft-sizeof(mementry);
							mementry * newMeta;
							newMeta = (mementry *)((char *)(meta +1)+x);
							newMeta->size = sizeLeft-sizeof(mementry);
							newMeta->free = 1;
							newMeta->prev = me;
							newMeta->next = NULL;
							me->next =newMeta;
							
							
						}else{
							lastPageAdded->largestAvailSize =sizeLeft;
						}
					meta = me;
				
				//Merge pages and then add additional pages
				}else{
					int oldSize = meta->size;
					sizeLeft = x - meta->size;
					int sizeLeftForOnePage = sizeLeft;
					meta->size = x;
					extraPages = (sizeLeft +4096 -1)/4096;
					
						int i=1024;
				
					while(i < 2044){ //find next spot to insert the page
						if(membook[i].threadOwner == 0){
							break;
						}
					
						if(running_thread->tid != membook[i].threadOwner->tid && membook[i].threadOwner!=NULL){
							break;
						}
						i++;
					}
						
					a = pages[i];
					//get current page, update size, and pointers
					int l = 0;
					pageEntry * prevPage = NULL;
					while(l<128){
						if(currTable->table[l]!=NULL&&currTable->table[l]->physicalMemLoc == i-1){
							prevPage = currTable->table[l];
							prevPage->largestAvailSize = prevPage->largestAvailSize - oldSize;
							break;
						
						}
						l++;
					}
					
					while(extraPages>0){
						
						freeNode * swapPage;
				if(head == NULL){
					//No more pages so return null
					swapfile();
					printf("ran out of pages\n");
					return NULL;
					}
					else{
						swapPage = head;
						freeNode * holder = head->next;
						swapPage->next = NULL; 
						head = holder;
						pagesLeft--;
					}
					//Find location of free page in physical memory so that we know where to swap
						int j = 1024;
						while(j<2044){
							if(pages[j]==swapPage->freePage){
								break;
							}
							j++;
						}
						membook[j].threadOwner = running_thread;
						//Dont want to swap the same page. No point
						if(pages[i] != swapPage->freePage){
							swap(pages[i],swapPage->freePage,NULL,i);
						}
						
						memset(pages[i],'0',4096);
						pageEntry * extraPE = (pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
						extraPE->isValid = 1;
						extraPE->physicalMemLoc = i;
						extraPE->swapLoc = -1;
						extraPE->initSpot = i;
						
						membook[extraPE->initSpot].threadOwner = running_thread;
						extraPE->next = NULL;
						prevPage->next = extraPE;
						extraPE->prev = prevPage;
						
						prevPage = extraPE;
						if(extraPages>1){
							extraPE->largestAvailSize = 0;
						}
						l = 0;
					   while(l<128){
						   if(currTable->table[l]==NULL){
							   currTable->table[l] = extraPE;
							   break;
						   
						   }
						   l++;
					   }
					   //currTable->table[i-1]->next = extraPE;
					   //currTable->table[i] = extraPE;
					   i++;
					   lastPageAdded = extraPE;
					   sizeLeft = sizeLeft-4096;
					   extraPages--;
					
					
				}
				
				sizeLeft = sizeLeft*-1;
				
				if(sizeLeft>33){
					lastPageAdded->largestAvailSize =4096 - sizeLeftForOnePage-sizeof(mementry);
					mementry * newMeta;
					newMeta = (mementry *)((char *)(meta +1)+x);
					newMeta->size = sizeLeft-sizeof(mementry);
					newMeta->free = 1;
					newMeta->prev = meta;
					newMeta->next = NULL;
					meta->next = newMeta;
					meta->free = 0;
					
					
				}else{
					lastPageAdded->largestAvailSize =4096-sizeLeftForOnePage;
				}
					
					
					
				}
		
		}
	}else{
		
		if(getpage(meta)!=getpage(meta->next)){
			
			int leftExtra = pages[(getpage(meta)+1)] - (void*)(meta+1);
			int rightExtra = (void*)(meta->next) - pages[(getpage(meta->next))];
			
			if(x <= leftExtra){
				int remaining = meta->size-x;
				int newLargestAvail = leftExtra - x;
				meta->size = x;
				meta->free = 0;
				
				int i = 0;
				pageEntry * pe = NULL;
				//get the page entry where the address is located. Helps us find see if there are dependencies
				int pageNum = getpage(meta);
				while(i<128){
					pe = running_thread->dir->table[i];
					if(pe!=NULL&&pe->initSpot==pageNum){
						break;
					}
					i++;
				}
				
				if(newLargestAvail>33){
					pe->largestAvailSize = pe->largestAvailSize -x - sizeof(mementry);
					mementry * newMeta;
					newMeta = (mementry *)((char *)(meta +1)+x);
					newMeta->size = remaining - sizeof(mementry);
					newMeta->free = 1;
					newMeta->prev = meta;
					newMeta->next = meta->next;
					newMeta->next->prev = newMeta;
					meta->next =newMeta;
					}else{
					pe->largestAvailSize = pe->largestAvailSize - x;
						
						if(pagesLeft>0){
						//Need more pages
						
						if(meta->free ==0){
							
							//int sizeLeft = x-4064;
							//extraPages = (sizeLeft +4096 -1)/4096;//Additional pages needed for request. total pages during this malloc is extraPages+1
							pageEntry * lastPageAdded = NULL;
							 i++; //next spot
							//loadPages();
							i = pe->initSpot;
							while(i < 2044){ //find next spot to insert the page
								if(membook[i].threadOwner == 0){
									break;
								}
							
								if(running_thread->tid != membook[i].threadOwner->tid && membook[i].threadOwner!=NULL){
									break;
								}
								i++;
							}
								
							a = pages[i];
				
							freeNode * swapPage;
							if(head == NULL){
								
								//No more pages so return null
								swapfile();
								printf("ran out of pages\n");
								return NULL;
								}else{
								
								swapPage = head;
								freeNode * holder = head->next;
								swapPage->next = NULL;
								head = holder;
								}
									//find the right physical location of the freePage
								int j = 1024;
								while(j<2044){
									if(pages[j]==swapPage->freePage){
										break;
									}
									j++;
								}
								membook[j].threadOwner = running_thread;
								if(pages[i] != swapPage->freePage){
									swap(pages[i],swapPage->freePage,NULL,i);
								}
								//i++;
								
								pageEntry * pe2=(pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__, 0);
								pe2->isValid = 1;
								pe2->physicalMemLoc = i;//make sure these are consistent
								pe2->swapLoc = -1;
								pe2->initSpot = i;
								pe2->next = pe->next;
								pe2->prev = pe;
								pe->next->prev = pe2;
								pe->next = pe2;
								
								
								mementry * me = (mementry*) a;
								me->size = remaining;
								me->prev=meta;
								me->next = meta->next;
								me->free = 1;
								meta->next->prev = me;
								meta->next = me;
								
								
								pe2->largestAvailSize =4096-sizeof(mementry);
								
								int l =0;
										 while(l<128){
											if(currTable->table[l]==NULL){
												currTable->table[l] = pe2;
											break;
												}
											l++;
										}
								membook[i].threadOwner = running_thread;
							
						}
						}
					}
			}
			else{
				int remaining = x- leftExtra;
				meta->size = x;
				meta->free = 0;
				
				
				int i = 0;
			
					
					pageEntry * pe = NULL;
					//get the page entry where the address is located. Helps us find see if there are dependencies
					int pageNum = getpage(meta);
					while(i<128){
						pe = running_thread->dir->table[i];
						if(pe!=NULL&&pe->initSpot==pageNum){
							break;
					}
					i++;
					}
					pe->largestAvailSize = pe->largestAvailSize -leftExtra;
					i = pe->initSpot;
					//may have to change the i because we can have a blank spot before this open page
						while(remaining -4096 >= 4096){  //calucalting how much to put in an open page, when exits will be at the right end
							
							while(i < 2044){ //find next spot to insert the page
								if(membook[i].threadOwner == 0){
									break;
								}
							
								if(running_thread->tid != membook[i].threadOwner->tid && membook[i].threadOwner!=NULL){
									break;
								}
								i++;
							}
								
							a = pages[i];
					freeNode * swapPage;
					
								if(head == NULL){
								
								//No more pages so return null
								swapfile();
								
								printf("ran out of pages\n");
								return NULL;
								}
								else{
								
								swapPage = head;
								freeNode * holder = head->next;
								swapPage->next = NULL;
								head = holder;
								}
									//find the right physical location of the freePage
								int j = 1024;
								while(j<2044){
									if(pages[j]==swapPage->freePage){
										break;
									}
									j++;
								}
								membook[j].threadOwner = running_thread;
								if(pages[i] != swapPage->freePage){
									swap(pages[i],swapPage->freePage,NULL,i);
								}
								//i++;
								
								pageEntry * pe2=(pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
								pe2->isValid = 1;
								pe2->physicalMemLoc = i;//make sure these are consistent
								pe2->swapLoc = -1;
								pe2->initSpot = i;
								pe2->next = pe->next;
								pe2->prev = pe;
								pe->next->prev = pe2;
								pe->next = pe2;
								pe2->largestAvailSize = 0;
					
					
					
					
					
					
					
					pe = pe2;
					remaining = remaining - 4096;
				}
						i++; //either putting mementry into another free page or where the right extra is located
				//pe = running_thread->dir->table[i];
				
				if(membook[i].threadOwner == 0 || running_thread->tid != membook[i].threadOwner->tid && membook[i].threadOwner!=NULL){  //another free page
								a = pages[i];
								freeNode * swapPage;
								if(head == NULL){
								//No more pages so return null
								printf("ran out of pages\n");
								swapfile();
								return NULL;
								}
								else{
								
								swapPage = head;
								freeNode * holder = head->next;
								swapPage->next = NULL;
								head = holder;
								}
									//find the right physical location of the freePage
								int j = 1024;
								while(j<2044){
									if(pages[j]==swapPage->freePage){
										break;
									}
									j++;
								}
								membook[j].threadOwner = running_thread;
								if(pages[i] != swapPage->freePage){
									swap(pages[i],swapPage->freePage,NULL,i);
								}
								//i++;
								
								pageEntry * pe3=(pageEntry *)myallocate(sizeof(pageEntry),__FILE__,__LINE__,0);
								pe3->isValid = 1;
								pe3->physicalMemLoc = i;//make sure these are consistent
								pe3->swapLoc = -1;
								pe3->initSpot = i;
								
								
								if(4096 -remaining >33){
									pe3->largestAvailSize = 4096 -remaining - sizeof(mementry);
									mementry * newMeta;
									newMeta = (mementry *)((char *)(meta +1)+x);
									newMeta->size = remaining - sizeof(mementry);
									newMeta->free = 1;
									newMeta->prev = meta;
									newMeta->next = meta->next;
									newMeta->next->prev = newMeta;
									meta->next =newMeta;
									
									pe3->next = pe->next;
									pe3->prev = pe;
									pe->next->prev = pe3;
									pe->next = pe3;
									}else{
										pe3->largestAvailSize = 4064- remaining;
									}
							
							
							
							
				}
				
				
				
				
				else{ // on rightextra page
				
				pageEntry * peRight = running_thread->dir->table[i]; // get page entry of the rightExtra page
				
				
				
						if(rightExtra>33){
							peRight->largestAvailSize = peRight->largestAvailSize - remaining - sizeof(mementry);
							mementry * newMeta;
							newMeta = (mementry *)((char *)(meta +1)+x);
							newMeta->size = remaining - sizeof(mementry);
							newMeta->free = 1;
							newMeta->prev = meta;
							newMeta->next = meta->next;
							newMeta->next->prev = newMeta;
							meta->next =newMeta;
							pe->next = peRight;
							peRight->prev = pe;
							}else{
								pe->largestAvailSize = pe->largestAvailSize -remaining;
					}
				}
				
			}
		}
		else{
		
		
		//Somwhere in the middle
		int remaining = meta->size-x;
		meta->size = x;
		meta->free = 0;
		
		int i = 0;
		pageEntry * pe = NULL;
		//get the page entry where the address is located. Helps us find see if there are dependencies
		int pageNum = getpage(meta);
		while(i<128){
			pe = running_thread->dir->table[i];
			if(pe!=NULL&&pe->initSpot==pageNum){
				break;
			}
			i++;
		}
		
		
		//make new mementry
		if(remaining>33){
			pe->largestAvailSize = pe->largestAvailSize -x - sizeof(mementry);
			mementry * newMeta;
			newMeta = (mementry *)((char *)(meta +1)+x);
			newMeta->size = remaining - sizeof(mementry);
			newMeta->free = 1;
			newMeta->prev = meta;
			newMeta->next = meta->next;
			newMeta->next->prev = newMeta;
			meta->next =newMeta;
		}else{
			pe->largestAvailSize = pe->largestAvailSize -x;
		}
		
		}
		
		
			}

    

	}
	
	
	
	
		pageTable * tab = running_thread->dir;
		int z = 0;
		while(z<128){
			pageEntry * page = tab->table[z];
			//mementry * m = 
			while(page!=NULL){
			printf("size:%i, physcial mem:%i validd:%i\n",page->largestAvailSize,page->physicalMemLoc,page->isValid);
			page = page->next;
			}
			z++;
				}
    //multiple pages
}

			else{ // OS malloc
				a = pages[0];
				meta = (mementry*)a;
				
				//traverse the meta datas and find the first place you can fit the malloc.
				while(meta!=NULL){
				if(meta->size>=x&& meta->free == 1){
					break;
				}
				meta = meta->next;
				}
			
				if(meta==NULL){
					meta = (mementry *)a;
					
					while(meta!=NULL){
					if(meta->next==NULL){
						break;
					}
					meta = meta->next;
					}
				}
				int oldSize = meta->size;
				if(meta->size>=x&&meta->free==1){
					
					meta->size = x;
					meta->free = 0;
					OSLandTotal = OSLandTotal - x;
					if(OSLandTotal > 33){
							mementry * newMeta;
							newMeta = (mementry *)((char *)(meta +1)+x);
							newMeta->size = oldSize - sizeof(mementry);
							newMeta->free = 1;
							newMeta->prev = meta;
							newMeta->next = NULL;
							meta->next =newMeta;
							OSLandTotal = OSLandTotal - sizeof(mementry);
					}
				
				
				
				}
			
	
	
	waitBool = 0;
	return meta+1;
	
	
			}
	
	

    
    waitBool = 0;
    return meta+1;
}





void print_schedule(){
	/*int i = 0;
	while(i<priorities){
		queue * q = priority[i];
		node * search = q->front;
		while(search!=NULL){
			tcb * cb = search->thread;
		//	printf("Priority:%i \t Size:%i tid:%i\n",i,q->size,cb->tid);
			search = search->next;
		}
		i++;
	}
*/
}


void wrapper(void *(*function)(void*), void* arg){
	waitBool = 1;
	void* retval = function(arg);
	waitBool = 0;
	my_pthread_exit(retval);
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	waitBool =1;
	if(isInit==0){
		//isInit++;
	initialize();
	}
	
	//Start setting up the control block for the thread
	tcb *control_block = (tcb*)myallocate(sizeof(tcb),__FILE__,__LINE__,0);
	control_block->tid = tid;
	tid++;
	control_block->state = embryo;
	control_block->dir= (pageTable*) myallocate(sizeof(pageTable),__FILE__,__LINE__,0);
	
	int i = 0;
	while(i<128){
		control_block->dir->table[i]=NULL;
		i++;
	}
	control_block->dir->isEmpty = 0;

	
	//Make a context for the thread
	ucontext_t * c = (ucontext_t *)myallocate(sizeof(ucontext_t),__FILE__,__LINE__,0);
	

	if(getcontext(c)==-1){
		perror("getcontext failed");
		exit(0);
	}
	
	
	void * stack = myallocate(STACK_SIZE,__FILE__,__LINE__,0);
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
	
	//printf("SUP");
		
	//Put thread into our scheduler
	enqueue(0, control_block);
	
	
	
	
		waitBool=0;
	//Yield so scheduler can do stuff
	my_pthread_yield();
	return 0;
	};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	raise(SIGVTALRM);
	
	return 0;
};

/* terminate a thread */
//make done array
//set tcb ret val to value ptr
//for join continue based on TID check, set value_ptr
//add to complete just in case thread calls join
void my_pthread_exit(void *value_ptr) {
	waitBool = 1;
	if(value_ptr == NULL){
		running_thread->state = terminate;
		running_thread->return_val =NULL;
		waitBool=0;
		my_pthread_yield();
	}
	else{
		running_thread->state = terminate;
		running_thread->return_val = (void *) value_ptr;
		waitBool=0;
		my_pthread_yield();
	}
};

/* wait for thread termination */

int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	//Upon call, search done queue
	//If its there, check value_ptr and set accordingly and return
	//If not there, move to waiting queue,
	
	//waitBool=1;
	
	int foundIt = 0;
	while(1){
		if(done_queue->size != 0){
			node * search = done_queue->front;
		
			while(search!=NULL){
				if(search->thread->tid == thread){
					if(search->thread->return_val == NULL || value_ptr == NULL){
						//*value_ptr = 0;
				}
					else{
					*value_ptr = search->thread->return_val;
					}
					foundIt=1;
					break;
				}
				search = search->next;
			
			}
			if(foundIt==1){
				
				break;
			}
			if(timeCounter > 4){
				timeCounter =0;
				
			}
		}
		
		
	}
		//not there, so wait
		//running_thread
	//waitBool=0;
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	//mutex = (my_pthread_mutex*)malloc(sizeof(my_pthread_mutex));
	mutex-> locked =0;
	mutex->isInit=1;
	mutex->wait = NULL;
	waiting_queue_init(mutex);
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	
	//current_mutex = mutex;
//raise(SIGVTALRM);


		if(mutex ==NULL){
			return -1;
		}
		waitBool = 1;
		int i =0;
		while(i < 5){
			if(wait_queue[i]==mutex){
				if(wait_queue[i]->locked==0){
break;
					//return;
					// return back to computations
				}
				else{
					tcb* search = wait_queue[i]->wait;
					tcb* prev = search;
					while(search!= NULL){
						prev = search;
						search = search->next;
					}
					if(prev == NULL){
						wait_queue[i]->wait = running_thread;
						running_thread->state = wait;
					}
					else{
					prev->next = running_thread;
					running_thread->state = wait;
					}
					break;
				}
				
			
			}
		
		
		
		}
	/*if(mutex->isInit == 0){
		return;
	}
	*/
	waitBool=0;
//	printf("lock\n");
	while(__atomic_test_and_set((volatile void*)&mutex->locked,__ATOMIC_RELAXED)){
		//break;
		my_pthread_yield();
		//printf("im in the loop, bitches\n");
		
	}
	/*if(mutex->locked == 0){
		return;
	}*/
	//mutex->wait = running_thread;
	
	
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	
	
	if(mutex ==NULL){
			return -1;
		}
		waitBool=1;
		int i =0;
		while(i < 5){
			if(wait_queue[i]==mutex){
				if(wait_queue[i]->wait!=NULL){
					node * temp = (node*) myallocate(sizeof(node),__FILE__,__LINE__,0);
					temp->thread=wait_queue[i]->wait;
					temp->thread->state = running;
					
					if(temp->thread->next !=NULL){
						wait_queue[i]->wait=wait_queue[i]->wait->next;
						temp->thread->next = NULL;
						enqueue_other(temp,running_queue);
						//printf("dequeueing thread in if: %d\n", temp->thread->tid);
						break;
					}
					else{
						wait_queue[i]->wait = NULL;
						temp->thread->next = NULL;
						enqueue_other(temp,running_queue);
						//printf("dequeueing thread in else: %d\n", temp->thread->tid);
						break;
					}
					
					
				}
				
				
				
			}
			i++;
	
		}
	
	mutex->locked = 0;
	waitBool=0;
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	waitBool=1;
	int i =0;
		while(i < 5){
			if(wait_queue[i]==mutex){
				if(wait_queue[i]->wait!=NULL){
					
					
					/*tcb *search = wait_queue[i]->wait;
					while(search!=NULL){
						node * temp = (node*) malloc(sizeof(node));
						temp->thread=search;
						printf("inserting back into running\n");
						temp->thread->state = running;
						enqueue_other(temp,running_queue);
						tcb *prev = search;
						search = search->next;
						
					}
					*/
					
					
				}
				mutex->isInit = 0;
				mutex->locked = 0;
					wait_queue[i]=NULL;
				
				
				
			}
			i++;
	
		}
	waitBool=0;
	return 0;
};


//change to i-1 around line 630
//change to mementry line 506 and 509
//added inserting in membook and running thread table at 519
//random i++ at 487
//add i ++ before ExtraPages loop at 543
//deleted 4096 from lastpageadded largest size
//added the pagefinder at 420ish, updated the largestavailsize when malloc inside page
//added membook[i] at 560