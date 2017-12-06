#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <wait.h>
#include <sys/mman.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <ctype.h>
#include <pthread.h>
#include "sorter_thread.h"


int main(int argc, char** argv)
{
	printf("Initial PID: %d\n", getpid());
	printf("TIDs of all child threads: ");
	//fflush(stdout);
	totalProcesses = 1;
	commaPlaceCount = 0;
	char* toSort = NULL;
	char outputPath[2999];
	char* holdMyString;
	char *letMeIn;
	char extName[955];
	root = NULL;
	bIsFirst = 1;
	memset(outputPath,'\0',2999);
	letMeIn = ".";
	holdMyString = ".";

	

	if (argc < 3)
	{
		//error
		exit(0);
	}

	int arg = 1;

	while (arg < argc)
	{
		if (argv[arg][1] == 'd')
		{
			if ((arg+1) < argc)
			{
				if (argv[arg+1][0] != '-')
				{
					letMeIn = argv[arg+1];
				}
			}
		}
		else if (argv[arg][1] == 'o')
		{
			if ((arg+1) < argc)
			{
				if (argv[arg+1][0] != '-')
				{
					holdMyString = argv[arg+1];
				}
			}
		}
		else if (argv[arg][1] == 'c')
		{
			if ((arg+1) < argc)
			{
				if (argv[arg+1][0] != '-')
				{
					toSort = argv[arg+1];
				}
			}
		}
		arg += 1;
	}

	if (toSort == NULL)
	{
		//error
		exit(0);
	}
	memset(extName,'\0',955);
	int toSortLen = getMyLength(toSort);
	strncat(extName,"AllFiles-sorted-",17);
	strncat(extName,toSort,toSortLen+1);
	strncat(extName,".csv",5);
	traverseDirectory(letMeIn,toSort,0);
	
	int holdMyStringLen = getMyLength(holdMyString);
	strncat(outputPath,holdMyString,holdMyStringLen+1);
	if(checkIfSlash(outputPath) == 0)
	{
		strncat(outputPath,"/",2);
	}
	int extNameLen = getMyLength(extName);
	strncat(outputPath,extName,extNameLen+1);
	
	newFP=fopen(outputPath,"w");
	//printSortedData(NULL);
	printTree(root);
	fclose(newFP);

	printf("\n");
	printf("Total number of threads: %d\n",totalProcesses);
	
	pthread_mutex_destroy(&insertionLock);
	exit(0);
}
void sortingFunc(char * fileName, char* toSort2, char *filePath)
{
	//int i = 0;
	int lenComma = getMyLength(toSort2);
	char toSort[955];
	memset(toSort,'\0',955);
	
	strncat(toSort,toSort2,lenComma+1);
	

	char ingoing[2999];
	memset(ingoing,'\0',2999);
	int filePathLen = getMyLength(filePath);
	strncat(ingoing,filePath,filePathLen+1);
	if(ingoing[getMyLength(ingoing)-1] != '/')
	{
		strncat(ingoing,"/",2);
	}
	int fileNameLen = getMyLength(fileName);
	strncat(ingoing,fileName,fileNameLen+1);
	//printf("filePath: %s\n",ingoing);
        FILE *fp;
        fp=fopen(ingoing,"r");
       char inputFile[955];
       memset(inputFile,'\0',955);
       strncat(inputFile,fileName,fileNameLen+1);
       char outputFile[955];
       memset(outputFile,'\0',955);
       int h=0;
       //int len= strlen(fileName);
	//printf("inputFile: %s\n",inputFile);
       while(h<fileNameLen)
       {
        //printf("this is I -->%d\n",i);
        if(inputFile[h]=='.'&&inputFile[h+1]=='c'&&inputFile[h+2]=='s'&&inputFile[h+3]=='v')
           {
              inputFile[h]='\0';
              inputFile[h+1]='\0';
              inputFile[h+2]='\0';
              inputFile[h+3]='\0';
              break;}
         h++;
       }
	int inputFileLen = getMyLength(inputFile);
	int outputFileLen = getMyLength(outputFile);
	strncat(outputFile,inputFile,inputFileLen+1);
	strncat(outputFile,outputFile,outputFileLen);
	//strcat(outputFile,);
        
       // printf("this is output %s\n",outputFile);
	//strcat(extName,toSort);
        char curr = '\0';
	//char * toSort = "Cat2";
	char*ar = (char*) malloc(sizeof(char)*500);
	memset(ar,'\0',sizeof(char)*500);
	
	int c1=0;
	int bquote=0;
	int bspace=0;
	int bIsCategory=1;
	int bTopOfRow = 1;
	int row = 0;
	int avoidSegFaultCommaCount = 0;
	int avoidSegFaultCommaCheck = 0;
	struct Rows **data = (struct Rows**)malloc(sizeof(struct Rows*)*1);
	Category *catHead = (struct Category*) malloc(sizeof(struct Category));
	catHead->next = NULL;
	catHead->catName = (char*)malloc(2 * sizeof(char));
	catHead->itemName = (char*)malloc(2 * 2 * sizeof(char));
	Category *catPtr = catHead;
	Category *pointer = NULL;
	struct Rows* entry = NULL;
	
	char thisShouldBeAQuote = '\0';
	int bIsEmpty = 0;
	bIsEmpty += 1;
	bIsEmpty -= 1;

	while(fscanf(fp,"%c",&curr)!=EOF)
	{
		bIsEmpty = 1;
		if (avoidSegFaultCommaCheck >= avoidSegFaultCommaCount && curr != '\n' && bIsCategory != 1)
		{
			//printf("Error: Wrong CSV Format for file: %s\t[%d]\n",fileName,getpid());
			//exit(totalProcesses);
		}		
		
		if (bIsCategory == 0 && catPtr == catHead && bTopOfRow == 1)
		{
			//data = (struct Rows**)realloc(data, sizeof(struct Rows*)*(row+1));
			entry = (struct Rows*)malloc(sizeof(struct Rows));
			//data[row] = entry;
			entry->cat = malloc(sizeof(struct Category));
			pointer = entry->cat;
			bTopOfRow = 0;
		}
		else if (bIsCategory == 1 && bTopOfRow == 1)
		{
			//printf("$$$$$$$$$$$\n");
			bTopOfRow = 0;
			//struct Rows* entry = 
			data[0] = (struct Rows*)malloc(sizeof(struct Rows)*1);
			data[0]->cat = malloc(sizeof(struct Category));
			pointer = data[0]->cat;
		}	

		if(curr=='"'&& bquote==0)
		{
			bquote=1;
			thisShouldBeAQuote = '"';
			continue;
		}
		else if (bquote==1 && thisShouldBeAQuote == '"')
		{
			if(curr != ' ')
			{
				bspace = 1;
			}
			ar[c1]=curr;
			thisShouldBeAQuote = '\0';
			c1++;
		}
		else if(curr=='"'&& bquote==1)
		{
			bquote=0;
			continue;
		}
		else if((curr==',' || curr == EOF || curr == '\n') && bquote==0)
		{
			if (ar[0] == '\0' && bIsCategory == 1)
			{
				//printf("Error: Wrong CSV Format\t[%d]\n",getpid());
				//exit(totalProcesses);
			}

			if (curr == '\n')
			{
				ar[c1-1] = '\0';
				c1 = getMyLength(ar);
				
				if (bIsCategory == 1)
				{					
					avoidSegFaultCommaCount++;
					catPtr->catName = (char *)realloc(catPtr->catName, c1);
					char *ar2 = malloc((c1+1) * sizeof(char));
					ar2 = setDuplicate(ar2,ar,c1);
					ar2[c1] = '\0';
					catPtr->catName = ar2;
					catPtr->next = NULL;
					//iGotYourIP(toSort, catHead);
					bIsCategory = 0;
				}
				else
				{
					//data[row]->cat = malloc(sizeof(struct Category));
					pointer->catName = malloc(100 * sizeof(char));
					pointer->itemName = malloc((c1+1) * sizeof(char));
					char *ar2 = malloc((c1+1) * sizeof(char));
					ar2 = setDuplicate(ar2,ar,c1);
					ar2[c1] = '\0';
					pointer->itemName = ar2;
					pointer->catName = catPtr->catName;
					pointer->next = NULL;
					pthread_mutex_lock(&insertionLock);
					root = insert(root,toSort,entry);
					pthread_mutex_unlock(&insertionLock);
					row += 1;
				}
				catPtr = catHead;
				avoidSegFaultCommaCheck++;
				
				if (avoidSegFaultCommaCheck < avoidSegFaultCommaCount)
				{
					//printf("Error: Wrong CSV Format\t[%d]\n",getpid());
					//exit(totalProcesses);
				}
				
				memset(ar,'\0',sizeof(char)*500);
				c1=0;
				bTopOfRow = 1;
				bIsCategory = 0;
				bspace=0;
				avoidSegFaultCommaCheck = 0;
				continue;
			}
			else if(bspace==1)
			{
				if (c1 > 1)
				{
					if (ar[c1-1] == ' ')
					{
						ar[c1-1] = '\0';
					}
				}
			
				if (bIsCategory == 1)
				{
					if (ar[0] == '\0')
					{
						//printf("Error: Wrong CSV Format\t[%d]\n",getpid());
						//exit(totalProcesses);
					}
					avoidSegFaultCommaCount++;
					catPtr->catName = (char *)realloc(catPtr->catName, c1);
					char *ar2 = malloc((c1+1) * sizeof(char));
					ar2 = setDuplicate(ar2,ar,c1);
					ar2[c1] = '\0';
					catPtr->catName = ar2;
					catPtr->next = (struct Category*) malloc(sizeof(struct Category));
					//printf("%d %s",c1,catPtr->catName);
				}	
				else if (curr == EOF)
				{
					break;
				}
				else
				{
					pointer->catName = (char *)malloc(100 * sizeof(char));
					pointer->itemName = (char *)malloc((c1+1) * sizeof(char));
					char *ar2 = (char *)malloc((c1+1) * sizeof(char));
					ar2 = setDuplicate(ar2,ar,c1);
					ar2[c1] = '\0';
					pointer->itemName = ar2;
					pointer->catName = catPtr->catName;
					pointer->next = (struct Category*) malloc(sizeof(struct Category));
					pointer = pointer->next;
				}
			}
			else if (curr == ',' && c1 == 0) //For empty entries
			{
				pointer->catName = (char *)malloc(100 * sizeof(char));
				pointer->itemName = (char *)malloc(1 * sizeof(char));
				pointer->itemName = "";
				pointer->catName = catPtr->catName;
				pointer->next = (struct Category*) malloc(sizeof(struct Category));
				pointer = pointer->next;
			}
			
			if (catPtr == catHead && bIsCategory == 1)
			{
				catHead = catPtr;
				catPtr = catPtr->next;
				catHead->next = catPtr;	
			}
			else if (bIsCategory == 0)
			{
				catPtr = catPtr->next;
			}
			else
			{
				catPtr = catPtr->next;
			}
			avoidSegFaultCommaCheck++;
			//printf("%s\n",ar);
			//root = insert(start,toSort,entry);
			memset(ar,'\0',sizeof(char)*500);
			c1=0;
			bspace=0;
			continue;
		}
		else
		{
			if(curr != ' ')
			{
				bspace=1;
				ar[c1]=curr;
				c1++;
			}
			else
			{
				int temp = c1 - 1;
				if (temp >= 0)
				{
					if (ar[temp] == ' ')
					{
						continue;
					}
					else
					{
						ar[c1]=curr;
						c1++;
					}
				}
				else
				{
					continue;
				}
			}
		}
	}

	if (bNeverCat == 0)
	{
		bNeverCat = 1;
		catHead2 = catHead;
	}

	/*if (bIsEmpty == 0)
	{
		//_exit(totalProcesses);
	}*/

	free(ar);
	free(data[0]->cat);
	free(data[0]);
	free(data);
	
	fclose(fp);
	return ;
}

//Copies a string to avoid errors for insertion
char* setDuplicate(char* newOne, char* original, int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		newOne[i] = original[i];
	}
	return newOne;
}

//Prints the final output of the data after sorting
void printSortedData(struct Rows *list)
{
//printf("came into print sorted\n");
	char outgoing[2999];
	memset(outgoing,'\0',2999);

	Category *p = NULL;

	if (bIsFirst == 1)
	{
		p = catHead2;
		bIsFirst = 0;
		int bIsFirstRow = 1;

		while (p != NULL)
		{
			if (bIsFirstRow == 1)
			{
				int bIsNameWithComma = isNameWithComma(p->catName);
				if (p->next == NULL)
				{
					if (bIsNameWithComma == 1)
					{
						fprintf(newFP,"\"%s\"",p->catName);
						//printf("\"1%s\"\n",p->catName);
					}
					else
					{
						fprintf(newFP,"%s",p->catName);
					}
					//p = data[0]->cat;
					bIsFirstRow = 0;
					//fprintf(newFP,"\n");
					continue;
				}
				else
				{
					if (bIsNameWithComma == 1)
					{
						fprintf(newFP,"\"%s\",",p->catName);
					}
					else
					{
						fprintf(newFP,"%s,",p->catName);
					}
				}
			}
			p = p->next;
		}
	}

	if (bIsFirst == 0)
	{
		//printf("Writing the non-category entries\n");
		fprintf(newFP,"\n");
		p = list->cat;
		while (p != NULL)
		{
			int bIsNameWithComma = isNameWithComma(p->itemName);
			if (p->next == NULL)
			{
				if (bIsNameWithComma == 1)
				{
					fprintf(newFP,"\"%s\"",p->itemName);
					//printf("\"2%s\"\n",p->itemName);
				}
				else
				{
					fprintf(newFP,"%s",p->itemName);
					//printf("\"3%s\"\n",p->itemName);
				}
				//p = data[0]->cat;
				//bIsFirstRow = 0;
				//fprintf(newFP,"\n");
				//continue;
			}
			else
			{
				if (bIsNameWithComma == 1)
				{
					fprintf(newFP,"\"%s\",",p->itemName);
					//printf("\"4%s\"\n",p->itemName);
				}
				else
				{
					fprintf(newFP,"%s,",p->itemName);
					//printf("\"5%s\"\n",p->itemName);
				}
			}

			p = p->next;
		}
	}
}

int isNameWithComma(char *stringToSearch)
{
	int i = 0;
	while (stringToSearch[i] != '\0')
	{
		if (stringToSearch[i] == ',')
		{
			return 1;
		}
		i++;
	}
	return 0;
}


//Checks if the user-entered category to sort by is a category in the CSV file
void iGotYourIP(char *toSort, struct Category *traveler)
{
	int areWeGood = 0;
	const char *holdString = traveler->catName;

	while (traveler != NULL)
	{
		holdString = traveler->catName;
		if (strcasecmp(holdString,toSort) == 0)
		{
			areWeGood = 1;
			break;
		}
		traveler = traveler->next;
	}

	if (areWeGood == 1)
	{
		//printf("\n%s\n",holdString);
		return;
	}

	pthread_exit(0);
	/*else
	{
	}*/
}

//Recursively travels through directory of files
void traverseDirectory(char *currItem, char* toSort, int threadsSpinned)
{
	DIR *directoryPtr;
	struct dirent *curr;
	pthread_t *TID_I_O = (pthread_t*)malloc(sizeof(pthread_t)*1);

	if ((directoryPtr = opendir(currItem)) == NULL)
	{
		//printf("Directory not found\t\n");
        	return;
	}

	while ((curr = readdir(directoryPtr)) != NULL)
	{
		if (curr->d_type == DT_DIR) 
		{
			if (strcmp(curr->d_name, ".") == 0 || strcmp(curr->d_name, "..") == 0)
			{
				continue;
			}

			int currLen = getMyLength(curr->d_name);

			if (currLen > 1)
			{
				if (curr->d_name[0] == '.' && curr->d_name[1] != '/')
				{
					continue;
				}
			}

			totalProcesses++;
			//--------------------------->>>>>>>>printf("Folder Fork on ---> %s\t\n",curr->d_name);
			pthread_t myTID; //= pthread_self();
			//printf("Folder Name: %s\t[%d]\n",entry->d_name,getpid());
			char toConcat[2999];
			strcpy(toConcat,currItem);
			
			if(checkIfSlash(currItem) == 1)
			{
				strncat(toConcat,curr->d_name,currLen+1);
				threadsSpinned++;
				
				TID_I_O = (pthread_t*)realloc(TID_I_O,sizeof(pthread_t)*threadsSpinned);

				hubData *threadHub1 = (struct hubData*)malloc(sizeof(struct hubData));
					int conCatLen = getMyLength(toConcat);
					threadHub1->name=malloc(sizeof(char)*(conCatLen+1));
					threadHub1->name=setDuplicate(threadHub1->name, toConcat,conCatLen);
					threadHub1->toSort=toSort;
					threadHub1->sortname=NULL;
					threadHub1->sorttoSort=NULL;
					threadHub1->sortcurrItem=NULL;
					threadHub1->functiontype=malloc(sizeof(int));//if default:0 traverseDirec:1 sortingFunc:2
					*threadHub1->functiontype=1;	
				pthread_create(&myTID, NULL, hubCenter, threadHub1);

				TID_I_O[threadsSpinned-1] = myTID;
			}
			else
			{
				strncat(toConcat,"/",2);
				strncat(toConcat,curr->d_name,currLen+1);
				threadsSpinned++;
				TID_I_O = (pthread_t*)realloc(TID_I_O,sizeof(pthread_t)*threadsSpinned);
				hubData *threadHub2 = (struct hubData*)malloc(sizeof(struct hubData));
					int conCatLen = getMyLength(toConcat);
					threadHub2->name=malloc(sizeof(char)*(conCatLen+1));
					threadHub2->name=setDuplicate(threadHub2->name, toConcat,conCatLen+1);
					threadHub2->toSort=toSort;
					threadHub2->sortname=NULL;
					threadHub2->sorttoSort=NULL;
					threadHub2->sortcurrItem=NULL;
					threadHub2->functiontype=malloc(sizeof(int));
					*threadHub2->functiontype=1;//if default:0 traverseDirec:1 sortingFunc:2
				pthread_create(&myTID, NULL, hubCenter, threadHub2);

				TID_I_O[threadsSpinned-1] = myTID;
			}
		} 
		else
		{
			//printf("File Name: %s\n",curr->d_name);
			if (checkIfCSV(curr->d_name) == 1)
			{
				if(checkIfAlreadySorted(curr->d_name)	== 0)
				{
					totalProcesses++;
					//printf("File Fork on ---> %s\t\n",curr->d_name);
					pthread_t myTID;// = pthread_self();
					threadsSpinned++;
					TID_I_O = (pthread_t*)realloc(TID_I_O,sizeof(pthread_t)*threadsSpinned);
					hubData *threadHub3= (struct hubData*)malloc(sizeof(struct hubData));
				    		threadHub3->name=NULL;
						threadHub3->toSort=NULL;
						int sortnameLen = getMyLength(curr->d_name);
						threadHub3->sortname=malloc(sizeof(char)*(sortnameLen+1));
						threadHub3->sortname=setDuplicate(threadHub3->sortname,curr->d_name,sortnameLen+1);
						threadHub3->sorttoSort=toSort;
						int currItemLen = getMyLength(currItem);
						threadHub3->sortcurrItem=malloc(sizeof(char)*(currItemLen+1));
						threadHub3->sortcurrItem=setDuplicate(threadHub3->sortcurrItem,currItem,currItemLen+1);
						threadHub3->functiontype=malloc(sizeof(int));
						*threadHub3->functiontype=2;//if default:0 traverseDirec:1 sortingFunc:2
					pthread_create(&myTID, NULL, hubCenter, threadHub3);
					TID_I_O[threadsSpinned-1] = myTID;
					
				}
				/*else
				{
					//printf("Nope, I am not going to sort %s again!\n",curr->d_name);
				}*/
			}
		}
    	}

	if (threadsSpinned >= 1)
	{
		int a = 0;
		while (a < threadsSpinned)
		{
			pthread_join(TID_I_O[a], NULL);
			a++;
		}
	}

	free(TID_I_O);

	closedir(directoryPtr);
}

//Checks to see if the current pathway has a slash or not
int checkIfSlash(char *name)
{
	int len = getMyLength(name);

	if (name[len-1] == '/')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int checkIfCSV(char *fileName)
{
	int len = getMyLength(fileName);

	if (fileName[len-4] == '.' && (fileName[len-3] == 'c' || fileName[len-3] == 'C') && (fileName[len-2] == 's' || fileName[len-2] == 'S') && (fileName[len-1] == 'v' || fileName[len-1] == 'V'))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int checkIfAlreadySorted(char *fileName)
{
	char *alSorted = "-sorted";
	int lenf = getMyLength(fileName);
	int lene = getMyLength(alSorted);
	int i = 0;
	int j = 0;

	for (i = 0; i < lenf; i++)
	{
		if (fileName[i] == alSorted[0])
		{
			for (j=0; j < lene; j++)
			{
				if (fileName[i+j] == alSorted[j])
				{
					if (j+1 == lene)
					{
						return 1;
					}
					continue;
				}
				else
				{
					break;
				}
			}
		}
	}
	
	return 0;
}

//Travels the row's link list to the category that we want to compare
char *catTravel(const char* catToSort, struct Category *traveler)
{
	const char *holdString = traveler->catName;

	while (strcasecmp(holdString,catToSort) != 0)
	{
		if (traveler->next != NULL)
		{
			traveler = traveler->next;
			holdString = traveler->catName;
		}
	}

	return traveler->itemName;
}

//Recursively travels through Binary Search Tree and 
Node* insert(Node *start, char *toSort, struct Rows* entry)
{
	  struct Node *temp;
	  char *tempCati = NULL;
	  char *tempCatj = NULL;

	  if(start==NULL)
	  {
		//printf("----END----\n");
		//pthread_mutex_lock(&insertionLock);
	      temp=(struct Node*) malloc(sizeof(struct Node));
	      temp->list = entry;
	      temp->bIsSeen = 0;
	      temp->left=NULL;
	      temp->right=NULL;
		//pthread_mutex_unlock(&insertionLock);
	      
		//printf("%s\n",temp->list->cat->next->itemName);
	      return temp;

	  }

	  tempCati = catTravel(toSort,start->list->cat);
	  tempCatj = catTravel(toSort,entry->cat);
	  //printf("1: %s\t2: %s\n",tempCati,tempCatj);

	  if (strcasecmp(tempCatj,tempCati) < 0)
	  {
		//printf("I--LEFT\n");
	      start->left=insert(start->left,toSort,entry);
	  }
	  else if (strcasecmp(tempCatj,tempCati) > 0)
	  {
		//printf("I--RIGHT\n");
	      start->right=insert(start->right,toSort,entry);
	  }
	  else
	  {
		//printf("I--RIGHT(DUPLICATE)\n");
	      start->right=insert(start->right,toSort,entry);
	  }
	  return start;
}

void printTree(Node *start)
{//printf("printing tree...\n");
	
	if (start == NULL)
	{
		//printf("What if...\n");
		return;
	}

	if (start->left == NULL)
	{
		if (start->bIsSeen == 0)
		{
			//printf("PRINT\n");
			printSortedData(start->list);
			start->bIsSeen = 1;
		}
	}

	if (start->left != NULL)
	{
		//printf("LEFT\n");
		printTree(start->left);

		if (start->left->bIsSeen == 1)
		{
			if (start->bIsSeen == 0)
			{
				//printf("PRINT?\n");
				printSortedData(start->list);
				start->bIsSeen = 1;
			}
		}
	}
	
	if (start->right != NULL)
	{
		//printf("RIGHT\n");
		printTree(start->right);
	}

	//printf("UP\n");
	return;
}

//Copies contents of one char array into another
char *fillItUp(char *oldStr)
{
	char newChar[2999];
	memset(newChar,'\0',2999);
	int len = getMyLength(oldStr);
	int i = 0;

	for (i = 0; i < len; i++)
	{
		newChar[i] = oldStr[i];
	}

	char *newStr = newChar;

	return newStr;
}

int getMyLength(char *dontSegFaultMe)
{
	char *dontSegFaultMe2 = dontSegFaultMe;
	
	while (*dontSegFaultMe != '\0')
	{
		dontSegFaultMe++;		
	}
	return dontSegFaultMe - dontSegFaultMe2;
}

//the function that will route the threads to whatever function call they needed to make, for all your threading needs
void * hubCenter(void *hubb)
{
	hubData *key=hubb;
	printf("%lu,",pthread_self());
	int func = *key->functiontype;
	//free(key->functiontype);
	//fflush(stdout);

	switch(func)
	{
		case 1:
		{
			int nameLen = getMyLength(key->name);
			int sortLen = getMyLength(key->toSort);
			char *name=malloc(sizeof(char)*(nameLen+1));
			char *sort=malloc(sizeof(char)*(sortLen+1));
			name=setDuplicate(name,key->name,nameLen);
			sort=setDuplicate(sort,key->toSort,sortLen);
			traverseDirectory(name,sort,0);
			free(sort);
			free(name);
			free(key->functiontype);
			free(key->name);
			free(hubb);
			pthread_exit(0);
			break;
		}
		case 2:
		{
			int sortnameLen = getMyLength(key->sortname);
			int sorttoSortLen = getMyLength(key->sorttoSort);
			int sortcurrItemLen = getMyLength(key->sortcurrItem);
			char *sortname=malloc(sizeof(char)*(sortnameLen+1));
			char *sorttoSort=malloc(sizeof(char)*(sorttoSortLen+1));
			char *sortcurrItem=malloc(sizeof(char)*(sortcurrItemLen+1));
			sortname=setDuplicate(sortname,key->sortname,sortnameLen);
			sorttoSort=setDuplicate(sorttoSort,key->sorttoSort,sorttoSortLen);
			sortcurrItem=setDuplicate(sortcurrItem,key->sortcurrItem,sortcurrItemLen);
			sortingFunc(sortname,sorttoSort,sortcurrItem);
			free(sortcurrItem);
			free(sorttoSort);
			free(sortname);
			free(key->functiontype);
			free(key->sortcurrItem);
			free(key->sortname);
			free(hubb);
			pthread_exit(0);
			break;
		}
	}
	return 0;	 
}

