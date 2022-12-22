#include "headers.h"

#define MAX 50

struct message
{
    long m_type;
    int message_data[4];
};
int SIGUSR1_handler(int signum);
int SIGUSR2_handler(int signum);

int main(int argc, char *argv[])
{
    // ignore sigusr1
    signal(SIGUSR1, SIGUSR1_handler);
    signal(SIGUSR2, SIGUSR2_handler);
    // TODO: implement the scheduler.
    // TODO: upon termination release the clock resources.
    initClk();

    // Variables received through execv() of process_generator:
    int processes_number = atoi(argv[1]);
    int algo_number = atoi(argv[2]);
    int quantum = atoi(argv[3]);

    // Message variables:
    key_t key_sch_pgen = 33; // key associated with the message queue
    // use IPC_CREAT to create the message queue if it does not exist
    int msgq_id = msgget(key_sch_pgen, 0666 | IPC_CREAT); // creating message queue
    struct message msg;
    int received; // to store return of msgrcv

    if (msgq_id == -1)
    {
        perror("Error in creation");
        exit(-1);
    }

    // Output file:
    FILE *fptr;
    fptr = fopen("schedular.log", "w");
    fprintf(fptr, "#At time x process y state arr w total z remain y wait k \n");

    // Structs and general variables:
    Queue *readyQueue = createQueue();
    Node *Node_to_insert;
    Node *p_executing = NULL; // process that will be executing
    int received_number = 0;  // number of processes received so far
    int time = getClk();
    int nexttime;
    int *p_PIDS = (int *)malloc(processes_number * sizeof(int)); // For process PIDs
    int indexPID = 0;
    int finishedProcesses = 0;

    // MLFQ vars
    int current_level;
    int stat_loc;
    int finished_processes;
    int *original_priorities = (int *)malloc(processes_number * sizeof(int));
    Queue *tempQueue = createQueue(); // used to in case a new processes is inserted at a higher level and nedd to be excuted first

    // statistics
    int useful_time=0;
    int avgwta =0;
    int avgwait =0;
    int total_time =0;

    Queue *finishedQueue = createQueue();
    int totalRuntime = 0;

    // Algorithm execution
    switch (algo_number)
    {

    case 1: // Shortest Job First
        while (!isEmpty(readyQueue) || (received_number < processes_number))
        {
            // We will break out of the below loop when we do not receive any message and our readyQueue is not empty
            do
            {
                // Do not wait for a message
                received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, IPC_NOWAIT);

                // msgrcv returns -1 if no message was received
                if (isEmpty(readyQueue) && received == -1) // no processes present to perform
                {
                    printf("Ready queue is empty. Waiting to receive a new process...\n");
                    printf("Current time : %d \n", getClk());
                    printf("Total processes received till now : %d\n", received_number);
                    printf("Remaining processes that have still not arrived: %d", processes_number - received_number);
                    printf("\n");
                    // wait for a message
                    received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, !IPC_NOWAIT);
                }

                // if a message has been received
                if (received != -1)
                {
                    // printf("Process with ID %d has just arrived\n",msg.message_data[0]);
                    Node_to_insert = newNode(msg.message_data[0], msg.message_data[1], msg.message_data[2], msg.message_data[3], WAITING);
                    enQueueSJF(readyQueue, Node_to_insert); // create fn to enqueue a node with these info FIFO
                    received_number++;
                }
            } while (received != -1); // since different processes can have the same arrival time, if I received a message enter to check if I will receive another one as well

            int pid;
            if (!isEmpty(readyQueue))
            {
                int clockk = getClk();
                int current_child_pid;
                p_executing = peek_queue(readyQueue);
                int burst_time = p_executing->Runtime;
                char buff1[5];
                sprintf(buff1, "%d", burst_time);
                argv[1] = buff1;
                printf("%d", atoi(argv[1]));
                deQueue(readyQueue);
                pid = fork();
                if (pid != 0)
                    current_child_pid = pid;
                if (pid == 0)
                {
                    if ((execv("./process.out", argv)) == -1)
                        perror("error:");
                }
                else
                {
                    int status;
                    // kill(current_child_pid, SIGUSR2);
                    waitpid(current_child_pid, &status, 0);
                    printf("Process with ID = %d has finished", p_executing->ID);
                    printf("\n");
                }
            }
        }
        break;

        // MARK
    case 2: // Preemptive Highest Priority First

        printf("\n\n\n\n\n\n\n\n");

        printf("HPF inside scheduler\n");
        // While there are still processes in the ready queue or there are still processes to be recieved
        while (!isEmpty(readyQueue) || (received_number < processes_number))
        {
            // We will break out of the below loop when we do not receive any message and our readyQueue is not empty
            do
            {
                // Do not wait for a message
                received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, IPC_NOWAIT);

                // msgrcv returns -1 if no message was received
                if (isEmpty(readyQueue) && received == -1) // no processes present to perform
                {
                    printf("Ready queue is empty. Waiting to receive a new process...\n");
                    printf("Current time : %d \n", getClk());
                    printf("Total processes received till now : %d\n", received_number);
                    printf("Remaining processes that have still not arrived : %d\n", processes_number - received_number);
                    // wait for a message
                    received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, !IPC_NOWAIT);
                }

                // if a message has been received
                // printf("recieved is %d\n", received);
                if (received != -1)
                {
                    printf("Process with ID %d has just arrived\n", msg.message_data[0]);
                    Node_to_insert = newNode(msg.message_data[0], msg.message_data[1], msg.message_data[2], msg.message_data[3], WAITING);
                    enQueueHPF(readyQueue, Node_to_insert); // create fn to enqueue a node with these info FIFO
                    received_number++;
                }
            } while (received != -1); // since different processes can have the same arrival time, if I received a message enter to check if I will receive another one as well

            nexttime = getClk();
            // printf("next time= %d\n", nexttime);
            // printf("currnt time=%d\n", time);
            // This below block is called every 1 second:
            if (nexttime > time)
            {
                time = nexttime;

                // If there is a process that is executing and there is another process with a higher priority we change the current executing process to it
                if (p_executing && peek_queue(readyQueue)->ID != p_executing->ID)
                {
                    // changing the current running process to the highest priority process
                    p_executing = peek_queue(readyQueue);
                }
                else
                {
                    p_executing = peek_queue(readyQueue);
                }

                // First time for process to run:
                if (p_executing->Status == WAITING)
                {

                    fflush(stdout);
                    pid_t PID = fork();
                    printf("////////////PID is %d///////////\n", PID);

                    fflush(stdout);

                    p_executing->Start_time = getClk();

                    if (PID == 0)
                    {
                        char buff1[5]; // for ID
                        char buff2[5]; // for Runtime
                        sprintf(buff1, "%d", p_executing->Runtime);
                        sprintf(buff2, "%d", p_executing->ID);
                        argv[1] = buff1;
                        argv[2] = buff2;

                        printf("\nI AM THE CHILLLD\n");

                        if (execv("./process.out", argv) == -1)
                        {
                            printf("help\n");
                            perror("Failed to execv\n");
                        }
                        exit(0);
                    }
                    else
                    {
                        // TODO: calculate Waiting_time. Put info in output file
                        // printf("created process with PID=%d for process ID=%d\n", PID, p_executing->ID);
                        p_PIDS[indexPID] = PID;
                        indexPID++;
                        p_executing->Start_time = getClk();
                        p_executing->PID = PID;
                        // change status to stopped and make process wait for execution
                        p_executing->Status = STOPPED;
                        kill(p_executing->PID, SIGUSR1);
                    }
                }
                // int status;
                // if (waitpid(p_executing->PID, &status, 0))
                //     if (WIFEXITED(status))
                //         printf("child exited with status of %d\n", WEXITSTATUS(status));
                //     else
                //         puts("child did not exit successfully");

                // run the process for 1 clock cycle
                // kill(p_executing->PID, SIGUSR1); // pause process
                kill(p_executing->PID, SIGUSR2); // run process
                sleep(1);                        // wait one clock cycle
                kill(p_executing->PID, SIGUSR1); // pause process
                printf("Process in execution is with ID %d \n", p_executing->ID);
                int x = kill(p_executing->PID, 0);
                // printf("finished?= %d\n", x);
                // remove the process that ended from the queue
                if (kill(p_executing->PID, 0) != 0)
                {
                    printf("finished process %d\n ", p_executing->ID);
                    p_executing = NULL;
                    deQueue(readyQueue);
                }
            }
        }

        break;
    case 3: // Round Robin

        // While there are still processes in the ready queue or there are still processes to be recieved
        while (!isEmpty(readyQueue) || (received_number < processes_number))
        {
            // We will break out of the below loop when we do not receive any message and our readyQueue is not empty
            do
            {
                // Do not wait for a message
                received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, IPC_NOWAIT);

                // msgrcv returns -1 if no message was received
                if (isEmpty(readyQueue) && received == -1) // no processes present to perform
                {
                    printf("Ready queue is empty. Waiting to receive a new process...\n");
                    printf("Current time : %d \n", getClk());
                    printf("Total processes received till now : %d\n", received_number);
                    printf("Remaining processes that have still not arrived : %d \n", processes_number - received_number);
                    // wait for a message
                    received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, !IPC_NOWAIT);
                }

                // if a message has been received
                if (received != -1)
                {
                    Node_to_insert = newNode(msg.message_data[0], msg.message_data[1], msg.message_data[2], msg.message_data[3], WAITING);
                    enQueueRR(readyQueue, Node_to_insert);
                    received_number++;
                    printf("Current ready queue : ");
                    printqueue(readyQueue);
                    printf("\n");
                }
            } while (received != -1); // since different processes can have the same arrival time, if I received a message enter to check if I will receive another one as well

            nexttime = getClk();

            // This below block is called every 1 second:
            if (nexttime > time)
            {
                time = nexttime;
                int PID;

                // If there is a process that is executing and its state is not finished then enqueue it again
                if (p_executing && p_executing->Status != FINISHED)
                {
                    enQueueRR(readyQueue, p_executing);
                    printf("Current ready queue : ");
                    printqueue(readyQueue);
                    printf("\n");
                }

                p_executing = peek_queue(readyQueue);
                deQueue(readyQueue);

                printf("Process in execution is with ID %d \n", p_executing->ID);

                if (p_executing->Status == WAITING)
                {
                    PID = fork();
                    totalRuntime += p_executing->Runtime;

                    if (PID == 0)
                    {
                        char buff1[5]; // for ID
                        char buff2[5]; // for Runtime
                        sprintf(buff1, "%d", p_executing->ID);
                        sprintf(buff2, "%d", p_executing->Runtime);
                        argv[1] = buff1;
                        argv[2] = buff2;
                        p_executing->Start_time = getClk();

                        if (execv("./process.out", argv) == -1)
                            perror("Failed to execv for process.c");
                    }
                    // First time for process to run:
                    else
                    {
                        // TODO: calculate Waiting_time. Put info in output file
                        p_PIDS[indexPID] = PID;
                        indexPID++;
                        p_executing->Start_time = getClk();
                        printf("Starting process with ID %d and PID %d\n", p_executing->ID, PID);

                        // Write to output file:
                        p_executing->Waiting_time = p_executing->Start_time - p_executing->Arrival;
                        fprintf(fptr, "At time  %d  process %d started arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);
                    }
                }
                // if process was stopped then resume its processing
                else if (p_executing->Status == STOPPED)
                {
                    // change process status
                    p_executing->Status = CONTINUE;
                    //printf("Resuming process with ID %d and PID %d\n", p_executing->ID, p_PIDS[p_executing->ID - 1]);

                    // Write to output file
                    p_executing->Waiting_time += getClk() - p_executing->Stopped_time;
                    fprintf(fptr, "At time  %d  process %d started arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);

                    // continue the process
                    kill(p_PIDS[p_executing->ID - 1], SIGUSR2);
                }

                /* Two possibilities:
                1. The remaining time is greater than one quantum. Therefore, run the process for one quantum and stop.
                2. The remaining time is less than one quantum. Therefore, run the process for the remaining time and stop.
                */

                // 1.
                if (p_executing->Remaining_time > quantum)
                {
                    // Sleep for one quantum period
                    sleep(quantum);

                    // stop the process
                    printf("Stopping process with ID %d and PID %d...\n", p_executing->ID, p_PIDS[p_executing->ID - 1]);
                    kill(p_PIDS[p_executing->ID - 1], SIGUSR1);
                    printf("Process with ID %d and PID %d has stopped\n", p_executing->ID, p_PIDS[p_executing->ID - 1]);

                    // decrease remaining time by one qnatum period
                    p_executing->Remaining_time -= quantum;

                    // change the process status
                    p_executing->Status = STOPPED;

                    // Record the time at which the process has stopped
                    p_executing->Stopped_time = getClk();

                    // write to file
                    fprintf(fptr, "At time  %d  process %d started arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);

                    // Check if this is the only and last process in the ready queue then run it as batch (until it finishes)
                    if (isEmpty(readyQueue) && finishedProcesses == processes_number - 1)
                    {
                        printf("Process with ID %d and PID %d is the last process, executing till the end\n", p_executing->ID, p_PIDS[p_executing->ID - 1]);

                        // Sleep for the remaining time
                        sleep(p_executing->Remaining_time);

                        // Update remaining time
                        p_executing->Remaining_time = 0;

                        // change the process status
                        p_executing->Status = FINISHED;

                        // Update finish time
                        p_executing->Finish_time = getClk();

                        // Calculate TA and WTA
                        p_executing->TA = getClk() - p_executing->Arrival;
                        p_executing->WTA = (float)p_executing->TA / p_executing->Runtime;

                        // write to file
                        fprintf(fptr, "At time  %d  process %d started arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);

                        printf("Finished process with ID %d at time %d \n", p_executing->ID, p_executing->Finish_time);
                        finishedProcesses++;
                        enQueueRR(finishedQueue, p_executing);
                        printf("Current finished queue : ");
                        printqueue(finishedQueue);
                        printf("\n");
                    }
                }

                // 2.
                else if (p_executing->Remaining_time <= quantum && p_executing->Runtime != 0)
                {
                    // Sleep for the remaining time
                    sleep(p_executing->Remaining_time);

                    // Update remaining time
                    p_executing->Remaining_time = 0;

                    // change the process status
                    p_executing->Status = FINISHED;

                    // Update finish time
                    p_executing->Finish_time = getClk();

                    // Calculate TA and WTA
                    p_executing->TA = getClk() - p_executing->Arrival;
                    p_executing->WTA = (float)p_executing->TA / p_executing->Runtime;

                    // write to file
                    fprintf(fptr, "At time  %d  process %d started arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);

                    printf("Finished process with ID %d at time %d \n", p_executing->ID, p_executing->Finish_time);
                    finishedProcesses++;
                    enQueueRR(finishedQueue, p_executing);
                    printf("Current finished queue : ");
                    printqueue(finishedQueue);
                    printf("\n");
                }
            }
        }
        break;

    case 4: // Multiple level Feedback Loop
        // While there are still processes in the ready queue or there are still processes to be recieved
        while (!isEmpty(readyQueue) || (received_number < processes_number))
        {
            // We will break out of the below loop when we do not receive any message and our readyQueue is not empty
            do
            {
                // Do not wait for a message
                received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, IPC_NOWAIT);

                // msgrcv returns -1 if no message was received
                if (isEmpty(readyQueue) && received == -1) // no processes present to perform
                {
                    //printf("Ready queue is empty. Waiting to receive a new process...\n");
                    //printf("Current time : %d \n", getClk());
                    //printf("Total processes received till now : %d\n", received_number);
                    //printf("Remaining processes that have still not arrived : %d \n", processes_number - received_number);
                    // wait for a message
                    received = msgrcv(msgq_id, &msg, sizeof(msg.message_data), 0, !IPC_NOWAIT);
                }

                // if a message has been received
                if (received != -1)
                {
                    Node_to_insert = newNode(msg.message_data[0], msg.message_data[1], msg.message_data[2], msg.message_data[3], WAITING);
                    enQueueHPF(readyQueue, Node_to_insert);
                    received_number++;
                }
            } while (received != -1); // since different processes can have the same arrival time, if I received a message enter to check if I will receive another one as well

            nexttime = getClk();
            if (nexttime > time)
            {
                time = nexttime;
                p_executing = peek_queue(readyQueue);
                deQueue(readyQueue); // dequeuing the proccess with the highest priority currently i.e. excuting the highest level existing
                
                current_level = p_executing->Priority; // updating current level
                //printf("Current level: %d\n", current_level);

                if (p_executing->Status == WAITING)
                {
                    int PID = fork();
                    totalRuntime += p_executing->Runtime;
                    p_executing->Remaining_time = p_executing->Runtime;

                    if (PID == 0)
                    {
                        char buff1[5]; // for ID
                        char buff2[5]; // for Runtime
                        sprintf(buff1, "%d", p_executing->ID);
                        sprintf(buff2, "%d", p_executing->Runtime);
                        argv[1] = buff1;
                        argv[2] = buff2;
                        p_executing->Start_time = getClk();

                        if (execv("./process.out", argv) == -1)
                            perror("Failed to execv for process.c");
                    }
                    else // First time for process to run:
                    {
                        // TODO: calculate Waiting_time. Put info in output file
                        original_priorities[p_executing->ID - 1] = p_executing->Priority;
                        p_PIDS[p_executing->ID - 1] = PID;
                        p_executing->Start_time = getClk();
                        //printf("Starting process with ID %d and PID %d\n", p_executing->ID, PID);

                        // Write to output file:
                        p_executing->Waiting_time = p_executing->Start_time - p_executing->Arrival;
                        fprintf(fptr, "At time  %d  process %d started arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);
                    }
                }
                else if (p_executing->Status == STOPPED) // if process was stopped and is continuing
                {
                    // change process status
                    p_executing->Status = CONTINUE;
                    printf("Resuming process with ID %d and PID %d\n", p_executing->ID, p_PIDS[p_executing->ID - 1]);
                    // record stopping time of process
                    p_executing->Stopped_time = getClk();
                    // Write to output file
                    p_executing->Waiting_time += getClk() - p_executing->Stopped_time;
                    fprintf(fptr, "At time  %d  process %d resumed arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);
                    // continue the process
                    kill(p_PIDS[p_executing->ID - 1], SIGUSR2);   
                }
                sleep(1); // apply quantum
                p_executing->Remaining_time -= 1;
                if (p_executing->Remaining_time == 0 ) // process is terminated i.e. remaining time =0
                {
                    finished_processes++;
                    // change process status
                    p_executing->Status = FINISHED;
                    p_executing->Finish_time = getClk();
                    //printf("process %d has finished\n", p_executing->ID);
                    // Calculate TA and WTA
                    p_executing->TA = p_executing->Finish_time - p_executing->Arrival;
                    p_executing->WTA = (float)p_executing->TA / p_executing->Runtime;
                    
                    useful_time = p_executing->Runtime + useful_time;
                    // calculating summation of avg waiting time
                    avgwait = (avgwait + (p_executing->Waiting_time));
                    // calculating summation of avg  weighted turn around time
                    avgwta = (avgwta + (p_executing->WTA));
                    // write to file
                    fprintf(fptr, "At time  %d  process %d finished arr %d total %d remain %d wait %d TA %d WTA %d  \n", p_executing->Finish_time, p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time,p_executing->TA,p_executing->WTA);
                    if (finished_processes == processes_number) total_time= getClk();
                }
                else
                {
                    // pause the process
                    kill(p_PIDS[p_executing->ID - 1], SIGUSR1); 
                    usleep(5000);
                    // change process status
                    p_executing->Status = STOPPED;
                    //printf("process %d has stopped\n", p_executing->ID);
                    fprintf(fptr, "At time  %d  process %d stopped arr %d total %d remain %d wait %d \n", getClk(), p_executing->ID, p_executing->Arrival, p_executing->Runtime, p_executing->Remaining_time, p_executing->Waiting_time);
                    
                    if (current_level == 10) // if I am at the lowest level
                    {
                        // return process to its original priority
                        p_executing->Priority = original_priorities[p_executing->ID - 1];
                        p_executing->next = NULL;
                        enQueueHPF(tempQueue,p_executing);
                        if (isEmpty(readyQueue))
                        {
                            while(!isEmpty(tempQueue))
                            {
                                p_executing = peek_queue(tempQueue);
                                deQueue(tempQueue);
                                p_executing->next = NULL;
                                enQueueHPF(readyQueue,p_executing);
                            }
                        }
                    }
                    else
                    {   // degrade priority to execute in another level
                        p_executing->Priority = p_executing->Priority +1;
                        p_executing->next = NULL;
                        enQueueHPF(readyQueue,p_executing);
                    }
                }
            }
        }
                break;
    }
    destroyClk(false);
    return 0;
}


// testing for process.c
int SIGUSR1_handler(int signum)
{
    //printf("received pause\n");
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = SIGUSR2_handler;
    sigaction(SIGUSR2, &sigact, NULL);
    pause();
}
int SIGUSR2_handler(int signum)
{
    //printf("recieved continue\n");
}