#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAXLENGTH 200
#define CONVERT 100000
#define WAIT 97500

struct train{
	char priority;
	char direction;
	char directionName[5];
	char state;    
	//N=new, S=Start, L=Loading, W=Waiting, R=Ready to Cross C=Crossing, D=Done, P=Pushing
	int loadTime;
	int crossTime;
	int trainid;
	pthread_cond_t waitingDispatch;
	pthread_mutex_t trainMutex;
};

struct timespec start;


/*************
* Struct to hold Trains
*************/
struct Node{
	struct Node *next;
	struct Node *prev;
	struct train *Train;
};

/*************
* List struct to hold Processes
*************/
struct List{
	struct Node *head;
	struct Node *tail;
};

/*************
* Initialize 
*************/
struct List *HE;
struct List *HW;
struct List *LE;
struct List *LW;

struct List *Done;

pthread_mutex_t mainTrack;
pthread_mutex_t moveTrainsMutex;
pthread_mutex_t trainMovedMutex;
pthread_mutex_t masterList;
pthread_mutex_t pushLock;

pthread_cond_t trainMoved;

int trainPushing;

pthread_barrier_t startBarrier;
pthread_barrier_t endBarrier;

/**************************
* Define Function Headers *
***************************/
void pushTrain(struct train *Train);
void popTrain(struct train *Train);
int countComplete(struct train Trains[],int numTrains);
struct timespec diff(struct timespec end);
void calculate_time(struct timespec current, long *msec, int *sec, int *min, int *hour);
void *RunTrain(void *tempTrain);
void setReadyTrain(struct train * Train);
void scheduleTrains(struct train Trains[], int numTrains);
void printList(struct List *list);
void init(int numTrains);
void emptyList(struct List *list);
void cleanUp();

/*pushTrain
Creates a Node and pushes it onto the proper list*/
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
	//if node at head
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
}

/*popTrain
pops the head of the list with the same priority as the train*/
void popTrain(struct train *Train){
	char priority = Train->priority;
	char direction = Train->direction;
	struct Node * tofree = NULL;
	struct List * list = NULL;

	//set head of list according to priority and direction
	if(priority == 'H' && direction == 'E'){
		list = HE;
	}else if(priority == 'H' && direction == 'W'){
		list = HW;
	}else if(priority == 'L' && direction == 'E'){
		list = LE;
	}else if(priority == 'L' && direction == 'W'){
		list = LW;
	}

	//pop the head of the list off
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

	//put on finished list to print out later if debugging
	if(Done->tail == NULL){
		Done->head = tofree;
		Done->tail = tofree;
	}else{
		Done->tail->next = tofree;
		tofree->prev = Done->tail;
		Done->tail = tofree;
	}
}

/*countComplete
Counts number of complete trains to compare with numTrains*/
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

/*function for determining the elapsed time*/
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

/*function to calculate the elapsed time*/
void calculate_time(struct timespec current, long *msec, int *sec, int *min, int *hour){
    struct timespec time_diff;
    time_diff = diff(current); 
    *sec = (int)time_diff.tv_sec % 60;
    *min = ((int)time_diff.tv_sec / 60) % 60;
    *hour = ((int)time_diff.tv_sec / 3600) % 24;
    *msec = time_diff.tv_nsec/100000000;
}

/*function to calculate the elapsed time in msec*/
void calculate_time_msec(struct timespec current, long *msec){
    struct timespec time_diff;
    time_diff = diff(current); 
    int sec, min, hour;
    sec = (int)time_diff.tv_sec % 60;
    min = ((int)time_diff.tv_sec / 60) % 60;
    hour = ((int)time_diff.tv_sec / 3600) % 24;
    *msec = (10*sec)+(time_diff.tv_nsec/100000000);
}

/*main train thread program*/
void *RunTrain(void *tempTrain){
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
    printf("%02d.%ld Train %2d is ready to go %4s\n", sec, msec ,Train->trainid,Train->directionName);
	fflush(stdout);
	Train->state = 'W';
	int lock;
	pthread_cond_signal(&trainMoved);
	while (pthread_mutex_lock(&pushLock) != 0){
		//wait
	}
	trainPushing--;
	pthread_mutex_unlock(&pushLock);
	while(Train->state == 'W' || Train->state == 'R'){
		pthread_cond_wait(&(Train->waitingDispatch),&(Train->trainMutex));
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
			}
		}

	}
	clock_gettime(CLOCK_MONOTONIC, &ts_current);
    calculate_time(ts_current, &msec, &sec, &min, &hour);
	printf("%02d.%ld Train %2d is ON the main track going %4s\n", sec, msec ,Train->trainid,Train->directionName);
	usleep(Train->crossTime*CONVERT);
	Train->state = 'D';
	clock_gettime(CLOCK_MONOTONIC, &ts_current);
    calculate_time(ts_current, &msec, &sec, &min, &hour);
	printf("%02d.%ld Train %2d is OFF the main track after going %4s\n", sec, msec ,Train->trainid,Train->directionName);
	pthread_mutex_unlock(&mainTrack);
	pthread_cond_signal(&trainMoved);
	pthread_barrier_wait(&endBarrier);
	usleep(WAIT);
	pthread_cond_signal(&trainMoved);
	pthread_exit(NULL);
}

/*checks to make sure the right train is waiting to go*/
void setReadyTrain(struct train *Train){
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

int trainsLoading(long msec,struct train Trains[], int numTrains){
	int timePassed = (int)msec;
	int i;
	int passed = 1;
	for(i=0;i<numTrains;i++){
		//fprintf(stderr,"%c %d",Trains[i].state,Trains[i].loadTime);
		if(Trains[i].loadTime <= timePassed && (Trains[i].state == 'P' || Trains[i].state == 'L')){
			passed = 0;
		}
	}
	//fprintf(stderr,"        %i",timePassed);
	//fprintf(stderr,"\n");
	return passed;
}

/*dispatcher thread*/
void scheduleTrains(struct train Trains[], int numTrains){
	struct train * waitingTrain = NULL;
	struct train * selectedTrain = NULL;
	struct timespec ts_current;
	char lastDirection = 'E';
    long msec;
    int i;
    clock_gettime(CLOCK_MONOTONIC, &start);

	while ((i = countComplete(Trains,numTrains)) != numTrains){
		pthread_cond_wait(&trainMoved,&trainMovedMutex);

		clock_gettime(CLOCK_MONOTONIC, &ts_current);
		calculate_time_msec(ts_current, &msec);

		while(trainsLoading(msec,Trains,numTrains) == 0){
			//wait
		}
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
			if(selectedTrain!= NULL)
				pthread_cond_signal(&(selectedTrain->waitingDispatch));
			if(waitingTrain!= NULL)
				pthread_cond_signal(&(waitingTrain->waitingDispatch));
			waitingTrain = selectedTrain;
			selectedTrain = NULL;
			
		}
		if(waitingTrain!= NULL)
				pthread_cond_signal(&(waitingTrain->waitingDispatch));
		pthread_mutex_unlock(&masterList);
	}
}

/*prints a list from head to tail*/
void printList(struct List *list){
	struct Node *temp = list->head;
	while(temp != NULL){
		printf("%i ",temp->Train->trainid);
		temp = temp->next;
	}
	printf("\n");
}

/*init components for program*/
void init(int numTrains){
	//init global variables
	trainPushing = 0;

	// init lists
	HE = malloc(sizeof(struct List));
	HW = malloc(sizeof(struct List));
	LE = malloc(sizeof(struct List));
	LW = malloc(sizeof(struct List));
	Done = malloc(sizeof(struct List));

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
	pthread_barrier_init(&endBarrier, NULL, numTrains);
	//init condition
	pthread_cond_init(&trainMoved,NULL);



}

/*empties a list if any elements in it*/
void emptyList(struct List *list){
	struct Node *temp = list->tail;
	while(temp !=NULL){
		if(temp->prev != NULL){
			temp = temp->prev;
			free(temp->next);
		}else{
			break;
		}
	}
	if(list->head != NULL){
		free(list->head);
	}
}

/*free and destroy all malloc data*/
void cleanUp(){
	//Free Done List
	emptyList(Done);
	
	//free lists
	free(HE);
	free(HW);
	free(LE);
	free(LW);
	free(Done);

	//destroy information mutexes
	pthread_mutex_destroy(&mainTrack);
	pthread_mutex_destroy(&masterList);
	pthread_mutex_destroy(&pushLock);
	
	//destroy condition mutexes
	pthread_mutex_destroy(&moveTrainsMutex);
	pthread_mutex_destroy(&trainMovedMutex);

	//destory barrier
	pthread_barrier_destroy(&startBarrier);

	//destroy condition
	pthread_cond_destroy(&trainMoved);

}


/*start up and creation of threads*/
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

	init(numTrains);

	struct train Trains[numTrains];
	pthread_t pthreads[numTrains];

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
			exit(-1);
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
			exit(-1);
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
		pthread_cond_init(&(Trains[i].waitingDispatch),NULL);
		pthread_mutex_init(&(Trains[i].trainMutex),NULL);
		pthread_mutex_lock(&(Trains[i].trainMutex));
		
		int rc = pthread_create(&pthreads[i], NULL, RunTrain, (void *)current);
		if (rc){
        		printf("ERROR; return code from pthread_create() is %d\n", (int)rc);
        		exit(-1);
    		}
	}
	
	fclose(fp);

	scheduleTrains(Trains,numTrains);

	for(i=0;i<numTrains;i++){
		pthread_join(pthreads[i],&Status);
	}

	//printList(Done);

	cleanUp();

	return (0);
}
