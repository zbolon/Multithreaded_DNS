#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include "multi-lookup.h"
#include "array.h"


void* producer(void* arg){
    producer_args *pargs = (producer_args *) arg;
    char **argv = pargs->argv;
    pthread_mutex_t *req_log_mutex = &pargs->arr->req_log_mutex;
    pthread_mutex_t * index_mutex = &pargs->arr->index_mutex;

    gettimeofday(&pargs->t, NULL);

    time_t start_s = pargs->t.tv_sec;
    suseconds_t start_m = pargs->t.tv_usec;

    


    while (1){
        // need a lock for files remaining

        pthread_mutex_lock(index_mutex);
        if (*pargs->index == pargs->argc){
            pthread_mutex_unlock(index_mutex);
            break;
        }
        FILE *fp = fopen(argv[*pargs->index], "r");
        *pargs->index += 1;
        pthread_mutex_unlock(index_mutex);

        char buffer[MAX_INPUT_LENGTH];

        if (fp == NULL){
            pthread_mutex_lock(&pargs->arr->stderr_mutex);
            fprintf(stderr, "Invalid file %s\n", argv[*pargs->index - 1]);
            pthread_mutex_unlock(&pargs->arr->stderr_mutex);
            continue;
        }

        
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {

            // need to call array_put
            array_put(pargs->arr, buffer);
            // write to log file
            pthread_mutex_lock(req_log_mutex);
            fputs(buffer, pargs->logfile);
            pthread_mutex_unlock(req_log_mutex);
            
        } 
        if (ferror(fp)){
            pthread_mutex_lock(&pargs->arr->stderr_mutex);
            fprintf(stderr, "Error reading from file %s\n", argv[*pargs->index - 1]);
            pthread_mutex_unlock(&pargs->arr->stderr_mutex);
        }


        pargs->files_read += 1;
        fclose(fp);
    }


    pthread_mutex_lock(&pargs->arr->mutex);
    pargs->arr->producers_remaining -= 1;
    if (pargs->arr->producers_remaining == 0) {
        pthread_cond_broadcast(&pargs->arr->not_empty);
    }
    pthread_mutex_unlock(&pargs->arr->mutex);

    

    gettimeofday(&pargs->t, NULL);
    pthread_mutex_lock(&pargs->arr->stdout_mutex);
    fprintf(stdout, "Thread <%ld> serviced %d files in %f seconds\n", pthread_self(), pargs->files_read, ((pargs->t.tv_sec - start_s) + ((pargs->t.tv_usec - start_m) / 1e6)));
    pthread_mutex_unlock(&pargs->arr->stdout_mutex);

    free(pargs);
    
    return NULL;

    
}

void* consumer(void* arg){
    consumer_args *cargs = (consumer_args *) arg;

    gettimeofday(&cargs->t, NULL);
    time_t start_s = cargs->t.tv_sec;
    suseconds_t start_m = cargs->t.tv_usec;


    while (1){
        // get from array
        char hostname[MAX_NAME_LENGTH];
        if (array_get(cargs->arr, hostname)){

            char buffer[MAX_IP_LENGTH];
            char *found = strchr(hostname, '\n');
            if (found != NULL){
                hostname[found - hostname] = '\0';
            }

            if (dnslookup(hostname, buffer, MAX_IP_LENGTH) == UTIL_SUCCESS){
                pthread_mutex_lock(&cargs->arr->res_log_mutex);
                fputs(hostname, cargs->logfile);
                fputs(", ", cargs->logfile);
                fputs(buffer, cargs->logfile);
                fputs("\n", cargs->logfile);
                pthread_mutex_unlock(&cargs->arr->res_log_mutex);
            } else {

                pthread_mutex_lock(&cargs->arr->res_log_mutex);
                fputs(hostname, cargs->logfile);
                fputs(", NOT_RESOLVED\n", cargs->logfile);
                pthread_mutex_unlock(&cargs->arr->res_log_mutex);
            }
            cargs->hosts_resolved += 1;
        } else {
            break;
        }
    }
    


    gettimeofday(&cargs->t, NULL);
    pthread_mutex_lock(&cargs->arr->stdout_mutex);
    fprintf(stdout, "Thread <%ld> resolved %d hosts in %f seconds\n", pthread_self(),cargs->hosts_resolved, ((cargs->t.tv_sec - start_s) + ((cargs->t.tv_usec - start_m) / 1e6)));
    pthread_mutex_unlock(&cargs->arr->stdout_mutex);
    free(cargs);

    return NULL;
}

int main(int argc, char *argv[]){
    if (argc < 6 || argc - (argc - 5) < 5){
        fprintf(stderr, "Not enough arguments\n");
        fprintf(stderr, "multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
        return -1;
    } else if (argc - 5 > MAX_INPUT_FILES){
        fprintf(stderr, "Too many input files\n");
        return -1;
    }

    if (!isNumber(argv[1])){
        fprintf(stderr, "Please enter a valid number for <# requester>\n");
        return -1;
    } else if (!isNumber(argv[2])){
        fprintf(stderr, "Please enter a valid number for <# resolver>\n");
        return -1;
    }

    if (atoi(argv[1]) > MAX_REQUESTER_THREADS){
        fprintf(stderr, "Too many requester threads (must be <= 10)\n");
        return -1;
    } else if (atoi(argv[1]) < 0){
        fprintf(stderr, "Need at least 1 requester thread\n");
        return -1;
    }
    if (atoi(argv[2]) > MAX_RESOLVER_THREADS){
        fprintf(stderr, "Too many resolver threads (must be <= 10)\n");
        return -1;
    } else if (atoi(argv[2]) < 0){
        fprintf(stderr, "Need at least 1 resolver thread\n");
        return -1;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    time_t start_s = t.tv_sec;
    suseconds_t start_m = t.tv_usec;

    array s;

    if(array_init(&s, atoi(argv[1]))){
        fprintf(stderr, "Could not create a shared array\n");
        return -1;
    }


    FILE *requester_log = fopen(argv[3], "w");
    if (requester_log == NULL){
        fprintf(stderr, "Failed to open log %s\n", argv[3]);
    }
    FILE *resolver_log = fopen(argv[4], "w");
    if (resolver_log == NULL){
        fprintf(stderr, "Failed to open %s\n", argv[4]);
    }

    pthread_t req[atoi(argv[1])];
    pthread_t res[atoi(argv[2])];
    int index = 5;

    for (int i = 0; i < atoi(argv[1]); i++){
        producer_args *pargs = malloc(sizeof(producer_args));
        

        pargs->arr = &s;
        pargs->index = &index;
        pargs->argc = argc;
        pargs->argv = argv;
        pargs->files_read = 0;
        pargs->logfile = requester_log;

        pthread_create(&req[i], NULL, producer, pargs);
    }

    for (int i = 0; i < atoi(argv[2]); i++){
        consumer_args *cargs = malloc(sizeof(consumer_args));
        cargs->arr = &s;
        cargs->hosts_resolved = 0;
        cargs->logfile = resolver_log;

        pthread_create(&res[i], NULL, consumer, cargs);
    }


    
    for (int i = 0; i < atoi(argv[1]); i++){
        pthread_join(req[i], NULL);
    }

    for (int i = 0; i < atoi(argv[2]); i++){
        pthread_join(res[i], NULL);
    }

    fclose(requester_log);
    fclose(resolver_log);

    array_free(&s);
    gettimeofday(&t, NULL);
    printf("%s: total time: is %f seconds\n", argv[0],((t.tv_sec - start_s) + ((t.tv_usec - start_m) / 1e6)));


    return 0;
}

int isNumber(char *buffer){
    for (int i = 0; buffer[i] != '\0'; i++){
        if (!isdigit(buffer[i])){
            return 0;
        }
    }
    return 1;
}
