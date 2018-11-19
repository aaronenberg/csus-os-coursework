#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "buffer.h"

typedef struct
{
    buffer_item buffer[BUFFER_SIZE];
    int in, out, count;
    sem_t *full, *empty;
    pthread_mutex_t *mutex;
} sbuf_t;

sbuf_t *
init_sbuf()
{
    sbuf_t *buf;
    buf = (sbuf_t *) malloc(sizeof(sbuf_t));
    if (buf == NULL)
        return NULL;

    buf->in    = 0;
    buf->out   = 0;
    buf->count = 0;
    buf->empty = (sem_t *) malloc(sizeof(sem_t));
    sem_init(buf->empty, 0, BUFFER_SIZE);
    buf->full  = (sem_t *) malloc(sizeof(sem_t));
    sem_init(buf->full, 0, 0);
    buf->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(buf->mutex, NULL);
    
    return buf;
}

void
delete_sbuf(sbuf_t *buf)
{
    pthread_mutex_destroy(buf->mutex);
    free(buf->mutex);
    sem_destroy(buf->full);
    free(buf->full);
    sem_destroy(buf->empty);
    free(buf->empty);
    free(buf);
}

int
insert_item(buffer_item item, sbuf_t *sbuffer)
{
    int result = -1;

    sem_wait(sbuffer->empty);
    pthread_mutex_lock(sbuffer->mutex);
    
    if (sbuffer->count < BUFFER_SIZE) {
        sbuffer->buffer[sbuffer->in] = item;
        sbuffer->in++;
        sbuffer->in %= BUFFER_SIZE;
        sbuffer->count++;
        result = 0;
        printf("producer produced %d\n", item);
    }

    pthread_mutex_unlock(sbuffer->mutex);
    sem_post(sbuffer->full);

    return result;
}

void *
producer(void *buf)
{
    sbuf_t *sbuffer;
    buffer_item item;
    pthread_t tid;
    unsigned int now, seed;

    sbuffer = (sbuf_t *) buf;
    now = time(NULL);
    tid = pthread_self();
    seed = now^tid;

    while (1) {
        sleep(rand() % 5 + 1);
        item = rand_r(&seed);

        if (insert_item(item, sbuffer))
            fprintf(stderr, "FAILURE: attempt insertion with full buffer\n");
    }
    return NULL;
}

int
remove_item(buffer_item *item, sbuf_t *sbuffer)
{
    int result = -1;

    sem_wait(sbuffer->full);
    pthread_mutex_lock(sbuffer->mutex);

    if (sbuffer->count > 0) {
        *item = sbuffer->buffer[sbuffer->out];
        sbuffer->out++;
        sbuffer->out %= BUFFER_SIZE;
        sbuffer->count--;
        result = 0;
        printf("consumer consumed %d\n", *item);
    }

    pthread_mutex_unlock(sbuffer->mutex);
    sem_post(sbuffer->empty);

    return result;
}

void *
consumer(void *buf)
{
    sbuf_t *sbuffer;
    buffer_item item;

    sbuffer = (sbuf_t *) buf;

    while (1) {
        sleep(rand() % 5 + 1);

        if (remove_item(&item, sbuffer))
            fprintf(stderr, "FAILURE: attempt removal from empty buffer\n");
    }
    return NULL;
}

int
main(int argc, char **argv)
{
    setbuf (stdout, NULL);

    sbuf_t *sharedbuffer;
    int duration, numproducers, numconsumers, numthreads;
    int i, sflag, pflag, cflag;
    char opt;

    while ((opt = getopt(argc, argv, "s:p:c:")) != -1)
    {
        switch(opt)
        {
            case 's':
                sflag = 1;
                duration = atoi(optarg);
                break;
            case 'p':
                pflag = 1;
                numproducers = atoi(optarg);
                break;
            case 'c':
                cflag = 1;
                numconsumers = atoi(optarg);
                break;
            case '?':
                break;
            default:
                abort();
        }
    }
    opterr = 0;

    if (sflag != 1 || cflag != 1 || pflag != 1) {
        fprintf(stderr, "Missing required arguments\n");
        printf("Usage: %s -s <duration> -p <nproducers> -c <nconsumers>\n", argv[0]);
        exit(1);
    }

    numthreads = numproducers + numconsumers;
    pthread_t tid[numthreads];

    sharedbuffer = init_sbuf();

    for (i = 0; i < numconsumers; i++) {
        if (pthread_create(&tid[i], NULL, consumer, sharedbuffer))
            fprintf(stderr, "Could not create thread\n");
    }
    for (i = 0; i < numproducers; i++) {
        if (pthread_create(&tid[i], NULL, producer, sharedbuffer))
            fprintf(stderr, "Could not create thread\n");
    }

    sleep(duration);

    delete_sbuf(sharedbuffer);

    return 0;
}
