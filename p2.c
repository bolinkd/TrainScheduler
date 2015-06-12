#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAXLENGTH 200

struct train{
	char priority;
	char direction;
	int loadTime;
	int waitTime;
	int trainid;
	int isComplete;
	pthread_mutex_t startMutex;
};

/*************
* Struct to hold Processes
* Includes: 
*************/
struct Node{
	struct Node *next;
	struct Node *prev;
	struct train *Train;
};


struct Master{
	struct Node *HE;
	struct Node *HW;
	struct Node *LE;
	struct Node *LW;
};

/*************
* Initialize 
* 
*************/
struct Master *Head = NULL;

pthread_mutex_t mainTrack;
pthread_cond_t startCond;
pthread_attr_t attr;

void *CreateTrain(void *tempTrain){
	struct train * Train = tempTrain;
	printf("Hello World! It's me, train #%d!\n", Train->trainid);
	fflush(stdout);
	pthread_cond_wait(&startCond, &Train->startMutex);
	pthread_mutex_unlock(&Train->startMutex);
	printf("I am Loading: %d\n",Train->trainid);
	sleep(Train->loadTime);
	while (pthread_mutex_trylock(&mainTrack) == 0){
		// DO NOTHING
		printf("I am Waiting: %d\n",Train->trainid);
		fflush(stdout);
	}
	printf("I am Completing: %d\n",Train->trainid);
	fflush(stdout);
	sleep(Train->waitTime);
	printf("I am Complete: %d\n",Train->trainid);
	fflush(stdout);
	Train->isComplete = 1;
	pthread_mutex_unlock(&mainTrack);
	pthread_exit(NULL);
}

void printTrain(struct train toPrint, int trainid){
	printf("Train %i is headed %c with priority %c after loading for %i and travels for %i\n",trainid,toPrint.direction,toPrint.priority,toPrint.loadTime,toPrint.waitTime);
	
}

void printTrains(struct train Trains[], int numTrains){
	int i;
	for(i=0;i<numTrains;i++){
		printTrain(Trains[i], i);
	}
}

void WaitCompletion(struct train Trains[], int numTrains){
	int i;
	sleep(10);
		
	pthread_cond_broadcast(&startCond);
	for(i=0;i<numTrains;i++){
		if(Trains[i].isComplete == 0){
			i--;	
		}
	}
	return;
}

int main(int argc, char *argv[]){
	int i;
	void *Status;
	if(argc != 3){
		printf("Two Arguments Required: use - (File Path} {Number Of Trains}");
		return (1);
	}
	
	FILE *fp = fopen(argv[1], "r");
	if(fp == 0){
		printf("Could not open file\n");
		return (1);
	}
	int numTrains = atoi(argv[2]);

	struct train Trains[numTrains];
	pthread_t pthreads[numTrains];
	pthread_mutex_t startMutexes[numTrains];

	//init attr
	pthread_attr_init(&attr);
	
	//init cond
	pthread_cond_init(&startCond,NULL);
	
	//init mutex
	pthread_mutex_init(&mainTrack, NULL);	
	

	//Create All Trains
	for(i=0;i<numTrains;i++){
		char fileDirection;
		int fileLoadTime;
		int fileWaitTime;
		char filePriority;
		struct train *current = &Trains[i];
		fscanf(fp,"%c:%i,%i\n", &fileDirection,&fileWaitTime,&fileLoadTime);
		if(fileDirection >= 65 && fileDirection <= 90){
			filePriority = 'H';
			fileDirection = fileDirection + 'a' - 'A';
		}else{
			filePriority = 'L';
		}
		Trains[i].priority = filePriority;
		Trains[i].direction = fileDirection;
		Trains[i].loadTime = fileLoadTime;
		Trains[i].waitTime = fileWaitTime;
		Trains[i].trainid = i;
		Trains[i].isComplete = 0;
		pthread_mutex_init(&(startMutexes[i]),NULL);
		int rc = pthread_create(&pthreads[i], NULL, CreateTrain, (void *)current);
		if (rc){
        		printf("ERROR; return code from pthread_create() is %d\n", (int)rc);
        		exit(-1);
    		}
	}
	
	printTrains(Trains,numTrains);
	WaitCompletion(Trains,numTrains);
	for(i=0;i<numTrains;i++){
		pthread_join(pthreads[i],&Status);
	}
	fflush(stdout);
	pthread_mutex_destroy(&mainTrack);
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&startCond);
	return (0);
}
