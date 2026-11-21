#include <stdio.h>
#include <stdlib.h>

#define MAX_P 100
#define IDLE -1

typedef struct {
    int pid; // 1-based indexing for processes
    int arrival; // Arrival Time
    int burst; // Burst Time 
    int remaining; // Time left from Burst to be executed
    int completion; // Completion Time
    int turnaround; // Turnaround Time = Completion Time - Arrival Time
    int waiting; // Waiting Time = Turnaround Time - Burst Time 
} Process;

typedef struct {
    int pid;  // IDLE for CPU idle 
    int start;
    int end;
} Slice;

// Circular Ready Queue
int queue[MAX_P + 1];
int front = 0, rear = 0, count = 0;

void enqueue(int idx) {
    queue[rear] = idx;
    rear = (rear + 1) % (MAX_P + 1);
    count++;
}

int dequeue(void) {
    int idx = queue[front];
    front = (front + 1) % (MAX_P + 1);
    count--;
    return idx;
}

// Sort processes by arrival time (ties broken by pid)
// Selection Sort
void sort_by_arrival(Process p[], int n) {
    Process temp;
    for (int i = 0; i < n-1; i++) {
        int min_idx = i;
        for(int j = i+1; j < n; j++) {
            if(p[min_idx].arrival > p[j].arrival || ((p[min_idx].arrival == p[j].arrival) && p[min_idx].pid > p[j].pid)) {
                min_idx = j;
            }
        }
        if(min_idx != i) {
            temp = p[i];
            p[i] = p[min_idx];
            p[min_idx] = temp;
        }
    }
}

// Core Round Robin
int round_robin(Process p[], int n, int quantum, Slice gantt[]) {
    int time = 0; // the simulated clock
    int next = 0; // index of next process that hasn't entered the queue yet
    int done = 0; // how many processes have finished 
    int g = 0; // how many gantt slices recoreded so far

    // sort all processes with respect to their arrival time
    sort_by_arrival(p, n);

    // initialize remianing with burst
    for(int i = 0; i < n; i++) {
        p[i].remaining = p[i].burst; 
    }

    // admit/enqueue all the processes that has enetered at time = 0
    while(next < n && p[next].arrival <= time) {
        enqueue(next);
        next++;
    }

    while(done < n) {
        if(count == 0) { // cpu is idle and not executing : jump to next arrival
            int t = p[next].arrival;
            gantt[g++] = (Slice){IDLE, time, t};
            time = t;
            while(next < n && p[next].arrival <= time){
                enqueue(next);
                next++;
            }
            continue; // ? Why is it here
        }

        int idx = dequeue();
        int run = (p[idx].remaining < quantum)? p[idx].remaining : quantum;
        
        gantt[g++] = (Slice){p[idx].pid, time, time + run};
        time += run;
        p[idx].remaining -= run;

        // new arrivals should be enqueued before the preempted process
        while(next < n && p[next].arrival <= time) {
            enqueue(next);
            next++;
        }

        if(p[idx].remaining > 0){
            enqueue(idx);
        }
        else {
            p[idx].completion = time;
            p[idx].turnaround = p[idx].completion - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            done++; 
        }
    }
    return g;
}

int merge_gantt(Slice gantt[], int g) {
    int m = 0; // write position
    for(int i = 0; i < g; i++){
        if(m > 0 && gantt[m-1].pid == gantt[i].pid && gantt[m-1].end == gantt[i].start)
            gantt[m-1].end = gantt[i].end; // same process continues: extend the block
        else
            gantt[m++] = gantt[i]; // different process: start a new block
    }
    return m; // new number of blocks
}
    
void print_gantt(Slice gantt[], int g) {
    // Row 1: the bar with process labels 
    printf("\nGantt Chart :\n|");

    for (int i = 0; i < g; i++) {
        int width = (gantt[i].end - gantt[i].start) * 2;
        if (width < 6)
            width = 6;

        char label[16];
        int len;
        if (gantt[i].pid == IDLE)
            len = snprintf(label, sizeof(label), "IDLE");
        else
            len = snprintf(label, sizeof(label), "P%d", gantt[i].pid);

        int left  = (width - len) / 2;
        int right = width - len - left;

        printf("%*s%s%*s|", left, "", label, right, "");
    }

    // Row 2: time labels under each border 
    char num[16];
    int prev_len = snprintf(num, sizeof(num), "%d", gantt[0].start);
    printf("\n%s", num); // first time, no extra padding

    for (int i = 0; i < g; i++) {
        int width = (gantt[i].end - gantt[i].start) * 2;
        if (width < 6)
            width = 6;

        int len = snprintf(num, sizeof(num), "%d", gantt[i].end);
        printf("%*s%s", width + 1 - prev_len, "", num);
        prev_len = len; // remember for the next padding
    }

    printf("\n");
}

void print_results(Process p[], int n, Slice gantt[], int g){
    double sum_wt = 0, sum_tat = 0;

    //print in pid order
    printf("\nPID\tAT\tBT\tCT\tTAT\tWT\n");

    for(int pid = 1; pid <= n; pid++){
        for(int i = 0; i < n; i++){
            if(p[i].pid == pid) {
                printf("P%d\t%d\t%d\t%d\t%d\t%d\n", p[i].pid, p[i].arrival, p[i].burst, p[i].completion, p[i].turnaround, p[i].waiting);
                sum_wt += p[i].waiting;
                sum_tat += p[i].turnaround;
            }
        }
    }

    int busy = 0;
    for(int i = 0; i < g; i++){
        if(gantt[i].pid != IDLE)
            busy += gantt[i].end - gantt[i].start;
    }

    int total = gantt[g-1].end - gantt[0].start;

    printf("\nAverage Waiting Time      : %.2f\n", sum_wt / n);
    printf("\nAverage Turnaround Time   : %.2f\n", sum_tat / n);
    printf("\nCPU Utilization           : %.2f%%\n", (double)busy / total * 100);
}

int main(){
    Process p[MAX_P];
    int n, quantum;

    printf("Enter number of Processes (Max %d) : ", MAX_P);
    if(scanf("%d", &n) != 1 || n < 1 || n > MAX_P){
        printf("Invalid Number of Processes..!\n");
        return 1;
    }

    int total_burst = 0;
    for(int i = 0; i < n; i++){
        p[i].pid = i + 1;
        printf("\nP%d -------\n", p[i].pid);

        printf("Arrival Time  : ");
        if(scanf("%d", &p[i].arrival) != 1 || p[i].arrival < 0){
            printf("Invalid Arrival Time..!\n");
            return 1;
        }

        printf("Burst Time    : ");
        if(scanf("%d", &p[i].burst) != 1 || p[i].burst <= 0){
            printf("Invalid Burst Time..!\n");
            return 1;
        }
        
        total_burst += p[i].burst;
    }

    printf("\nEnter Time Quantum : ");
    if(scanf("%d", &quantum) != 1 || quantum <= 0){
        printf("Invalid Time Quantum..!\n");
        return 1;
    }

    Slice* gantt = (Slice *)malloc((total_burst + n +1) * sizeof(Slice));
    if(!gantt){
        printf("Out of Memory..!\n");
        return 1;
    } 

    // g = gantt_count i.e. number of blocks of processes exceuted
    int g = round_robin(p, n, quantum, gantt);
    g = merge_gantt(gantt, g);

    print_gantt(gantt, g);
    print_results(p, n, gantt, g);

    free(gantt);
    return 0;
}