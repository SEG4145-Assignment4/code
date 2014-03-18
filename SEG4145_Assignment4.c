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

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStartStk[TASK_STK_SIZE];			/* Start task's stack						 */
OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];         /* Stacks for other (child) tasks				 */
INT8U TaskData[N_TASKS];				/* Parameters to pass to each task                     */
OS_EVENT* queue;
char dummyArr[5];
void* TaskPointers[N_TASKS] = { lcdTask, Task2 };	/* Function pointers to the tasks */

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

    /*
     * Here we create all other tasks (threads)
     */
	void* pointer = (void*)dummyArr;
	queue = OSQCreate(&pointer, 5);
	if (queue == 0) printf("LOL U SUK");
    TaskStartCreateTasks();

    INT8U circle_cw = 0; //true if circle goes clockwise, false if circle goes counter-clockwise
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
                        circle_cw = (circle_cw + 1) % 2;
                    }
                    break;
                case '2':
                    if (!mode) {
                        command = TURN_90_CW_MESSAGE;
                        sendMessageThingy(queue, &command);
                    }
                    else {
                        command = PERFORM_CIRCLE_CW_MESSAGE + circle_cw;
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
    INT8U whoami = *(int*) pdata;
    INT8U counter = whoami % 2;

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
	/*int i = 0;
	char msg = 'a';
	INT8U* err;*/
	int mode = 1;
	int radius = 2;
	int direction = 0; //0 for clockwise, 1 for counterclockwise
    while (1) {
		/*if (i == 0) {
			INT8U result = OSQPost(queue, (void*)&msg);
			switch (result) {
				case OS_NO_ERR:
					printf("Message was deposited in the queue.\n");
					break;
				case OS_Q_FULL:
					printf("No room in the queue.\n");
					break;
				case OS_ERR_EVENT_TYPE:
					printf("pevent is not pointing to a message queue.\n");
					break;
				case OS_ERR_PEVENT_NULL:
					printf("pevent is a NULL pointer.\n");
					break;
				case OS_ERR_POST_NULL_PTR:
					printf("msg is a NULL pointer.\n");
					break;
				}
			i++;
		} else {
			char rec[2];
			printf("getting value\n");

			char* returned = (char*)OSQPend(queue, 0, err);

			if (returned != 0) {
				printf("got value\n");
				rec[0] = *returned;
				rec[1] = '\0';
				printf("set null terminator\n");
				printf(rec);
				printf("printed\n");
			} else {
				printf("Didn't get notang.\n");
				INT8U error = *err;
				switch (error){
					case OS_NO_ERR:
						printf("Message was received");
						break;
					case OS_TIMEOUT:
						printf("Message was not received within the specified timeout.");
						break;
					case OS_ERR_EVENT_TYPE:
						printf("pevent is not pointing to a message queue");
						break;
					case OS_ERR_PEVENT_NULL:
						printf("pevent is a NULL pointer");
						break;
					case OS_ERR_PEND_ISR:
						printf("This function was called from an ISR and uC/OS-II must suspend the task. To avoid this don't call this function from an ISR.");
						break;
				}
			}
			i--;
		}*/
		char *cmd = "aaaaa"; //TODO read from message queue
		if(cmd != NULL) {
			if(mode == 1) {
				//TODO add to lcd message queue ("Mode " + mode)
				if(strcmp(cmd, "0")) {
					printf("Move one tile forward\n");
				}
				else if(strcmp(cmd, "1")) {
					printf("Move one tile backwards\n");
				}
				else if(strcmp(cmd, "2")) {
					printf("Turn 90 degrees clockwise\n");
				}
				else if(strcmp(cmd, "3")) {
					mode = 2;
					printf("Changing to mode 2\n");
					//TODO add to lcd message queue ("Mode " + mode)
				}
				else {
					printf("Invalid command\n");
				}
			}
			else { //mode == 2
				if(strcmp(cmd, "0")) {
					radius++;
					if(radius == 5) {
						radius = 2;
					}
					printf("Circle radius changed to ");
					printf("%d", radius);
					printf("\n");
				}
				else if(strcmp(cmd, "1")) {
					if(direction == 0) {
						direction = 1;
						printf("Changing circle direction to counter-clockwise\n");
					}
					else {
						direction = 0;
						printf("Changing circle direction to clockwise\n");
					}
				}
				else if(strcmp(cmd, "2")) {
					printf("Moving ");
					if(direction == 1) {
						printf("counter-");
					}
					printf("clockwise in a circle with radius ");
					printf("%d", radius);
					printf("\n");
				}
				else if(strcmp(cmd, "3")) {
					mode = 1;
					printf("Changing to mode 1\n");
					//TODO add to lcd message queue ("Mode " + mode)
				}
				else {
					printf("Invalid command\n");
				}
			}
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
