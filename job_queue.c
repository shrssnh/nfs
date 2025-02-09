//
// Created by Akshit  Kumar on 21/11/23.
//

#include "job_queue.h"


void insert(jq q, int id, int type, int server, int client, enum Status status, char *job_string) {
    job ptr = (job_ *) malloc(sizeof(job_));
    if (ptr == NULL) {
        perror("008: Memory exceeded\n");
        exit(1);
    } else {
        ptr->id = id;
        ptr->type = type;
        ptr->server = server;
        ptr->client = client;
        ptr->status = status;

        char *new_js = malloc(strlen(job_string + 1));
        strcpy(new_js, job_string);
        ptr->job_string = new_js;

        if ((q->front == NULL) && (q->rear == NULL)) {
            q->front = q->rear = ptr;
        } else {
            q->rear->next = ptr;
            q->rear = ptr;
        }
    }
    printf("Node inserted\n");
}

job dequeue(jq q) {
    if (q->front == NULL) {
        printf("Underflow.\n");
        return NULL;
    } else {
        job tmp = q->front;
        q->front = q->front->next;
        return tmp;
    }
}
