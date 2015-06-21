#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAXLENGTH 200
#define CONVERT 100000

struct train{
	char priority;
	char direction;
	char directionName[5];
	char state;    //N=new, S=Start, L=Loading, W=Waiting, R=Ready to Cross C=Crossing, D=Done
	int loadTime;
	int crossTime;
	int trainid;
};

struct timespec start,curr;



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
struct Master *MasterHead;
struct Master *MasterTail;

pthread_mutex_t mainTrack;
pthread_mutex_t moveTrainsMutex;
pthread_mutex_t masterList;

pthread_cond_t trainMoved;
pthread_cond_t signalMove;

pthread_barrier_t startBarrier;
pthread_barrier_t endBarrier;


void pushTrain(struct train *Train){
	char priority = Train->priority;
	char direction = Train->direction;
	struct Node * newNode = malloc(sizeof(struct Node));
	newNode->Train = Train;
	if(priority == 'H' && direction == 'E'){
		if(MasterTail->HE == NULL){
			MasterHead->HE = (struct Node *)newNode;
			MasterTail->HE = (struct Node *)newNode;
		}else{
			struct Node * temp = MasterTail->HE;
			temp->next = (struct Node *)newNode;
			MasterTail->HE = (struct Node *)newNode; 
		}
	}else if(priority == 'H' && direction == 'W'){
		if(MasterTail->HW == NULL){
			MasterHead->HW = (struct Node *)newNode;
			MasterTail->HW = (struct Node *)newNode;
		}else{
			struct Node * temp = MasterTail->HW;
			temp->next = (struct Node *)newNode;
			MasterTail->HW = (struct Node *)newNode; 
		}
	}else if(priority == 'L' && direction == 'E'){
		if(MasterTail->LE == NULL){
			MasterHead->LE = (struct Node *)newNode;
			MasterTail->LE = (struct Node *)newNode;
		}else{
			struct Node * temp = MasterTail->LE;
			temp->next = (struct Node *)newNode;
			MasterTail->LE = (struct Node *)newNode; 
		}
	}else if(priority == 'L' && direction == 'W'){
		if(MasterTail->LW == NULL){
			MasterHead->LW = (struct Node *)newNode;
			MasterTail->LW = (struct Node *)newNode;
		}else{
			struct Node * temp = MasterTail->LW;
			temp->next = (struct Node *)newNode;
			MasterTail->LW = (struct Node *)newNode; 
		}
	}else{
		printf("Train ERROR %i : p = %c , d = %c\n",Train->trainid,priority, direction);
	}
}

void popTrain(struct train *Train){
	char priority = Train->priority;
	char direction = Train->direction;
	struct Node * tofree = NULL;
	if(priority == 'H' && direction == 'E'){
		if(MasterHead->HE->next == NULL){
			tofree = (struct Node *)MasterHead->HE;
			MasterHead->HE = NULL;
			MasterTail->HE = NULL;
		}else{
			tofree = (struct Node *)MasterHead->HE;
			MasterHead->HE = (struct Node *)MasterHead->HE->next; 
		}
	}else if(priority == 'H' && direction == 'W'){
		if(MasterHead->HW->next == NULL){
			tofree = (struct Node *)MasterHead->HE;
			MasterHead->HW = NULL;
			MasterTail->HW = NULL;
		}else{
			tofree = (struct Node *)MasterHead->HW;
			MasterHead->HW = (struct Node *)MasterHead->HW->next; 
		}
	}else if(priority == 'L' && direction == 'E'){
		if(MasterHead->LE->next == NULL){
			tofree = MasterHead->LE;
			MasterHead->LE = NULL;
			MasterTail->LE = NULL;
		}else{
			tofree = MasterHead->LE;
			MasterHead->LE = MasterHead->LE->next; 
		}
	}else if(priority == 'L' && direction == 'W'){
		if(MasterHead->LW->next == NULL){
			tofree = (struct Node *)MasterHead->LW;
			MasterHead->LW = NULL;
			MasterTail->LW = NULL;
		}else{
			tofree = (struct Node *)MasterHead->LW;
			MasterHead->LW = (struct Node *)MasterHead->LW->next; 
		}
	}else{
		printf("ERROR");
	}
	//free(tofree);
}


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
	Train->state = 'S';
	pthread_barrier_wait(&startBarrier);
	Train->state = 'L';
	usleep(Train->loadTime*CONVERT);
	while (pthread_mutex_lock(&masterList) != 0){
		//wait
	}
	pushTrain(Train);
	pthread_mutex_unlock(&masterList);
	//printf("Train %2d is ready to go %4s\n",Train->trainid,Train->directionName);
	fflush(stdout);
	Train->state = 'W';
	int lock;
	pthread_cond_signal(&trainMoved);
	while(Train->state == 'W' || Train->state == 'R'){
		if(Train->state == 'R'){
			//try lock
			lock = pthread_mutex_trylock(&mainTrack);
			if(lock == 0){
				Train->state = 'C';
				while (pthread_mutex_lock(&masterList) != 0){
					//wait
				}
				popTrain(Train);
				pthread_mutex_unlock(&masterList);
			}//else{
			//wait for move
				//printf("Train %i is waiting for track to empty\n",Train->trainid);
			//	pthread_cond_wait(&signalMove,&moveTrainsMutex);
			//}

		}/*else{
			//wait for signal
			//printf("Train %i is waiting\n",Train->trainid);
			pthread_cond_wait(&signalMove,&moveTrainsMutex);
		}*/

	}
	
	//printf("Train %2d is ON the main track going %4s\n",Train->trainid,Train->directionName);
	usleep(Train->crossTime*CONVERT);
	pthread_cond_signal(&trainMoved);
	Train->state = 'D';
	printf("Train %2d is OFF the main track after going %4s\n",Train->trainid,Train->directionName);
	pthread_mutex_unlock(&mainTrack);
	//pthread_barrier_wait(&endBarrier);
	if(Train->trainid == 0){
		pthread_mutex_unlock(&moveTrainsMutex);
	}
	pthread_exit(NULL);
}

void setReadyTrain(struct train * Train){
	if(MasterHead->HE != NULL){
		if(MasterHead->HE->Train->trainid == Train->trainid){
			//set to waiting
			MasterHead->HE->Train->state = 'R';
		}else{
			//set to not waiting
			MasterHead->HE->Train->state = 'W';
		}
	}
	if(MasterHead->HW != NULL){
		if(MasterHead->HW->Train->trainid == Train->trainid){
			//set to waiting
			MasterHead->HW->Train->state = 'R';
		}else{
			//set to not waiting
			MasterHead->HW->Train->state = 'W';
		}
	}
	if(MasterHead->LE != NULL){
		if(MasterHead->LE->Train->trainid == Train->trainid){
			//set to waiting
			MasterHead->LE->Train->state = 'R';
		}else{
			//set to not waiting
			MasterHead->LE->Train->state = 'W';
		}
	}
	if(MasterHead->LW != NULL){
		if(MasterHead->LW->Train->trainid == Train->trainid){
			//set to waiting
			MasterHead->LW->Train->state = 'R';
		}else{
			//set to not waiting
			MasterHead->LW->Train->state = 'W';
		}
	}
}

void WaitCompletion(struct train Trains[], int numTrains){
	struct train * waitingTrain = NULL;
	struct train * selectedTrain = NULL;
	char lastDirection = 'E';
	int i;
	while ((i = countComplete(Trains,numTrains)) != numTrains){
		//printf("COMPLETE =  %i\n",i);
		//Do Nothing
		//sleep(2);
		//for(i=0;i<numTrains;i++){
		//	printf("%c ",Trains[i].state);
		//}
		//printf("\n");
		pthread_cond_wait(&trainMoved,&moveTrainsMutex);
		//if(countComplete(Trains,numTrains) == numTrains){
		//	break;
		//}
		while (pthread_mutex_lock(&masterList) != 0){
			//wait
		}
		if(waitingTrain != NULL){
			if(waitingTrain->state == 'C' || waitingTrain->state == 'D'){
				lastDirection = waitingTrain->direction;
				waitingTrain = NULL;
			}
		}
		if(waitingTrain == NULL){
			if(lastDirection == 'W'){
				//SELECT TRAIN FROM EAST
				if(MasterHead->HE != NULL){
					selectedTrain = MasterHead->HE->Train;
				}else if(MasterHead->HW != NULL){
					selectedTrain = MasterHead->HW->Train;
				}else if(MasterHead->LE != NULL){
					selectedTrain = MasterHead->LE->Train;
				}else if(MasterHead->LW != NULL){
					selectedTrain = MasterHead->LW->Train;
				}
			}else{
				//SELECT TRAIN FROM WEST
				if(MasterHead->HW != NULL){
					selectedTrain = MasterHead->HW->Train;
				}else if(MasterHead->HE != NULL){
					selectedTrain = MasterHead->HE->Train;
				}else if(MasterHead->LW != NULL){
					selectedTrain = MasterHead->LW->Train;
				}else if(MasterHead->LE != NULL){
					selectedTrain = MasterHead->LE->Train;
				}
			}
		}else{
			// Compare to see if any queued trains should be before waiting train
			if(waitingTrain->priority == 'L'){
				//check the high priority queue
				if(MasterHead->HE != NULL){
					selectedTrain = MasterHead->HE->Train;
				}
				if(MasterHead->HW != NULL){
					selectedTrain = MasterHead->HW->Train;
				}
			}else{
				if(waitingTrain->direction == 'E' && lastDirection == 'E'){
					if(MasterHead->HW != NULL){
						selectedTrain = MasterHead->HW->Train;
					}
				}
				if(waitingTrain->direction == 'W' && lastDirection == 'W'){
					if(MasterHead->HE != NULL){
						selectedTrain = MasterHead->HE->Train;
					}
				}
			}
		}
		if(selectedTrain != NULL){
			setReadyTrain(selectedTrain);
			waitingTrain = selectedTrain;
			selectedTrain = NULL;
			
		}
		pthread_mutex_unlock(&masterList);
		pthread_cond_broadcast(&signalMove);
	}
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
	

	MasterHead = malloc(sizeof(struct Node)*4);
	MasterTail = malloc(sizeof(struct Node)*4);
	//init mutex
	pthread_mutex_init(&mainTrack, NULL);
	pthread_mutex_init(&masterList, NULL);	
	pthread_mutex_init(&moveTrainsMutex, NULL);

	pthread_mutex_lock(&moveTrainsMutex);

	//init barrier
	pthread_barrier_init(&endBarrier, NULL, numTrains);
	pthread_barrier_init(&startBarrier, NULL, numTrains);

	//init condition
	pthread_cond_init(&trainMoved,NULL);
	pthread_cond_init(&signalMove,NULL);

	//Create All Trains
	for(i=0;i<numTrains;i++){
		char fileDirection = 'A';
		int fileLoadTime = -1;
		int fileCrossTime = -1;
		char filePriority = 'A';
		struct train *current = &Trains[i];
		fscanf(fp,"%c:%i,%i\n", &fileDirection,&fileLoadTime,&fileCrossTime);
		if (fileDirection == 'A' || fileLoadTime == -1 || fileCrossTime == -1){
			printf("Number of trains is too high\n");
			exit(1);
		}
		if(fileDirection == 'w' || fileDirection == 'e'){
			filePriority = 'L';
			if(fileDirection == 'w'){
				fileDirection = 'W';
			}else{
				fileDirection = 'E';
			}
		}else{
			filePriority = 'H';
		}
		if(fileDirection != 'W' && fileDirection != 'E'){
			printf("Direction of train is not E or W\n");
			exit(1);
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
	pthread_mutex_destroy(&mainTrack);
	pthread_barrier_destroy(&startBarrier);
	pthread_barrier_destroy(&endBarrier);

	free(MasterHead);
	return (0);
}
