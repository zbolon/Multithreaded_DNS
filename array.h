#ifndef ARRAY_H
#define ARRAY_H
#include <pthread.h>

#define ARRAY_SIZE 8
#define MAX_NAME_LENGTH 17

typedef struct {
    char *array[ARRAY_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;

    pthread_mutex_t stdout_mutex, stderr_mutex;
    pthread_mutex_t req_log_mutex, res_log_mutex;
    pthread_mutex_t index_mutex;

    int producers_remaining;

} array;

int array_init(array *s, int num);
int array_put(array *s, char *hostname);
int array_get(array *s, char *hostname);
void array_free(array *s);

#endif
