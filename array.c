#include "array.h"
#include <stdlib.h>
#include <string.h>

int array_init(array *s, int num) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        char *memory = malloc(MAX_NAME_LENGTH + 1);
        if (!memory){
            return -1;
        }
        s->array[i] = memory;
    }
    s->head = 0;
    s->tail = 0;
    s->count = 0;
    s->producers_remaining = num;

    if (pthread_mutex_init(&s->mutex, NULL) != 0){
        return -1;
    }
    if (pthread_mutex_init(&s->index_mutex, NULL) != 0){
        pthread_mutex_destroy(&s->mutex);
        return -1;
    }
    if (pthread_mutex_init(&s->stdout_mutex, NULL) != 0){
        pthread_mutex_destroy(&s->mutex);
        pthread_mutex_destroy(&s->index_mutex);
        return -1;
    }
    if (pthread_mutex_init(&s->stderr_mutex, NULL) != 0){
        pthread_mutex_destroy(&s->mutex);
        pthread_mutex_destroy(&s->index_mutex);
        pthread_mutex_destroy(&s->stdout_mutex);
        return -1;
    }
    if (pthread_mutex_init(&s->req_log_mutex, NULL) != 0){
        pthread_mutex_destroy(&s->mutex);
        pthread_mutex_destroy(&s->index_mutex);
        pthread_mutex_destroy(&s->stdout_mutex);
        pthread_mutex_destroy(&s->stderr_mutex);
        return -1;
    }
    if (pthread_mutex_init(&s->res_log_mutex, NULL) != 0){
        pthread_mutex_destroy(&s->mutex);
        pthread_mutex_destroy(&s->index_mutex);
        pthread_mutex_destroy(&s->stdout_mutex);
        pthread_mutex_destroy(&s->stderr_mutex);
        pthread_mutex_destroy(&s->req_log_mutex);
        return -1;
    }



    if (pthread_cond_init(&s->not_full, NULL) != 0) {
        pthread_mutex_destroy(&s->mutex);
        pthread_mutex_destroy(&s->index_mutex);
        pthread_mutex_destroy(&s->stdout_mutex);
        pthread_mutex_destroy(&s->stderr_mutex);
        pthread_mutex_destroy(&s->req_log_mutex);
        pthread_mutex_destroy(&s->res_log_mutex);
        return -1;
    }
    if (pthread_cond_init(&s->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&s->mutex);
        pthread_mutex_destroy(&s->index_mutex);
        pthread_mutex_destroy(&s->stdout_mutex);
        pthread_mutex_destroy(&s->stderr_mutex);
        pthread_mutex_destroy(&s->req_log_mutex);
        pthread_mutex_destroy(&s->res_log_mutex);
        return -1;
    }
    return 0;
}

int array_put(array *s, char *hostname){
    pthread_mutex_lock(&s->mutex);

    while (s->count == ARRAY_SIZE){
        pthread_cond_wait(&s->not_full, &s->mutex);
    }

    s->tail = (s->head + s->count) % ARRAY_SIZE;
    
    memcpy(s->array[s->tail], hostname, strlen(hostname) + 1);
    s->count += 1;

    pthread_cond_signal(&s->not_empty);
    pthread_mutex_unlock(&s->mutex);

    return 0;
}


int array_get(array *s, char *hostname){
    pthread_mutex_lock(&s->mutex);
    while (s->count == 0 && s->producers_remaining > 0){
        pthread_cond_wait(&s->not_empty, &s->mutex);
    }
    if (s->count == 0 && s->producers_remaining == 0){
        pthread_mutex_unlock(&s->mutex);
        return 0;
    }

    memcpy(hostname, s->array[s->head], strlen(s->array[s->head]) + 1);
    s->head = (s->head + 1) % ARRAY_SIZE;
    s->count = s->count - 1;

    pthread_cond_signal(&s->not_full);
    pthread_mutex_unlock(&s->mutex);
    

    return 1;
}


void array_free(array *s) {
    pthread_mutex_lock(&s->mutex);
    for (int i = 0; i < ARRAY_SIZE; i++){
        free(s->array[i]);
    }

    pthread_mutex_unlock(&s->mutex);

    pthread_mutex_destroy(&s->mutex);
    pthread_mutex_destroy(&s->index_mutex);
    pthread_mutex_destroy(&s->stdout_mutex);
    pthread_mutex_destroy(&s->stderr_mutex);
    pthread_mutex_destroy(&s->res_log_mutex);
    pthread_mutex_destroy(&s->req_log_mutex);
}
