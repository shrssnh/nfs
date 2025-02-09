#ifndef _DEFS_H_
#define _DEFS_H_

#include "helper.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "job_queue.h"

#define PORT_NSS "8011" // ns <-> ss
#define PORT_NSC "8021" // client <-> ns
#define PORT_SSC "8031" // client <-> ss


#define LRU_LENGTH 10
#define SERVER_ADDR "192.168.1.159"
#define BACKLOG 10
#define BUFFER 1024
#define MAX_FILES_PER_DIRECTORY 256

#define MAX_JOBS_STORAGE 32
#define MAX_CLIENT 64 // max number of clients and servers

#define HEARTBEAT 1

// Naming Serever States

#define JOB_UNALLOCATED 0
#define JOB_ALLOCATED 1
#define JOB_PROCESSING 2
#define JOB_COMPLETED 3
#define JOB_FAILED 4

// char ROOT_ADDRESS[] = "./storage_server/root"
// client stuff
#define CLIENT_BUFFER_LENGTH 1024

#endif
