#ifndef FINAL_PROJECT_37_JOB_QUEUE_H
#define FINAL_PROJECT_37_JOB_QUEUE_H

#include "defs.h"

enum Status {
    RUNNING, DONE, FAILURE, QUEUED
};
typedef struct job_struct {
    int id;
    int type;
    int server;
    int client;
    enum Status status;
    char *job_string;
    struct job_struct *next;
} job_, *job;

typedef struct j_q {
    job front;
    job rear;
} jq_, *jq;


void insert(jq q, int id, int type, int server, int client, enum Status status, char *job_string);

job dequeue(jq q);


#endif //FINAL_PROJECT_37_JOB_QUEUE_H
