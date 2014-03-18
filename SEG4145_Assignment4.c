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

// Task message prefixes
#define MOTOR_PFX "Motor Task: "
#define DISPLAY_PFX "Display Task: "
#define LCD_PFX "Message displayed on LCD: "
#define LED_PFX "LED Pattern: "
#define LINE_1_PFX "Line 1: "
#define LINE_2_PFX "Line 2: "

// Message queue constants
#define MOTOR_QUEUE_SIZE 1
#define LCD_QUEUE_SIZE 1

// Motor constants
#define MIN_DELAY 50

// Debug flag
// #define DEBUG_MODE

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void TaskStart(void *data);				/* Startup task							 */
static void TaskStartCreateTasks(void);         /* Will be used to create all the child tasks          */
void displayTask(void*); /* Task 1 declaration */
void motorTask(void*); /* Task 2 declaration */
void sendMessage(OS_EVENT* queue, OS_EVENT* sem, void* msg); /* sendMessage declaration */
void* receiveMessage(OS_EVENT* queue, OS_EVENT* sem); /* receiveMessage declaration */
void printQueueError(INT8U error); /* printQueueError declaration */
void printSemaphoreError(INT8U error); /* printSemaphoreError declaration */
void stopAfterDelay(int delay, int wasMoving); /* stopAfterDelay declaration */

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStartStk[TASK_STK_SIZE];			/* Start task's stack						 */
OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];         /* Stacks for other (child) tasks				 */
INT8U TaskData[N_TASKS];				/* Parameters to pass to each task                     */
void* TaskPointers[N_TASKS] = { displayTask, motorTask };	/* Function pointers to the tasks */

// Motor queue variables
OS_EVENT* motorQueue; /* The queue for sending messages to the motor */
void* motorQueueData[MOTOR_QUEUE_SIZE]; /* The memory space for the motor queue */
OS_EVENT* motorSem; /* The semaphore controlling access to the motor queue */

//LCD queue variables
OS_EVENT* lcdQueue;
void* lcdQueueData[LCD_QUEUE_SIZE]; /* The memory space for the motor queue */
OS_EVENT* lcdSem; /* The semaphore controlling access to the motor queue */

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

	// LCD queue initialization
	lcdQueue = OSQCreate(lcdQueueData, LCD_QUEUE_SIZE);
	lcdSem = OSSemCreate(LCD_QUEUE_SIZE);

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
                        sendMessage(motorQueue, motorSem, (void*)&command);
                    }
                    else {
                        command = INCREASE_CIRCLE_RADIUS_MESSAGE;
                        sendMessage(motorQueue, motorSem, (void*)&command);
                    }
                    break;
                case '1':
                    if (!mode) {
                        command = MOVE_TILE_BACKWARD_MESSAGE;
                        sendMessage(motorQueue, motorSem, (void*)&command);
                    }
                    else {
                        circle_ccw = (circle_ccw + 1) % 2;
                    }
                    break;
                case '2':
                    if (!mode) {
                        command = TURN_90_CW_MESSAGE;
                        sendMessage(motorQueue, motorSem, (void*)&command);
                    }
                    else {
                        command = PERFORM_CIRCLE_CW_MESSAGE + circle_ccw;
                        sendMessage(motorQueue, motorSem, (void*)&command);
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

void displayTask(void *pdata)
{
    while (1) {
     char *msg = (char*)receiveMessage(lcdQueue, lcdSem);
	 if(msg != NULL) {
		printf("%s%s\n", DISPLAY_PFX, msg);
	 }

	OSTimeDly(50);
    }
}

void motorTask(void *pdata)
{
	int radius = 2;
    while (1) {
		INT8U* cmd = (INT8U*)receiveMessage(motorQueue, motorSem);

		if (cmd != 0) {
			char ledBuf[100];
			char lcdBuf[100];
			switch (*cmd) {
				case MOVE_TILE_FORWARD_MESSAGE:
					printf("%sMove one tile forward\n", MOTOR_PFX);
					sprintf(ledBuf, "%s01010101", LED_PFX);
					sprintf(lcdBuf, "%s%sMoving FWD", LCD_PFX, LINE_2_PFX);
					sendMessage(lcdQueue, lcdSem, ledBuf);
					sendMessage(lcdQueue, lcdSem, lcdBuf);
					stopAfterDelay(MIN_DELAY, 1);
					break;
				case MOVE_TILE_BACKWARD_MESSAGE:
					printf("%sMove one tile backwards\n", MOTOR_PFX);
					sprintf(ledBuf, "%s11001100", LED_PFX);
					sprintf(lcdBuf, "%s%sMoving BK", LCD_PFX, LINE_2_PFX);
					sendMessage(lcdQueue, lcdSem, ledBuf);
					sendMessage(lcdQueue, lcdSem, lcdBuf);
					stopAfterDelay(MIN_DELAY, 1);
					break;
				case TURN_90_CW_MESSAGE:
					printf("%sTurn 90 degrees clockwise\n", MOTOR_PFX);
					sprintf(ledBuf, "%s00111100", LED_PFX);
					sprintf(lcdBuf, "%s%sTurning CW", LCD_PFX, LINE_2_PFX);
					sendMessage(lcdQueue, lcdSem, ledBuf);
					sendMessage(lcdQueue, lcdSem, lcdBuf);
					stopAfterDelay(MIN_DELAY, 1);
					break;
				case PERFORM_CIRCLE_CW_MESSAGE:
					printf("%sMoving in a clockwise circle of radius %d\n", MOTOR_PFX, radius);
					sprintf(ledBuf, "%s00111100", LED_PFX);
					sprintf(lcdBuf, "%s%sTurning CW", LCD_PFX, LINE_2_PFX);
					sendMessage(lcdQueue, lcdSem, ledBuf);
					sendMessage(lcdQueue, lcdSem, lcdBuf);
					stopAfterDelay(MIN_DELAY * (radius - 1), 1); // Make the delay proportional to the radius
					break;
				case PERFORM_CIRCLE_CCW_MESSAGE:
					printf("%sMoving in a counter-clockwise circle of radius %d\n", MOTOR_PFX, radius);
					sprintf(ledBuf, "%s11000011", LED_PFX);
					sprintf(lcdBuf, "%s%sTurning CCW", LCD_PFX, LINE_2_PFX);
					sendMessage(lcdQueue, lcdSem, ledBuf);
					sendMessage(lcdQueue, lcdSem, lcdBuf);
					stopAfterDelay(MIN_DELAY * (radius - 1), 1); // Make the delay proportional to the radius
					break;
				case INCREASE_CIRCLE_RADIUS_MESSAGE:
					radius++;
					if(radius == 5) {
						radius = 2;
					}
					printf("%sCircle radius changed to %d\n", MOTOR_PFX, radius);
					sprintf(lcdBuf, "%s%sRadius: %d", LCD_PFX, LINE_2_PFX, radius);
					sendMessage(lcdQueue, lcdSem, lcdBuf);
					stopAfterDelay(MIN_DELAY, 0);
					break;
				default:
					printf("%sInvalid command\n");
					break;
			}
		}

		OSTimeDly(50);
    }
}

// Stops the robot after the given delay in ticks. If the robot was
// moving, prints a message on behalf of the motor task and sets
// the LED pattern.
void stopAfterDelay(int delay, int wasMoving) {
	OSTimeDly(delay); // Wait an arbitrary amount of time
	if (wasMoving) {
		printf("%sMotors have stopped\n", MOTOR_PFX);
		char ledBuf[100];
		sprintf(ledBuf, "%s00000000", LED_PFX);
		sendMessage(lcdQueue, lcdSem, ledBuf);
	}
	char lcdBuf[200];
	sprintf(lcdBuf, "%s%sStopped", LCD_PFX, LINE_2_PFX);
	sendMessage(lcdQueue, lcdSem, lcdBuf);
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
