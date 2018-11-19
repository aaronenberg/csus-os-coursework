#define PID_I 0
#define ARRIVAL_I 1
#define BURST_I 2

#define FCFS 0
#define RR 1
#define SRTF 2

typedef struct node_t
{
    long pid;
    long arrival;
    long burst;
    struct node_t *next;
} node_t;
