/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*					SEG4145: Real-Time and Embedded System Design
*
*							Assignment #4 Skeleton
*
*                     (c) Copyright 2010- Stejarel C. Veres, cveres@site.uottawa.ca
* 					  Portions adapted after Jean J. Labrosse
*
*                As is, this program will create a main (startup) task which will in turn
*             spawn two children. One of them will count odd numbers, the other - even ones.
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define TASK_STK_SIZE			512		/* Size of start task's stacks                         */
#define TASK_START_PRIO			0		/* Priority of your startup task		       	 */
#define N_TASKS                 2           /* Number of (child) tasks to spawn                    */

//Messages between keyboardControl and movementControl
#define MOVE_TILE_FORWARD_MESSAGE 0
#define MOVE_TILE_BACKWARD_MESSAGE 1
#define TURN_90_CW_MESSAGE 2
#define PERFORM_CIRCLE_CW_MESSAGE 3
#define PERFORM_CIRCLE_CCW_MESSAGE 4
#define INCREASE_CIRCLE_RADIUS_MESSAGE 5

// Message queue constants
#define MOTOR_QUEUE_SIZE 1

// Debug flag
// #define DEBUG_MODE

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void sendMessageThingy(OS_EVENT* queue, void* msg);
void TaskStart(void *data);				/* Startup task							 */
static void TaskStartCreateTasks(void);         /* Will be used to create all the child tasks          */
void lcdTask(void*); /* Task 1 declaration */
void Task2(void*); /* Task 2 declaration */
<<<<<<< HEAD
=======
void keyboardControlTask(void*); /* keyboardControlTask declaration */
void sendMessage(OS_EVENT* queue, OS_EVENT* sem, void* msg); /* sendMessage declaration */
void* receiveMessage(OS_EVENT* queue, OS_EVENT* sem); /* receiveMessage declaration */
void printQueueError(INT8U error); /* printQueueError declaration */
void printSemaphoreError(INT8U error); /* printSemaphoreError declaration */
>>>>>>> FINALLY added message sending logic.

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStartStk[TASK_STK_SIZE];			/* Start task's stack						 */
OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];         /* Stacks for other (child) tasks				 */
INT8U TaskData[N_TASKS];				/* Parameters to pass to each task                     */
<<<<<<< HEAD
OS_EVENT* queue;
char dummyArr[5];
void* TaskPointers[N_TASKS] = { lcdTask, Task2 };	/* Function pointers to the tasks */
=======

// Motor queue variables
OS_EVENT* motorQueue; /* The queue for sending messages to the motor */
void* motorQueueData[MOTOR_QUEUE_SIZE]; /* The memory space for the motor queue */
OS_EVENT* motorSem; /* The semaphore controlling access to the motor queue */

void* TaskPointers[N_TASKS] = { Task1, Task2, keyboardControlTask };	/* Function pointers to the tasks */
>>>>>>> FINALLY added message sending logic.

/*
*********************************************************************************************************
*                                             MAIN FUNCTION
*********************************************************************************************************
*/

int main(void)
{
    OSInit();						/* Initialize uC/OS-II						 */

    /*
     * Create and initialize any semaphores, mailboxes etc. here
     */

    OSTaskCreate(TaskStart, (void *) 0,
		     &TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);	/* Create the startup task	 */

    OSStart();						/* Start multitasking						 */

    return 0;
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void TaskStart(void *pdata)
{
    INT16S key;
    pdata = pdata;                                         /* Prevent compiler warning                 */


#if OS_TASK_STAT_EN
    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */
#endif

    /*
     * Display version information
     */

    printf("Startup (Main) Task:\n");
    printf("--------------------\n");
    printf("Running under uC/OS-II V%4.2f (with WIN32 port V%4.2f).\n",
           ((FP32) OSVersion())/100, ((FP32)OSPortVersion())/100);
    printf("Press the Escape key to stop.\n\n");

	// Motor queue initialization
	motorQueue = OSQCreate(motorQueueData, MOTOR_QUEUE_SIZE);
	motorSem = OSSemCreate(MOTOR_QUEUE_SIZE);
	
    /*
     * Here we create all other tasks (threads)
     */
    TaskStartCreateTasks();

    INT8U circle_ccw = 0; //true if circle goes counter-clockwise, false if circle goes clockwise
    INT8U mode = 0; //mode 0 == mode 1 in requirements, mode 1 == mode 2 in requirements
    while (1) {
        INT8U command;
        if (PC_GetKey(&key) == TRUE) { //See if key has been pressed
            switch (key) {
                case 0x1B:
                    exit(0);
                    break;
                case '0':
                    if (!mode) {
                        command = MOVE_TILE_FORWARD_MESSAGE;
                        sendMessageThingy(queue, &command);
                    }
                    else {
                        command = INCREASE_CIRCLE_RADIUS_MESSAGE;
                        sendMessageThingy(queue, &command);
                    }
                    break;
                case '1':
                    if (!mode) {
                        command = MOVE_TILE_BACKWARD_MESSAGE;
                        sendMessageThingy(queue, &command);
                    }
                    else {
                        circle_ccw = (circle_ccw + 1) % 2;
                    }
                    break;
                case '2':
                    if (!mode) {
                        command = TURN_90_CW_MESSAGE;
                        sendMessageThingy(queue, &command);
                    }
                    else {
                        command = PERFORM_CIRCLE_CW_MESSAGE + circle_ccw;
                        sendMessageThingy(queue, &command);
                    }
                    break;
                case '3':
                    mode = !mode;
                    break;
            }
        }
        OSTimeDly(10);
    }
}

/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static void TaskStartCreateTasks(void)
{
    INT8U i;
    INT8U prio;

    for (i = 0; i < N_TASKS; i++) {
        prio = i + 1;
        TaskData[i] = prio;
        OSTaskCreateExt(TaskPointers[i],
                        (void *) &TaskData[i],
                        &TaskStk[i][TASK_STK_SIZE - 1],
                        prio,
                        0,
                        &TaskStk[i][0],
                        TASK_STK_SIZE,
                        (void *) 0,
                        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR | OS_TASK_OPT_SAVE_FP);
    }
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void lcdTask(void *pdata)
{
    while (1) {
     char *msg = "aaaaa"; //TODO read from message queue
	 if(msg != NULL) {
		printf("Message printed to LCD screen: ");
		printf(msg);
		printf("\n");
	 }

	OSTimeDly(50);
    }
}

void Task2(void *pdata)
{
	int radius = 2;
    while (1) {
		INT8U cmd = -1; //TODO read from message queue
		if(cmd == MOVE_TILE_FORWARD_MESSAGE) {
			printf("Move one tile forward\n");
		}
		else if(cmd == MOVE_TILE_BACKWARD_MESSAGE) {
			printf("Move one tile backwards\n");
		}
		else if(cmd == TURN_90_CW_MESSAGE) {
			printf("Turn 90 degrees clockwise\n");
		}
		else if(cmd == PERFORM_CIRCLE_CW_MESSAGE) {
			printf("Moving in a clockwise circle\n");
		}
		else if(cmd == PERFORM_CIRCLE_CCW_MESSAGE) {
			printf("Moving in a counter-clockwise circle\n");
		}
		else if(cmd == INCREASE_CIRCLE_RADIUS_MESSAGE) {
			radius++;
			if(radius == 5) {
				radius = 2;
			}
			printf("Circle radius changed to ");
			printf("%d", radius);
			printf("\n");
		}
		else {
			printf("Invalid command\n");
		}

		OSTimeDly(50);
    }
}

void sendMessageThingy(OS_EVENT* queue, void* msg) { //LOLOL LEO REPLACE THIS WITH THE REALZ ONE
    /*
    INT8U err = OSQPOST(queue, msg);
    while (err == OS_Q_FULL) {
        err = OSQPOST(queue, msg);
    }
    return err;
    */
}

// Sends the given message to the given message queue. Returns 1
// if it succeeds, 0 otherwise.
void sendMessage(OS_EVENT* queue, OS_EVENT* sem, void* msg) {
	INT8U result = -1;
	INT8U semErr;
	OSSemPend(sem, 0, &semErr); // Assume that nothing can go wrong
#ifdef DEBUG_MODE
		printSemaphoreError(semErr);
#endif
	while (result != OS_NO_ERR) { // Keep trying until it sends
		result = OSQPost(queue, msg);
#ifdef DEBUG_MODE
		printQueueError(result);
#endif
		OSTimeDly(10);
	}
	fflush(stdout);
}

// Attempts to receive a message from the given message
// queue. It blocks until a message is available. If an
// error occurs, it returns a null pointer.
void* receiveMessage(OS_EVENT* queue, OS_EVENT* sem) {
	INT8U err;
	void* received = OSQPend(queue, 0, &err);
	INT8U semErr = OSSemPost(sem);
#ifdef DEBUG_MODE
	printQueueError(err);
	printSemaphoreError(semErr);
#endif
	return received;
}

// Used for debugging. Prints the error message corresponding to
// the given error code in the context of a message queue.
void printQueueError(INT8U error) {
	switch (error){
		case OS_NO_ERR:
			printf("Message was deposited in the queue or message was received\n");
			break;
		case OS_TIMEOUT:
			printf("Message was not received within the specified timeout\n");
			break;
		case OS_ERR_PEND_ISR:
			printf("This function was called from an ISR and uC/OS-II must suspend the task. To avoid this don't call this function from an ISR\n");
			break;
		case OS_Q_FULL:
			printf("No room in the queue\n");
			break;
		case OS_ERR_EVENT_TYPE:
			printf("pevent is not pointing to a message queue\n");
			break;
		case OS_ERR_PEVENT_NULL:
			printf("pevent is a NULL pointer\n");
			break;
		case OS_ERR_POST_NULL_PTR:
			printf("msg is a NULL pointer\n");
			break;
	}
}

// Used for debugging. Prints the error message corresponding to
// the given error code in the context of a semaphore.
void printSemaphoreError(INT8U error) {
	switch (error) {
		case OS_NO_ERR:
			printf("Semaphore is available or has been signaled or released\n");
			break;
		case OS_TIMEOUT:
			printf("Semaphore was not obtained within the specified timeout\n");
			break;
		case OS_ERR_EVENT_TYPE:
			printf("pevent is not pointing to a semaphore\n");
			break;
		case OS_ERR_PEVENT_NULL:
			printf("pevent is a NULL pointer\n");
			break;
		case OS_ERR_PEND_ISR:
			printf("This function was called from an ISR and uC/OS-II must suspend the task. To avoid this don't call this function from an ISR\n");
			break;
		case OS_SEM_OVF:
			printf("Semaphore counts has overflowed (> 65535)\n");
			break;
	}
}
