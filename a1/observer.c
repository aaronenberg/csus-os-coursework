#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


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

long long
token_to_llong(char * token)
{
    errno = 0;
    char *temp;
    long long val = strtoll(token, &temp, 0);
    if (temp == token || *temp != '\0' ||
    ((val == LLONG_MIN || val == LLONG_MAX) && errno == ERANGE))
    fprintf(stderr, 
            "Could not convert '%s' to long and leftover string is: '%s'\n",
            token, temp);
    return val;
}

long
token_to_long(char * token)
{
    errno = 0;
    char *temp;
    long val = strtol(token, &temp, 0);
    if (temp == token || *temp != '\0' ||
    ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE))
    fprintf(stderr, 
            "Could not convert '%s' to long and leftover string is: '%s'\n",
            token, temp);
    return val;
}

long long *
parse_procstat_cpu(FILE *file, int num_tokens)
{
    int i = 0;
    int j = 0;
    char * token;
    long long *cpu_time = malloc(sizeof(long long) * num_tokens);
    long hz = sysconf(_SC_CLK_TCK);
    char * search = " ";
    token = strtok(get_line(file, 0), search);

    while (token != NULL && j < num_tokens)
    {
        i++;
        token = strtok(NULL, search);
        if ( i == 1 || i == 3 || i == 4 ) {
            cpu_time[j] = token_to_llong(token) / hz;
            j++;
        }
    }
    return cpu_time;
}

void
print_diskstats(FILE *file)
{
    size_t size = LINE_MAX + 1;
    char * line = malloc(LINE_MAX + 1);
    while (getline(&line, &size, file) != -1) {
        printf("%s", line);
        printf("\t\t\t");
    }
    free(line);
    return;
}

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

char *
get_time_of_boot(FILE *file)
{
    char * token;
    long time_of_boot;
    char * search = " ";
    token = strtok(get_line(file, 0), search);
    token = strtok(NULL, search);
    time_of_boot = token_to_long(trimwhitespace(token));
    time_t boot_time = (time_t) time_of_boot;
    token = ctime(&time_of_boot);
    return token;
}

char *
get_total_processes_since_boot(FILE *file)
{
    char * token;
    char * search = " ";
    token = strtok(get_line(file, 0), search);
    token = strtok(NULL, search);
    return token;
}

char *
get_meminfo(FILE *file, int line_number)
{
    char * meminfo = malloc(sizeof(char*));
    meminfo = get_line(file, line_number);
    return meminfo;
}

char *
get_loadavg(FILE *file)
{
    char *loadavg;
    char *search = " ";
    loadavg = strtok(get_line(file, 0), search);
    return loadavg; 
}

void
print_info(const char *filename, FILE * file)
{
    int num_tokens;
    int num_cores = 4;
    int i;
    char * line = malloc(LINE_MAX + 1);

    if (strcmp(filename, "/proc/sys/kernel/hostname") == 0) {
        printf("Machine hostname:\t%s", get_line(file, 0));
    }
    if (strcmp(filename, "/proc/version") == 0) {
        printf("Kernel version:\t\t%s", get_line(file, 0));

    }
    if (strcmp(filename, "/proc/stat") == 0) {
        printf("CPU time: \t\tUSER\t\tSYSTEM\tIDLE\n\t\t\t" );
        num_tokens = 3;
        long long *cpu_time = parse_procstat_cpu(file, num_tokens);
        for (i = 0; i < num_tokens; i++) {
            printf("%ld\t", cpu_time[i]);
        }
        printf("seconds\n\n");
        line = get_line(file, num_cores + 1);
        printf("# of context switches:\t%s", line);
        printf("Time of last boot:\t%s", get_time_of_boot(file));
        printf("# of processes created: %s\n",
                get_total_processes_since_boot(file));
    }
    if (strcmp(filename, "/proc/diskstats") == 0) {
        printf("# of disk requests\t");
        print_diskstats(file);
        printf("\n");
    }
    if (strcmp(filename, "/proc/meminfo") == 0) {
        printf("%s", get_meminfo(file, 0));
        printf("%s", get_meminfo(file, 0));
    }
    if (strcmp(filename, "/proc/loadavg") == 0) {
        printf("\t\t\t%s\n", get_loadavg(file));
    }
    free(line);
    return; 
}

void
print_file(const char *filename)
{
    FILE * file;
    file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Cannot open file \n");
        exit(0);
    }
    print_info(filename, file);
    fclose(file);
    return;
}

struct Time
{
    const long second;
    const long minute;
    const long hour;
    const long day;
};

struct Time
calc_time_from_seconds(long n)
{
    const long days = n / (24 * 3600);

    n = n % (24 * 3600);
    const long hours = n / 3600;
    
    n %= 3600;
    const long minutes = n / 60;

    n %= 60;
    const long seconds = n;

    struct Time time =
    {
        seconds,
        minutes,
        hours,
        days
    };

    return time;
}

void standard_report(const char **FILE_LIST)
{
    print_file(FILE_LIST[0]);
    struct sysinfo si;
    sysinfo(&si);
    struct Time uptime = calc_time_from_seconds(si.uptime);
    printf("Uptime:");
    printf("\t\t\t%02ld:%02ld:%02ld:%02ld (dd:hh:mm:ss)\n", 
            uptime.day, uptime.hour, uptime.minute, uptime.second);

    print_file(FILE_LIST[1]);
    printf("CPU info\n");
    printf("--------\n");
    system("lscpu | grep -e 'Architecture' -e 'Model name'");
    printf("\n");

    return;
}

void
short_report(const char **FILE_LIST)
{
    print_file(FILE_LIST[2]);
    print_file(FILE_LIST[3]);
    return;
}

void
long_report(const char **FILE_LIST, int interval, int duration)
{
    short_report(FILE_LIST);
    print_file(FILE_LIST[4]);
    printf("Load average (1 minute):\n");
    struct timeval now;
    int iteration = 0;
    while (iteration < duration) {
        sleep(interval);
        print_file(FILE_LIST[5]);
        iteration += interval;
    }
    return;
}

int
main(int argc, char **argv)
{
    // all writes to stdout unbuffered
    setbuf (stdout, NULL);

    const int NUM_FILES = 5;
    const char *FILE_LIST[NUM_FILES];
    FILE_LIST[0] = "/proc/sys/kernel/hostname";
    FILE_LIST[1] = "/proc/version";
    FILE_LIST[2] = "/proc/stat";
    FILE_LIST[3] = "/proc/diskstats";
    FILE_LIST[4] = "/proc/meminfo";
    FILE_LIST[5] = "/proc/loadavg";
    char report_type[16];
    int interval;
    int duration;
    char c;

    strcpy(report_type, "Standard");

    while ((c = getopt (argc, argv, "sl:")) != -1)
    switch (c)
    {
        case 's':
        strcpy(report_type, "Short");
        break;
        case 'l':
        strcpy(report_type, "Long");
        interval = atoi(optarg);
        duration = atoi(argv[3]);
        break;
        case '?':
        if (optopt == 'l') {
            fprintf (stderr, 
                    "Option -%c requires two arguments: \
                    interval (integer), duration (integer).\n",
                    optopt);
        }
        else if (isprint (optopt)) {
            fprintf (stderr, 
                     "Unknown option `-%c'.\n", 
                     optopt);
        }
        else {
            fprintf (stderr,
                     "Unknown option character `\\x%x'.\n",
                     optopt);
        }
        return 1;
        default:
        abort ();
    }
    opterr = 0;

    struct timeval * tv = malloc(sizeof (struct timeval));
    gettimeofday(tv, NULL);
    printf("Date:\t\t\tStatus report type %s at %s", 
            report_type, ctime(&(tv->tv_sec)));
    free(tv);

    standard_report(FILE_LIST);
    
    if ( strcmp(report_type, "Short") == 0 ) {
        short_report(FILE_LIST);
    }
    if ( strcmp(report_type, "Long") == 0 ) {
        long_report(FILE_LIST, interval, duration);
    }

    return 0;
}
