#ifndef MULTI_H
#define MULTI_H

#include <pthread.h>
#include "array.h"
#include <sys/time.h>
#include <stdio.h>
#include "util.h"

#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_INPUT_LENGTH 32
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

typedef struct{
    array* arr;
    char **argv;
    int *index, argc;
    int files_read;
    FILE *logfile;
    struct timeval t;
    
} producer_args;

typedef struct{
    array* arr;
    int hosts_resolved;
    FILE *logfile;
    struct timeval t;
} consumer_args;




void* producer(void* arg);
void* consumer(void* arg);
int isNumber(char *buffer);

#endif
