#include "./../defs.h"
#include "storage.h"

int sockfd;
int running = 0;
int jobs[MAX_JOBS_STORAGE];
// pthread_t threads[];

char RECV_BUFFER[BUFFER];

pthread_mutex_t serverlock;


// Thred for heartbeat
void *heartbeat() {
    char beat[] = "heartbeat";
    while (running) {
        if ((send(sockfd, beat, strlen(beat), 0)) == -1) {
            fprintf(stderr, "Error sending Heartbeat\n", errno);
        }
        sleep(HEARTBEAT);
    }
}

// Function to initialize and send file tree
void sendTree(int jobpos) {
    char root[] = "./storage_server/root";

    struct stat statbuf;
    if (stat(root, &statbuf) == -1) {
        perror("Error getting file status");
    }

    char *data = (char *) malloc(sizeof(char) * 1040);
    snprintf(data, strlen(root) + 24, "REQ %d 1 %o 0 %s;", jobs[jobpos], statbuf.st_mode, root);
    int check = 1;
    while (check) {  // Tries to send thrice
        check *= sendData(data, sockfd);
        if (check >= 8) {
            free(data);
            // return 3; // Error code for being unable to send
        }
    }
    free(data);

    check = sendFilesRecursively(root, sockfd, jobs[jobpos]);
    if ((check) != 0) {
        char msg[] = "Error in initializing Storage Server!\n";
        if (check != 3) sendError(sockfd, check, msg);
        else fprintf(stderr, "Error in sending data to server...\n");
    }

    char stop[32];
    sprintf(stop, "REQ %d STOP;", jobs[jobpos]);
    sendData(stop, sockfd);
}


typedef struct client_args {
    int client_idx;
    int client_sock;
    char address[INET6_ADDRSTRLEN];
} *ctargs;

int
rmtree(const char path[]) {
    size_t path_len;
    char *full_path;
    DIR *dir;
    struct stat stat_path, stat_entry;
    struct dirent *entry;

    // stat for the path
    stat(path, &stat_path);

    // if path does not exists or is not dir - exit with status -1
    if (S_ISDIR(stat_path.st_mode) == 0) {
        fprintf(stderr, "%s: >%s<\n", "Is not directory", path);
        return -1;
    }

    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL) {
        fprintf(stderr, "%s: %s\n", "Can`t open directory", path);
        return -1;
    }

    // the length of the path
    path_len = strlen(path);

    // iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL) {

        // skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // determinate a full path of an entry
        full_path = calloc(path_len + 1 + strlen(entry->d_name) + 1, sizeof(char));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // stat for the entry
        stat(full_path, &stat_entry);

        // recursively remove a nested directory
        if (S_ISDIR(stat_entry.st_mode) != 0) {
            rmtree(full_path);
            free(full_path);
            continue;
        }

        // remove a file object
        if (unlink(full_path) == 0)
            printf("Removed a file:\t%s\n", full_path);
        else
            printf("Can't remove a file:\t%s\n", full_path);
        free(full_path);
    }

    // remove the devastated directory and close the object of it
    if (rmdir(path) == 0)
        printf("Removed a directory:\t%s\n", path);
    else
        printf("Can't remove a directory:\t%s\n", path);

    closedir(dir);
    return 1;
}

char *LRU[LRU_LENGTH] = {NULL};
char *LRU_key[LRU_LENGTH] = {NULL};
pthread_mutex_t lru_mutex;
int lru_index = 0;

void insert_LRU(char *file_path, char *contents) {
    pthread_mutex_lock(&lru_mutex);
    int circle = (lru_index) % LRU_LENGTH;
    LRU[circle] = strdup(contents);
    LRU_key[circle] = strdup(file_path);
    lru_index++;
    pthread_mutex_unlock(&lru_mutex);
}

int increment_LRU_index() {
    return (lru_index++) % LRU_LENGTH;
}

void client_handler(void *args) {
    int client_idx = ((struct client_args *) args)->client_idx;
    int client_sock = ((struct client_args *) args)->client_sock;

    char *client_address = ((struct client_args *) args)->address;
    printf("%s connected.\n", client_address);

    char buffer[CLIENT_BUFFER_LENGTH] = {0};

    for (;;) {
        recver(client_sock, buffer, CLIENT_BUFFER_LENGTH, 0);

        buffer[strcspn(buffer, "\r\n")] = 0;
        char *tmp = strdup(buffer);
        char *token = strtok(tmp, " ");

        if (strcmp(token, "CREATE") == 0) {
            char *file_path = strtok(NULL, " ");
            char *type_ = strtok(NULL, " ");

            if (strcmp(type_, "1") == 0) {
                mkdir(file_path, 0700);
                sender(client_sock, "DONE", 5);
            } else {
                FILE *fptr;

                fptr = fopen(file_path, "w");
                close(fptr);

                sender(client_sock, "DONE", 5);
            }

            int r = 1;
            pthread_exit(&r);
        }
        if (strcmp(token, "DELETE") == 0) {
            char *file_path = strtok(NULL, " ");
            char *type_ = strtok(NULL, " ");

            if (strcmp(type_, "1") == 0) {
                int status = rmtree(file_path);
                if (status == 1) {
                    sender(client_sock, "DONE", 4);
                } else {
                    sender(client_sock, "FAIL", 4);
                }
            } else {
                remove(file_path);
                sender(client_sock, "DONE", 4);
            }

            int r = 1;
            pthread_exit(&r);
        }
        if (strcmp(token, "WRITE") == 0) {
            char *file_path = strtok(NULL, " ");
            char *contents = strtok(NULL, " ");

            FILE *fptr;

            fptr = fopen(file_path, "w");
            fputs(contents, fptr);
            fclose(fptr);

            sender(client_sock, "DONE", 5);
            int r = 1;
            pthread_exit(&r);
        }
        if (strcmp(token, "READ") == 0) {
            char *file_path = strtok(NULL, " ");
            int lru_found = -1;
            for (int i = 0; i < lru_index && i < LRU_LENGTH; i++) {
                if ((strcmp(LRU_key[i], file_path) == 0)) {
                    lru_found = i;
                    break;
                }
            }

            if (lru_found > -1) {
                printf("Found in LRU!\n");
                sender(client_sock, LRU[lru_found], strlen(LRU[lru_found]) + 1);
                int r = 1;
                pthread_exit(&r);

            }
            FILE *f = fopen(file_path, "rb");
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

            char *string = malloc(fsize + 1);
            fread(string, fsize, 1, f);
            fclose(f);

            string[fsize] = 0;
            insert_LRU(file_path, string);

            sender(client_sock, string, fsize);
            int r = 1;
            pthread_exit(&r);

        }

    }
}

void *client_listener(void *sfd_client_pass) {
    int main_listener = *((int *) sfd_client_pass);

    struct sockaddr_storage client_addr;
    socklen_t addrlen;

    struct pollfd client_fds[1];
    client_fds[0].fd = main_listener;
    client_fds[0].events = POLLIN;

    int client_count = 0;
    pthread_t client_thread[MAX_CLIENT];


    for (;;) {
        // wait for new connections
        // if new connection
        // check if listener
        // if listener => create a new thread
        int poll_count = poll(client_fds, 1, -1);
        int client_new_fd = accept(main_listener, (struct sockaddr *) &client_addr, &addrlen);

        ctargs tmp = (ctargs) malloc(sizeof(struct client_args));
        tmp->client_idx = client_count;
        tmp->client_sock = client_new_fd;
        strcpy(tmp->address, getip(client_new_fd));

        pthread_create(&client_thread[client_count], NULL, client_handler, (void *) tmp);
        client_count++;

        if (tmp->client_idx == MAX_CLIENT) {
            break;
        }
    }

}

int main() {

    // Initiate connection with Naming Server
    sockfd = initconn(SERVER_ADDR, PORT_NSS);
    int sockfd_client = initserver(SERVER_ADDR, PORT_SSC);

    pthread_t client_listen;
    pthread_create(&client_listen, NULL, client_listener, &sockfd_client);
    if (sockfd != -1) running = 1;
    else {
        printf("Could not initialize server...\n");
        return 0;
    }

    // Initialize serverlock and heartbeat
    pthread_mutex_init(&serverlock, NULL);
    // pthread_t heart;
    // pthread_create(&heart, NULL, heartbeat, NULl);
    // pthread_join(heart, NULL);

    // Inititlize job array
    for (int i = 0; i < MAX_JOBS_STORAGE; i++) jobs[i] = -1;

    while (1) {

        // Recieve requests from Naming Server
        int r;
        if ((r = recv(sockfd, RECV_BUFFER, BUFFER,
                      0)) == -1) {
            fprintf(stderr, "Issues recv reqs: %d\n", errno);
            sendError(sockfd, 4, "Could not receive request, please resend...\n");
            continue;
        };

        // TEST
        printf("Command recieved: %s\n", RECV_BUFFER);

        // Get command string and assign job id for a free job space
        char *command = strtok(RECV_BUFFER, " ");
        int jobpos, curjob;
        for (int i = 0; i < MAX_JOBS_STORAGE; i++) {
            if (jobs[i] == -1) {
                char *jobid = strtok(NULL, " ");
                // jobid = strtok(NULL, ";");
                // printf("Job: %s\n", jobid);
                jobid[strlen(jobid) - 1] = '\0';
                sscanf(jobid, "%d", &jobs[i]);
                curjob = i;
                break;
            }
        }

        if (strcmp(command, "TREE") == 0) {
            sendTree(jobs[curjob]);
        }

        // if (strcmp(command, "REQ") == 0) {
        //     char *type = strtok(NULL, " ");
        //     if (strcmp(type, "CREATE") == 0) {
        //         //
        //     }
        //     else if (strcmp(type, "DELETE") == 0) {
        //         //
        //     }
        //     else if (strcmp(type, "READ") == 0) {
        //         //
        //     }
        //     else if (strcmp(type, "WRITE") == 0) {
        //         //
        //     }
        //     else {
        //         sendError(sockfd, 6, "Invalid request...\n");
        //         continue;
        //     }
        // }
        // else if (strcmp(command, "TREE") == 0) {
        //     sendTree(jobs[curjob]);
        // }
        // else if (strcmp(command, "TRANSFER") == 0) {
        //     //
        // }
        // else if (strcmp(command, "RECIEVE") == 0) {
        //     //
        // }
        // else {
        //     sendError(sockfd, 6, "Invalid request...\n");
        //     continue;
        // }
    }

    pthread_mutex_destroy(&serverlock);
    close(sockfd);
    return 1;
}
