#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAXLENGTH 200

struct train{
	char priority;
	char direction;
	char state;
	//N=new, R=Ready, L=Loading, W=Waiting, C=Crossing, D=Done
	int loadTime;
	int waitTime;
	int trainid;
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

int countComplete(struct train Trains[],int numTrains){
	int complete = 0;
	int i;
	for(i=0;i<numTrains;i++){
		if(Trains[i].state == 'D'){
			complete++;
		}
	}
	return complete;
}

void *CreateTrain(void *tempTrain){
	struct train * Train = tempTrain;
	sleep(5);
	Train->state = 'R';
	pthread_cond_wait(&startCond, &Train->startMutex);
	pthread_mutex_unlock(&Train->startMutex);
	Train->state = 'L';
	sleep(Train->loadTime);
	while (pthread_mutex_lock(&mainTrack) != 0){
		// DO NOTHING
		Train->state = 'W';
	}
	Train->state = 'C';
	sleep(Train->waitTime);
	Train->state = 'D';
	pthread_mutex_unlock(&mainTrack);
	pthread_exit(NULL);
}

void printTrain(struct train toPrint, int trainid){
	if(toPrint.state == 'N'){
		printf("Train %i is created\n",trainid);
	}else if(toPrint.state == 'R'){
		printf("Train %i is headed %c with priority %c after loading for %i and travels for %i\n",trainid,toPrint.direction,toPrint.priority,toPrint.loadTime,toPrint.waitTime);
	}else if(toPrint.state == 'L'){
		printf("Train %i is loading for %i and ready to go %c with priority %c and travels for %i\n",trainid,toPrint.loadTime,toPrint.direction,toPrint.priority,toPrint.waitTime);
	}else if(toPrint.state == 'C'){
		printf("Train %i is crossing the track for %i\n",trainid,toPrint.waitTime);
	}else if(toPrint.state == 'D'){
		printf("Train %i has crossed the track\n",trainid);
	}else if(toPrint.state == 'W'){
		printf("Train %i is waiting to cross the track\n",trainid);	
	}else{
		printf("Train %i has state %c and is lost\n",trainid,toPrint.state);
	}
	fflush(stdout);
	
}

void printTrains(struct train Trains[], int numTrains){
	int i;
	printf("------------------------------------------------------------------------\n");
	for(i=0;i<numTrains;i++){
		printTrain(Trains[i], i);
	}
	printf("------------------------------------------------------------------------\n");
	fflush(stdout);
}

void WaitCompletion(struct train Trains[], int numTrains){
	int i=0;
	while (i != numTrains){
		if(Trains[i].state = 'R')
			i++;
	}
	printf("All trains ready to go\n");
	sleep(5);
	pthread_cond_broadcast(&startCond);
	while (countComplete(Trains,numTrains) != numTrains){
		printTrains(Trains,numTrains);
		sleep(2);
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
		Trains[i].state = 'N';
		int rc = pthread_create(&pthreads[i], NULL, CreateTrain, (void *)current);
		if (rc){
        		printf("ERROR; return code from pthread_create() is %d\n", (int)rc);
        		exit(-1);
    		}
	}
	
	printTrains(Trains,numTrains);
	WaitCompletion(Trains,numTrains);
	printTrains(Trains,numTrains);
	for(i=0;i<numTrains;i++){
		pthread_join(pthreads[i],&Status);
	}
	fflush(stdout);
	pthread_mutex_destroy(&mainTrack);
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&startCond);
	return (0);
}
