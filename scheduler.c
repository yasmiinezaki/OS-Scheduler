#include "headers.h"

#define MAX 50

struct message
{
     long m_type;
     int message_data[4];
};

int main(int argc, char *argv[])
{
    //TODO: implement the scheduler.
    //TODO: upon termination release the clock resources.
    initClk();

    //Variables received through execv() of process_generator:
    int processes_number = atoi(argv[1]);
    int algo_number = atoi(argv[2]);
    int quantum = atoi(argv[3]);

    //Message variables:
    key_t key_sch_pgen=33; //key associated with the message queue
    //use IPC_CREAT to create the message queue if it does not exist
    msgq_id = msgget(key_sch_pgen, 0666 | IPC_CREAT);  // creating message queue
    struct message msg;
    int received; //to store return of msgrcv 

    if(msgq_id==-1)
    {
        perror("Error in creation");
        exit(-1);
    }

    //Structs and general variables:
    Queue *readyQueue=createQueue();
    Node *Node_to_insert;
    Node *p_executing=NULL; //process that will be executing
    int received_number=0; //number of processes received so far
    int time=getClk();
    int nexttime;
    int *p_PIDS = (int *)malloc(processes_number * sizeof(int)); //For process PIDs
    int indexPID = 0;

//Algorithm execution
switch(algo_number)
{
    case 1: //Shortest Job First

    break;

    case 2: //Preemptive Highest Priority First
    break;

    case 3: //Round Robin
    //While there are still processes in the ready queue or there are still processes to be recieved
    while(!isEmpty(&readyQueue) || (received_number<processes_number))
    {
        //We will break out of the below loop when we do not receive any message and our readyQueue is not empty
        do
        {
            //Do not wait for a message
            received=msgrcv(msgq_id,&msg,sizeof(msg.message_data), 0, IPC_NOWAIT);

            //msgrcv returns -1 if no message was received
            if(isEmpty(&readyQueue) && received==-1) //no processes present to perform
            {
                printf("Ready queue is empty. Waiting to receive a new process...\n")
                printf("Current time : %d \n",getClk());
                printf("Total processes received till now : %d\n", received_number);
                printf("Remaining processes that have still not arrived : %d",processes_number-received_number);
                //wait for a message
                received=msgrcv(msgq_id,&msg,sizeof(msg.message_data), 0, !IPC_NOWAIT);
            }

            //if a message has been received
            if(received != -1) 
            {
                
                Node_to_insert=newNode(msg.message_data[0],msg.message_data[1],msg.message_data[2],msg.message_data[3], WAITING);       
                enqueueRR(readyQueue,Node_to_insert); //create fn to enqueue a node with these info FIFO
                received_number++;
            }
        }while(received != -1); //since different processes can have the same arrival time, if I received a message enter to check if I will receive another one as well

    nexttime=getClk();

    //This below block is called every 1 second:
    if(nexttime>time)
    {
        time=nexttime;
        int PID,Status;
        
        //If there is a process that is executing and its state is not finished then enqueue it again
        if(p_executing && p_executing->Status != FINISHED)
        {
            enQueueRR(&readyQueue,p_executing);
        }

        p_executing=peek_queue(readyQueue);
        deQueue(readyQueue);

        printf("Process in execution is with ID %d \n",p_executing->ID);
        
        if(p_executing->STATUS == WAITING)
        {
            PID=fork();
            if(PID==0)
            {
                char buff1[5]; //for ID
                char buff2[5]; //for Runtime
                sprintf(buff1, %d, p_executing->ID);
                sprintf(buff2, %d, p_executing->Runtime);
                argv[1]=buff1;
                argv[2]=buff2;
                p_executing->Start_time=getClk();

                if(execv("./process.out",argv)==-1)
                    perror("Failed to execv");
            }
            //First time for process to run:
            else
            {
                //TODO: calculate Waiting_time. Put info in output file
                p_PIDS[indexPID]=PID;
                indexPID++;
                p_executing->Start_time=getClk();
                //signal wait   
            }        
        }
        //if process was stopped then resume its processing
        else if(p_executing->STATUS == STOPPED)
        {
            p_executing->STATUS = CONTINUE;
        }


    }
    break;

    case 4: //Multiple level Feedback Loop
    break;

}


destroyClk(true);
}
