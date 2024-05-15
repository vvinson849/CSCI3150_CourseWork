/*
* CSCI3150 Introduction to Operating Systems
*
* --- Declaration ---
* For all submitted files, including the source code files and the written
* report, for this assignment:
*
* I declare that the assignment here submitted is original except for source
* materials explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/
*
*
* Source material acknowledgements (if any):
*
* Students whom I have discussed with (if any):
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void outprint(int time_x, int time_y, int pid, int arrival_time, int remaining_time);

void SortProcess(Process* p, int num);

void getArrival(int time_x, int time_y, Process* proc, LinkedQueue** ProcessQueue, int proc_num, int queue_num);

void sortByPid(Process* p, int num);

int allQueueEmpty(LinkedQueue** ProessQueue, int queue_num);

// Implement by students
void scheduler(Process* proc, LinkedQueue** ProcessQueue, int proc_num, int queue_num, int period) {

    printf("Process number: %d\n", proc_num);
    for(int i=0; i<proc_num; i++)
        printf("%d %d %d\n", proc[i].process_id, proc[i].arrival_time, proc[i].execution_time);

    printf("\nQueue number: %d\n", queue_num);
    printf("Period: %d\n", period);
    for(int i=0; i<queue_num; i++) 
        printf("%d %d %d\n", i, ProcessQueue[i]->time_slice, ProcessQueue[i]->allotment_time);
    
/*     
       an example of outprint function,
       it will output "Time_slot:1-2, pid:3, arrival-time:4, remaining_time:5" to output.log file.
   
    outprint(1,2,3,4,5);
 */

    int S = 0; // current time
    int period_value = period;

    while(1) {
        /* retrieve all arrival processes */
        getArrival(S, S+1, proc, ProcessQueue, proc_num, queue_num);

        /* periodical boost */
        if(S % period_value == 0 && S > 0) {
            int total_len_ProcessQueue = 0;
            for(int i=0; i<queue_num; ++i)
                total_len_ProcessQueue += Length(ProcessQueue[i]);
            Process* allProc = (Process*)malloc(total_len_ProcessQueue * sizeof(Process));
            int k = 0;
            for(int i=0; i<queue_num; ++i) { // retrieve all the process
                int len = Length(ProcessQueue[i]);
                for(int j=0; j<len; ++j)
                    allProc[k++] = DeQueue(ProcessQueue[i]);
            }
            sortByPid(allProc, total_len_ProcessQueue);
            for(int i=0; i<total_len_ProcessQueue; ++i) { // put back the sorted process into the highest prority queue
                allProc[i].service_time = 0;
                ProcessQueue[queue_num-1] = EnQueue(ProcessQueue[queue_num-1], allProc[i]);
            }
            if(allProc!=NULL)
                free(allProc);
        }
        
        /* Proceed to S+1 if all ProcessQueues are empty */
        if(allQueueEmpty(ProcessQueue, queue_num)) {
            ++S;
            continue;
        }

        /* retrieve the process for this time slice */
        Process current_proc;
        int current_queue;
        for(int i=queue_num-1; i>=0; --i)
            if(!IsEmptyQueue(ProcessQueue[i])) {
                current_proc = DeQueue(ProcessQueue[i]);
                current_queue = i;
                break;
            }

        /* schedule the process */
        int time_x = S, 
            time_y,
            pid = current_proc.process_id,
            arrival_time = current_proc.arrival_time,
            remain_time,
            this_round_exec_time;

        if(current_proc.execution_time < ProcessQueue[current_queue]->time_slice)
            time_y = S + current_proc.execution_time;
        else
            time_y = S + ProcessQueue[current_queue]->time_slice;

        if(time_x<period && time_y>=period) { // check if being the time for periodical boost
            time_y = period;
            period += period_value; // the time for next boost
        }

        // update the attributes of current process
        this_round_exec_time = time_y - time_x;
        current_proc.execution_time -= this_round_exec_time;
        remain_time = current_proc.execution_time;
        current_proc.service_time += this_round_exec_time;
        if(remain_time == 0) {
            current_proc.completion_time = time_y;
            current_proc.turnaround_time = current_proc.completion_time - current_proc.arrival_time;
        }

        outprint(time_x, time_y, pid, arrival_time, remain_time);

        S += this_round_exec_time; // update S

        for(int i=0; i<queue_num; ++i) // update the waiting time of the processes in the ProcessQueue
            for(LinkedQueue* queue = ProcessQueue[i];
                queue != NULL;
                queue = queue->next)
                queue->proc.waiting_time += this_round_exec_time;

        /* retrieve the processes arrived during the current process running */
        getArrival(time_x+1, time_y, proc, ProcessQueue, proc_num, queue_num);

        if(current_proc.execution_time > 0 && // lower the priority if allotment has been used up
            current_proc.service_time >= ProcessQueue[current_queue]->allotment_time &&
            current_queue > 0) { 
            current_proc.service_time = 0; // reset the allotment time of the process
            ProcessQueue[current_queue-1] = EnQueue(ProcessQueue[current_queue-1], current_proc);
        } else if(current_proc.execution_time > 0)
            ProcessQueue[current_queue] = EnQueue(ProcessQueue[current_queue], current_proc);

        /* continue to execute if there is any process in any ProcessQueue */
        if(!allQueueEmpty(ProcessQueue, queue_num))
            continue;

        /* continue to execute if there is any process coming in the future */
        for(int i=0; i<proc_num; ++i) 
            if(proc[i].arrival_time >= S)
                continue;
        
        break;

    }

}

void getArrival(int time_x, int time_y, Process* proc, LinkedQueue** ProcessQueue, int proc_num, int queue_num) {
    int count = 0;
    for(int i=0; i<proc_num; ++i) // count how many arrival processes
        if(proc[i].arrival_time>=time_x && proc[i].arrival_time<time_y)
            ++count;

    if(count > 0) {
        Process* tmpQueue = (Process*)malloc(count * sizeof(Process)); // temp queue for the arrival processes
        int j = 0;
        for(int i=0; i<proc_num; ++i) 
            if(proc[i].arrival_time>=time_x && proc[i].arrival_time<time_y)
                tmpQueue[j++] = proc[i];

        SortProcess(tmpQueue, count); // sort the arrival processes according to their pid and arrival time

        for(int i=0; i<count; ++i) { // enqueue the arrival processes to the highest priority queue
            tmpQueue[i].service_time = 0; // initialise the allotment time of the process
            ProcessQueue[queue_num-1] = EnQueue(ProcessQueue[queue_num-1], tmpQueue[i]);
        }
        free(tmpQueue);
    }
}

void sortByPid(Process* p, int num) {
    /* simple selection sort by process id */
    for(int i=0; i<num; ++i) {
        int min = i;
        for(int j=i; j<num; ++j)
            if(p[j].process_id < p[min].process_id)
                min = j;
        Process tmp = p[min];
        p[min] = p[i];
        p[i] = tmp;
    }
}

int allQueueEmpty(LinkedQueue** ProcessQueue, int queue_num) {
    for(int i=0; i<queue_num; ++i)
        if(!IsEmptyQueue(ProcessQueue[i]))
            return 0;
    return 1;
}
