#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scheduler.h"


char *
get_line(FILE * file, int line_number)
{
    size_t size = LINE_MAX + 1;
    int count = 0;
    char * line = NULL;
    while (getline(&line, &size, file) != -1)
    {
        if (count == line_number) {
            return line;
        }
        count++;
    }
    return line;
}

node_t *
init_node(long *node_data)
{
    node_t *node;
    node = (node_t *) malloc(sizeof(node_t));
    if (node == NULL)
        return NULL;

    node->pid     = node_data[PID_I];
    node->arrival = node_data[ARRIVAL_I];
    node->burst   = node_data[BURST_I];
    node->next    = (node_t *) malloc(sizeof(node_t)); 
    return node;
}

long
token_to_long(char *token)
{
    errno = 0;
    char *temp;
    long val;
    val = strtol(token, &temp, 0);
    if (temp == token || *temp != '\0' ||
    ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE))
    fprintf(stderr, 
            "Could not convert '%s' to long and leftover string is: '%s'\n",
            token, temp);
    return val;
}

node_t *
create_node_from_file(FILE *input, int line_number)
{
    node_t *node;
    char *token;
    char *separator;
    long *node_data;

    node_data = (long *) malloc(sizeof(long) * 3);

    separator = " ";

    token = strtok(get_line(input, line_number), separator);
    node_data[PID_I] = token_to_long(token); 

    token = strtok(NULL, separator);
    node_data[ARRIVAL_I] = token_to_long(token);

    token = strtok(NULL, "\n");
    node_data[BURST_I] = token_to_long(token);

    node = init_node(node_data);

    return node;
}

node_t *
insert_empty(node_t *tail, node_t *node)
{
    tail = node;
    tail->next = tail;
    return tail;
}

node_t *
insert_end(node_t *tail, node_t *node)
{
    node->next = tail->next;
    tail->next = node;
    tail = node;
    return tail;
}

node_t *
find_insert(node_t *tail, node_t *node)
{
    node_t *current;

    current = tail->next;
    while (current->next != tail->next && current->next->arrival < node->arrival)
        current = current->next;

    node->next = current->next;
    current->next = node;

    return tail;
}

node_t *
sorted_insert(node_t *tail, node_t *node)
{

    if (tail == NULL)
        return insert_empty(tail, node);

    else if (node->arrival >= tail->arrival)
        return insert_end(tail, node);

    else
        return find_insert(tail, node);
}

node_t *
remove_head(node_t *tail)
{
    node_t *old_head;

    old_head = tail->next;
    tail->next = tail->next->next;
    free(old_head);
    return NULL;
}

int
shortest_remaining_time_first(node_t *tail)
{
    int sys_time = 0;
    node_t *current;
    node_t *temp, *temp_tmp;
    do {
        current = tail->next;

        while (current->burst != 0)
        {
            temp = current->next;
            while (temp != current)
            {
                if ((temp->burst < current->burst && temp->arrival <= sys_time)
                    || (temp->burst == current->burst && temp->pid < current->pid 
                    && temp->arrival < sys_time))
                {
                    if (tail->arrival <= sys_time &&
                        tail->burst < current->burst)
                        tail = current;

                    else if (temp->burst == current->burst && 
                            temp->pid < current->pid && temp->arrival < sys_time)
                    {
                        temp_tmp = current;
                        while ( temp_tmp->next != temp)
                            temp_tmp = temp_tmp->next;
                        temp_tmp->next = temp->next;
                        temp->next = current;
                        current = temp;
                        tail->next = temp;
                    }
                    else
                    {
                        current->next = tail;
                        tail->next = temp;
                        temp_tmp = temp;
                        while (temp_tmp->next != tail)
                            temp_tmp = temp_tmp->next;
                        temp_tmp->next = current;
                    }
                    current = temp;
                    temp = current->next;
                }
                else
                    temp = temp->next;
            }
            current->burst--;
            printf("<system time   %d> ", sys_time); 
            printf("process %ld is running\n", current->pid);
            sys_time++;
        }
        printf("<system time   %d> ", sys_time); 
        printf("process %ld is finished.......\n", 
                current->pid);
        remove_head(tail);
    } while (current != tail->next);
    return sys_time;
}

int
round_robin(node_t *tail, int time_quantum, int num_process)
{
    int sys_time = 0;
    size_t i;
    node_t *temp;
    node_t *current;

    do {
        current = tail->next;
        if (current->burst <= time_quantum)
        {
            for (i = 0; i < current->burst; i++)
            {
                printf("<system time   %d> ", sys_time); 
                printf("process %ld is running\n", current->pid);
                sys_time++;
            }
            current->burst = 0;
            printf("<system time   %d> ", sys_time); 
            printf("process %ld is finished......\n", current->pid);
            remove_head(tail);
            num_process--;
        }
        else 
        {
            for (i = 0; i < time_quantum; i++)
            {
                printf("<system time   %d> ", sys_time); 
                printf("process %ld is running\n", current->pid);
                (current->burst)--;
                sys_time++;
            }
            while (current->next->arrival <= sys_time)
            {
                current = current->next;
                if (current == tail) {
                    tail = current->next;
                    break;
                }
            }
            if (tail == current->next)
                continue;

            temp = current->next;
            current->next = tail->next;
            tail->next = tail->next->next;
            current->next->next = temp;
        }
    } while (num_process != 0);
    return sys_time;
}

int
first_come_first_serve(node_t * tail)
{
    int sys_time = 0;
    node_t *current;
    current = tail->next;

    do {
        while (current->burst != 0)
        {
            (current->burst)--;
            printf("<system time   %d> ", sys_time); 
            printf("process %ld is running\n", 
                    current->pid, current->burst);
            sys_time++;
        }
        printf("<system time   %d> ", sys_time); 
        printf("process %ld is finished......\n", current->pid);
        current = current->next;
    }
    while (current != tail->next);
    return sys_time;
}

void
traverse(node_t *tail, int scheduler_type, int time_quantum, int num_process)
{
    int sys_time = 0;
    if (tail == NULL) {
        printf("Found no scheduled processes to execute. Exiting...\n");
        return;
    }
    if (scheduler_type == RR ) {
        sys_time = round_robin(tail, time_quantum, num_process);
    }
    else if (scheduler_type == SRTF) {
        sys_time = shortest_remaining_time_first(tail);
    }
    else if (scheduler_type == FCFS) {
        sys_time = first_come_first_serve(tail);
    }
    printf("<system time   %d> ", sys_time); 
    printf("All processes finished..........\n");
}

int
main(int argc, char **argv)
{
    setbuf (stdout, NULL);
    int process, num_processes;
    int ch;
    FILE *input;
    node_t *tail = NULL;
    int time_quantum;
    int scheduler_type;
    char *scheduler_arg = NULL;
    const char *fp;

    fp = argv[1];
    scheduler_arg = argv[2];

    if (!scheduler_arg) {
        printf("Missing argument: scheduler_type\n");
        return 1;
    }
    if (!strcmp(scheduler_arg, "FCFS")) {
        scheduler_type = 0;
    } else if (!strcmp(scheduler_arg, "RR")) {
        scheduler_type = 1;
        if (argc < 4) {
            printf("Missing argument: time quantum for round robin scheduler\n");
            return 1;
        }
        time_quantum = atoi(argv[3]);
    } else if (!strcmp(scheduler_arg, "SRTF")) {
        scheduler_type = 2;
    } else {
        printf("Invalid argument: scheduler_type must be from (FCFS, RR, SRTF)\n");
        return 1;
    }

    input = fopen(fp, "r"); 
    if (input == NULL) {
        printf("Error opening file \n"); 
        return 1;
    }

    num_processes = 0;
    while((ch = getc(input)) != EOF)
    {
        if (ch == '\n')
            num_processes++;
    }
    fclose(input);

    for (process = 0; process < num_processes; process++)
    {
        input = fopen(fp, "r");
        node_t *node;
        node = create_node_from_file(input, process);
        fclose(input);
        tail = sorted_insert(tail, node);
    }

    traverse(tail, scheduler_type, time_quantum, num_processes);

    if (scheduler_type == FCFS)
        free(tail);

    return 0;
}
