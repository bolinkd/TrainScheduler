#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAXLENGTH 200

struct train{
	char priority;
	char direction;
	char directionName[5];
	char state;    //N=new, R=Ready, L=Loading, W=Waiting, C=Crossing, D=Done
	int loadTime;
	int crossTime;
	int trainid;
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
pthread_mutex_t startMutex;
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
	Train->state = 'R';
	pthread_cond_wait(&startCond, &startMutex);
	pthread_mutex_unlock(&startMutex);
	Train->state = 'L';
	sleep(Train->loadTime);
	printf("Train %2d is ready to go %4s\n",Train->trainid,Train->directionName);
	fflush(stdout);
	while (pthread_mutex_lock(&mainTrack) != 0){
		// DO NOTHING
		Train->state = 'W';
	}
	printf("Train %2d is ON the main track going %4s\n",Train->trainid,Train->directionName);
	Train->state = 'C';
	sleep(Train->crossTime);
	Train->state = 'D';
	printf("Train %2d is OFF the main track after going %4s\n",Train->trainid,Train->directionName);
	pthread_mutex_unlock(&mainTrack);
	pthread_exit(NULL);
}


void WaitCompletion(struct train Trains[], int numTrains){
	int i=0;
	while (i != numTrains){
		if(Trains[i].state == 'R'){
			i++;
		}
	}
	pthread_cond_broadcast(&startCond);

	while (countComplete(Trains,numTrains) != numTrains){
		//Do Nothing
		//printf("%c %c %c\n",Trains[0].state,Trains[1].state,Trains[2].state);
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
	
	//init cond
	pthread_cond_init(&startCond,NULL);
	
	//init mutex
	pthread_mutex_init(&mainTrack, NULL);	
	pthread_mutex_init(&startMutex,NULL);
	pthread_mutex_lock(&startMutex);


	//Create All Trains
	for(i=0;i<numTrains;i++){
		char fileDirection;
		int fileLoadTime;
		int fileCrossTime;
		char filePriority;
		struct train *current = &Trains[i];
		fscanf(fp,"%c:%i,%i\n", &fileDirection,&fileCrossTime,&fileLoadTime);
		if(fileDirection >= 65 && fileDirection <= 90){
			filePriority = 'H';
			fileDirection = fileDirection + 'a' - 'A';
		}else{
			filePriority = 'L';
		}
		Trains[i].priority = filePriority;
		Trains[i].direction = fileDirection;
		if(Trains[i].direction == 'E'){
			strcpy(Trains[i].directionName, "East");
		}else{
			strcpy(Trains[i].directionName, "West");
		} 
		Trains[i].loadTime = fileLoadTime;
		Trains[i].crossTime = fileCrossTime;
		Trains[i].trainid = i;
		Trains[i].state = 'N';
		int rc = pthread_create(&pthreads[i], NULL, CreateTrain, (void *)current);
		if (rc){
        		printf("ERROR; return code from pthread_create() is %d\n", (int)rc);
        		exit(-1);
    		}
	}
	
	WaitCompletion(Trains,numTrains);
	for(i=0;i<numTrains;i++){
		pthread_join(pthreads[i],&Status);
	}
	fflush(stdout);
	pthread_mutex_destroy(&mainTrack);
	pthread_cond_destroy(&startCond);
	return (0);
}
