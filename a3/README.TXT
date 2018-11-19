FILE DESCRIPTIONS:
    input.1 - file contains list of tasks to be scheduled and run by the program
    scheduler.h - header file contains definitions for scheduler.c program
    scheduler.c - program with implementations for the 3 scheduling algorithms

COMPILE COMMAND:
    gcc -o scheduler scheduler.c scheduler.h

RUN COMMAND:
    To run the program with First Come First Serve scheduling:
        ./scheduler input.1 FCFS

    To run the program with Round Robin scheduling (must specify time quantum):
        ./scheduler input.1 RR <time_quantum>
        - where <time_quantum> is a positive integer

    To run the program with Shortest Remaining Time First:
        ./scheduler input.1 SRTF
