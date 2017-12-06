typedef struct Category
{
	struct Category *next;
	char *catName;
	char *itemName;
}Category;

typedef struct Rows
{
	struct Category *cat;
}*Rows;

typedef struct Node
{
  struct Rows *list;
  struct Node *left,*right;
  int bIsSeen;
}Node;

typedef struct hubData
{
	char*name;
	char*toSort;
	char*sortname;
	char*sorttoSort;
	char*sortcurrItem;
	char*sortfileOutput;
	int* functiontype;           
}hubData;

Node *root;
int totalProcesses;
int commaPlaceCount;
int bIsFirst;
int bNeverCat=0;
FILE *newFP;
struct Category* catHead2;
pthread_mutex_t insertionLock;


char *catTravel(const char* catToSort, struct Category *traveler);
char* setDuplicate(char* newOne, char* original, int size);
void printSortedData(struct Rows *list);
int isNameWithComma(char *stringToSearch);
void iGotYourIP(char *toSort, struct Category *traveler);
int checkIfSlash(char *name);
int checkIfCSV(char *fileName);
void traverseDirectory(char *name, char *toSort, int threadsSpinned);
void sortingFunc(char *fileName, char *toSort, char *filePath);
int checkIfAlreadySorted(char *fileName);
int hasCommasForSortCat(char *toSort);
char *fillItUp(char *oldStr);
Node* insert(Node *start, char *toSort,struct Rows* entry);
void printTree(Node *start);
int getMyLength(char *dontSegFaultMe);
void *hubCenter(void *hub);
