#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAXLENGTH 200
#define CONVERT 100000
#define WAIT 100000

struct train{
	char priority;
	char direction;
	char directionName[5];
	char state;    
	//N=new, S=Start, L=Loading, W=Waiting, R=Ready to Cross C=Crossing, D=Done, P=Pushing
	int loadTime;
	int crossTime;
	int trainid;
};

struct timespec start;


void printList();


/*************
* Struct to hold Processes
* Includes: 
*************/
struct Node{
	struct Node *next;
	struct Node *prev;
	struct train *Train;
};


struct List{
	struct Node *head;
	struct Node *tail;
};

/*************
* Initialize 
* 
*************/
struct List *HE;
struct List *HW;
struct List *LE;
struct List *LW;

struct List *Waiting;

pthread_mutex_t mainTrack;
pthread_mutex_t moveTrainsMutex;
pthread_mutex_t trainMovedMutex;
pthread_mutex_t masterList;
pthread_mutex_t pushLock;

pthread_cond_t trainMoved;
pthread_cond_t signalMove;

int trainPushing;

pthread_barrier_t startBarrier;


void printList(){
	struct Node * temp = HE->head;
	fprintf(stderr,"------------------------------------------\n");
	fprintf(stderr,"----------FORWARD-------------------------\n");
	fprintf(stderr,"------------------------------------------\n");
	fprintf(stderr,"HE: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->next;
	}
	fprintf(stderr,"\n");
	temp = HW->head;
	fprintf(stderr,"HW: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->next;
	}
	fprintf(stderr,"\n");
	temp = LE->head;
	fprintf(stderr,"LE: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->next;
	}
	fprintf(stderr,"\n");
	temp = LW->head;
	fprintf(stderr,"LW: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->next;
	}
	fprintf(stderr,"\n");
	fprintf(stderr,"------------------------------------------\n");
	
	temp = HE->tail;
	fprintf(stderr,"------------------------------------------\n");
	fprintf(stderr,"----------BACKWARD------------------------\n");
	fprintf(stderr,"------------------------------------------\n");
	fprintf(stderr,"HE: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->prev;
	}
	fprintf(stderr,"\n");
	temp = HW->tail;
	fprintf(stderr,"HW: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->prev;
	}
	fprintf(stderr,"\n");
	temp = LE->tail;
	fprintf(stderr,"LE: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->prev;
	}
	fprintf(stderr,"\n");
	temp = LW->tail;
	fprintf(stderr,"LW: ");
	while(temp != NULL){
		fprintf(stderr," (Train: %i, State: %c) ",temp->Train->trainid,temp->Train->state);
		temp = (struct Node *)temp->prev;
	}
	fprintf(stderr,"\n");
	fprintf(stderr,"------------------------------------------\n");
}


void pushTrain(struct train *Train){
	char priority = Train->priority;
	char direction = Train->direction;
	struct Node * newNode = malloc(sizeof(struct Node));
	struct Node * temp = NULL;
	struct Node * tail = NULL;
	newNode->Train = Train;
	int inserted = 0;
	struct List * list = NULL;

	if(priority == 'H' && direction == 'E'){
		list = HE;
	}else if(priority == 'H' && direction == 'W'){
		list = HW;
	}else if(priority == 'L' && direction == 'E'){
		list = LE;
	}else if(priority == 'L' && direction == 'W'){
		list = LW;
	}

	if(list->head == NULL){
		list->head = newNode;
		list->tail = newNode;
		inserted = 1;
	}else{
		temp = list->head;
		while(temp != NULL && inserted == 0){
			if(temp->Train->loadTime < Train->loadTime){
				if(temp->next != NULL){
					//next train
					temp = temp->next;
				}else{
					//insert at tail
					temp->next = newNode;
					list->tail = newNode;
					newNode->prev = temp;
					inserted = 1;
				}
			}else if(temp->Train->loadTime > Train->loadTime){
				//insert before
				if(temp->prev == NULL){
					//at head
					list->head = newNode;
				}else{
					newNode->prev = temp->prev;
					newNode->prev->next = newNode;
				}
				newNode->next = temp;
				temp->prev = newNode;
				inserted = 1;
			}else{
				//check id
				if(temp->Train->trainid > Train->trainid){
					//insert before
					if(temp->prev == NULL){
						//at head
						list->head = newNode;
					}else{
						newNode->prev = temp->prev;
						newNode->prev->next = newNode;
					}
					newNode->next = temp;
					temp->prev = newNode;
					inserted = 1;
				}else if(temp->Train->trainid < Train->trainid){
					if(temp->next != NULL){
						temp = temp->next;
					}else{
						//insert at tail
						list->tail = newNode;
						temp->next = newNode;
						newNode->prev = temp;
						inserted = 1;
					}
				}else{
					printf("ERROR: trains with same id %d",temp->Train->trainid);
				}
			}
		}
	}

	/*
	if(priority == 'H' && direction == 'E'){
		temp = HE->head;
		tail = HE->tail;
	}else if(priority == 'H' && direction == 'W'){
		temp = HW->head;
		tail = HE->tail;
	}else if(priority == 'L' && direction == 'E'){
		temp = LE->head;
		tail = HE->tail;
	}else if(priority == 'L' && direction == 'W'){
		temp = LW->head;
		tail = HE->tail;
	}
	if(temp == NULL){
		insertHead(priority,direction,newNode);
		inserted = 1;
	}else{
		while(temp != NULL && temp->next != NULL && inserted == 0){
			if(temp->next->Train->loadTime < newNode->Train->loadTime){
				temp = temp->next;
			}else if(temp->next->Train->loadTime == newNode->Train->loadTime){
				if(temp->Train->trainid < newNode->Train->trainid){
					temp = temp->next;
				}else{
					//insert infront
					newNode->next = temp->next;
					newNode->prev = temp;
					temp->next->prev = newNode;
					temp->next = newNode;
					inserted = 1;
				}
			}else{
				//insert infront
				newNode->next = temp->next;
				newNode->prev = temp;
				temp->next->prev = newNode;
				temp->next = newNode;
				inserted = 1;
			}
		}	
	}
	if(inserted == 0){
		//insert at tail
		temp->next = newNode;
		tail = newNode;
		newNode->prev = temp;
	}*/
}

void popTrain(struct train *Train){
	char priority = Train->priority;
	char direction = Train->direction;
	struct Node * tofree = NULL;
	struct Node * head = NULL;
	struct Node * tail = NULL;

	struct List * list = NULL;

	if(priority == 'H' && direction == 'E'){
		list = HE;
	}else if(priority == 'H' && direction == 'W'){
		list = HW;
	}else if(priority == 'L' && direction == 'E'){
		list = LE;
	}else if(priority == 'L' && direction == 'W'){
		list = LW;
	}

	if(list->head->next == NULL){
		tofree = list->head;
		list->head = NULL;
	}else{
		tofree = list->head;
		list->head = list->head->next;
		list->head->prev =  NULL;
	}
	if(tofree->Train->trainid != Train->trainid){
		printf("ERROR\n");
	}
/*	if(priority == 'H' && direction == 'E'){
		tofree = HE->head;
		if(HE->head->next == NULL){
			HE->head = NULL;
			HE->tail = NULL;
		}else{
			HE->head = HE->head->next;
		}
	}else if(priority == 'H' && direction == 'W'){
		tofree = HW->head;
		if(HW->head->next == NULL){
			HW->head = NULL;
			HW->tail = NULL;
		}else{
			HW->head = HW->head->next;
		}
	}else if(priority == 'L' && direction == 'E'){
		tofree = LE->head;
		if(LE->head->next == NULL){
			LE->head = NULL;
			LE->tail = NULL;
		}else{
			LE->head = LE->head->next;
		}
	}else if(priority == 'L' && direction == 'W'){
		tofree = LW->head;
		if(LW->head->next == NULL){
			LW->head = NULL;
			LW->tail = NULL;
		}else{
			LW->head = LW->head->next;
		}
	}*/
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


struct timespec diff(struct timespec end){
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

/**/
void calculate_time(struct timespec current, long *msec, int *sec, int *min, int *hour){
    struct timespec time_diff;
    time_diff = diff(current); 
    *sec = (int)time_diff.tv_sec % 60;
    *min = ((int)time_diff.tv_sec / 60) % 60;
    *hour = ((int)time_diff.tv_sec / 3600) % 24;
    *msec = time_diff.tv_nsec/100000000;
}

void *CreateTrain(void *tempTrain){
	struct timespec ts_current;
    long msec;
    int sec;
    int min;
    int hour;

	struct train * Train = tempTrain;
	Train->state = 'S';
	pthread_barrier_wait(&startBarrier);
	Train->state = 'L';
	usleep(Train->loadTime*CONVERT);
	Train->state = 'P';
	while (pthread_mutex_lock(&pushLock) != 0){
		//wait
	}
	trainPushing++;
	pthread_mutex_unlock(&pushLock);
	while (pthread_mutex_lock(&masterList) != 0){
		//wait
	}
	pushTrain(Train);
	pthread_mutex_unlock(&masterList);
	clock_gettime(CLOCK_MONOTONIC, &ts_current);
    calculate_time(ts_current, &msec, &sec, &min, &hour);
    printf("%02d:%02d:%02d.%1ld Train %2d is ready to go %4s\n", hour, min, sec, msec ,Train->trainid,Train->directionName);
	fflush(stdout);
	Train->state = 'W';
	int lock;
	pthread_cond_signal(&trainMoved);
	trainPushing--;
	while(Train->state == 'W' || Train->state == 'R'){
		if(Train->state == 'R'){
			//try lock
			lock = pthread_mutex_trylock(&mainTrack);
			//printf("Train %i is in state R %i\n",Train->trainid,lock);
			if(lock == 0){
				Train->state = 'C';
				while (pthread_mutex_lock(&masterList) != 0){
					//wait
				}
				popTrain(Train);
				pthread_mutex_unlock(&masterList);
			}
		}

	}
	clock_gettime(CLOCK_MONOTONIC, &ts_current);
    calculate_time(ts_current, &msec, &sec, &min, &hour);
	printf("%02d:%02d:%02d.%1ld Train %2d is ON the main track going %4s\n", hour, min, sec, msec ,Train->trainid,Train->directionName);
	usleep(Train->crossTime*CONVERT);
	pthread_cond_signal(&trainMoved);
	Train->state = 'D';
	clock_gettime(CLOCK_MONOTONIC, &ts_current);
    calculate_time(ts_current, &msec, &sec, &min, &hour);
	printf("%02d:%02d:%02d.%1ld Train %2d is OFF the main track after going %4s\n", hour, min, sec, msec ,Train->trainid,Train->directionName);
	pthread_mutex_unlock(&mainTrack);
	//pthread_cond_signal(&trainMoved);
	pthread_exit(NULL);
}

void setReadyTrain(struct train * Train){
	if(HE->head != NULL){
		if(HE->head->Train->trainid == Train->trainid)
			HE->head->Train->state = 'R';
		else
			HE->head->Train->state = 'W';
	}
	if(HW->head != NULL){
		if(HW->head->Train->trainid == Train->trainid)
			HW->head->Train->state = 'R';
		else
			HW->head->Train->state = 'W';
	}
	if(LE->head != NULL){
		if(LE->head->Train->trainid == Train->trainid)
			LE->head->Train->state = 'R';
		else
			LE->head->Train->state = 'W';
	}
	if(LW->head != NULL){
		if(LW->head->Train->trainid == Train->trainid)
			LW->head->Train->state = 'R';
		else
			LW->head->Train->state = 'W';
	}
}

void scheduleTrains(struct train Trains[], int numTrains){
	struct train * waitingTrain = NULL;
	struct train * selectedTrain = NULL;
	char lastDirection = 'E';
	int i;
	clock_gettime(CLOCK_MONOTONIC, &start);
	while ((i = countComplete(Trains,numTrains)) != numTrains){

		pthread_cond_wait(&trainMoved,&trainMovedMutex);
		while(trainPushing > 0){
			//printf("Train Pushing = %i\n",trainPushing);
		}
		while (pthread_mutex_lock(&masterList) != 0){
			//wait
		}
		//printList();
		if(waitingTrain != NULL){
			if(waitingTrain->state == 'C' || waitingTrain->state == 'D'){
				lastDirection = waitingTrain->direction;
				waitingTrain = NULL;
			}
		}
		if(waitingTrain == NULL){
			if(lastDirection == 'W'){
				//SELECT TRAIN FROM EAST
				if(HE->head != NULL){
					selectedTrain = HE->head->Train;
				}else if(HW->head != NULL){
					selectedTrain = HW->head->Train;
				}else if(LE->head != NULL){
					selectedTrain = LE->head->Train;
				}else if(LW->head != NULL){
					selectedTrain = LW->head->Train;
				}
			}else{
				//SELECT TRAIN FROM WEST
				if(HW->head != NULL){
					selectedTrain = HW->head->Train;
				}else if(HE->head != NULL){
					selectedTrain = HE->head->Train;
				}else if(LW->head != NULL){
					selectedTrain = LW->head->Train;
				}else if(LE->head != NULL){
					selectedTrain = LE->head->Train;
				}
			}
		}else{
			// Compare to see if any queued trains should be before waiting train
			if(waitingTrain->priority == 'L'){
				//check the high priority queue

				if(lastDirection == 'E'){
					//SELECT west Train
					if(waitingTrain->direction == 'E'){
						if(LW->head != NULL){
							selectedTrain = LW->head->Train;
						}
						if(HW->head != NULL){
							selectedTrain = HW->head->Train;
						}else if(HE->head != NULL){
							selectedTrain = HE->head->Train;
						}
					}else{
						if(HW->head != NULL){
							selectedTrain = HW->head->Train;
						}else if(HE->head != NULL){
							selectedTrain = HE->head->Train;
						}
					}
									
				}else{
					//SELECT east Train
					if(waitingTrain->direction == 'W'){
						if(LE->head != NULL){
							selectedTrain = LE->head->Train;
						}
						if(HE->head != NULL){
							selectedTrain = HE->head->Train;
						}else if(HW->head != NULL){
							selectedTrain = HW->head->Train;
						}
					}else{
						if(HE->head != NULL){
							selectedTrain = HE->head->Train;
						}else if(HW->head != NULL){
							selectedTrain = HW->head->Train;
						}
					}
				}
			}else{
				if(waitingTrain->direction == 'E' && lastDirection == 'E'){
					if(HW->head != NULL){
						selectedTrain = HW->head->Train;
					}
				}
				if(waitingTrain->direction == 'W' && lastDirection == 'W'){
					if(HE->head != NULL){
						selectedTrain = HE->head->Train;
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
	trainPushing = 0;
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

	// init lists
	HE = malloc(sizeof(struct List));
	HW = malloc(sizeof(struct List));
	LE = malloc(sizeof(struct List));
	LW = malloc(sizeof(struct List));

	//init information mutexes
	pthread_mutex_init(&mainTrack, NULL);
	pthread_mutex_init(&masterList, NULL);
	pthread_mutex_init(&pushLock, NULL);
	//init 	condition mutexes
	pthread_mutex_init(&moveTrainsMutex, NULL);
	pthread_mutex_init(&trainMovedMutex, NULL);

	//lock condition mutexes
	pthread_mutex_lock(&trainMovedMutex);
	pthread_mutex_lock(&moveTrainsMutex);

	//init barrier
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
	
	scheduleTrains(Trains,numTrains);
	for(i=0;i<numTrains;i++){
		pthread_join(pthreads[i],&Status);
	}
	pthread_mutex_destroy(&mainTrack);
	pthread_barrier_destroy(&startBarrier);

//	free(Master);
	return (0);
}