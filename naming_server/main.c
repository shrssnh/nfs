#include "./../defs.h"
#include "naming.h"
#include "search.h"
#include "./../job_queue.h"

#define BACKLOG 10
// #define MAX_STORES 64

struct storage **storages = NULL;
int no_stores = 0;
int job_id = 1;

struct searchres {
    struct node **list;
    int len;
};

// Function to print tree
void printTree(struct node *nod, int tabs) {
    for (int i = 0; i < tabs; i++) printf("\t");
    printf("%s\n", nod->name);
    if (nod->type == 1) {
        for (int i = 0; i < nod->no_child; i++) {
            printTree(nod->children[i], tabs + 1);
        }
    }
}

// char **

char CLIENT_BUFFER[CLIENT_BUFFER_LENGTH];

void *storage_listener(void *sfd_storag) {
    int sfd_storage = *((int *) sfd_storag);
    for (;;) {
        struct sockaddr_storage store_addr;
        socklen_t addr_size = sizeof store_addr;
        int new_fd;
        if ((new_fd = accept(sfd_storage, (struct sockaddr *) &store_addr,
                             &addr_size)) == -1) {
            fprintf(stderr, "Issues accepting client reqs: %d\n", errno);
            exit(1);
        };

        pthread_t newstorage;
        pthread_create(&newstorage, NULL, storage_handler, &new_fd);
        pthread_join(newstorage, NULL);
    }
}

typedef struct client_args {
    int client_idx;
    int client_sock;
    char address[INET6_ADDRSTRLEN];
} *ctargs;

void sendOptions(char *CLIENT_BUFFER, int sock) {
    rresult tmp = (struct r_result_ *) malloc(sizeof(struct r_result_));

    char options[16384];
    options[0] = '\0';
    char temp[1024];

    int pos;
    for (int i = 0; i < strlen(CLIENT_BUFFER); i++) {
        if (CLIENT_BUFFER[i] == '/') pos = i+1;
    }

    for (int i = 0; i < no_stores; i++) {

        rsearch(CLIENT_BUFFER, storages[i]->root, tmp);

        while (tmp != NULL) {
            if (tmp->found != NULL && tmp->found->path != NULL && strcmp(tmp->found->name, &CLIENT_BUFFER[pos]) == 0) {
                printf("Path: %s\n", tmp->found->path);
                snprintf(temp, sizeof(temp), "%s %s\n", storages[i]->ip, tmp->found->path);
                strcat(options, temp);
            }
            tmp = tmp->next;
        }
    }
    if ((send(sock, options, strlen(options), 0)) == -1) {
        fprintf(stderr, "Issues sending message: %d\n", errno);
    }
}

void sendChildren(struct node *nd, int clientsock) {
    char temp[4096];
    temp[0] = '\0';
    for (int i = 0; i < nd->no_child; i++) {
        strcat(temp, nd->children[i]->name);
        strcat(temp, "\n");
    }
    if ((send(clientsock, temp, strlen(temp), 0)) == -1) {
        fprintf(stderr, "Issues sending message: %d\n", errno);
    }
}

void sendInfo(struct node *nd, int clientsock) {
    char temp[4096];
    if (nd->type) snprintf(temp, sizeof(temp), "Path: %s\tDIRECTORY\tPermissions: %o\tSize: %lld\n", nd->path, nd->perms, nd->size);
    else snprintf(temp, sizeof(temp), "Path: %s\tFILE\tPermissions: %o\tSize: %lld\n", nd->path, nd->perms, nd->size);
    if ((send(clientsock, temp, strlen(temp), 0)) == -1) {
        fprintf(stderr, "Issues sending message: %d\n", errno);
    }
}

void *client_handler(void *args) {
    int client_idx = ((struct client_args *) args)->client_idx;
    int client_sock = ((struct client_args *) args)->client_sock;
    char *client_address = ((struct client_args *) args)->address;

    printf("%s connected.\n", client_address);
    recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
    send(client_sock, "ACK", 4, 0);

    // job result = dequeue(tmp);
    // strcpy(CLIENT_BUFFER, "test");

    for (;;) {
        memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
        recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
        printf("(%s): %s\n", client_address, CLIENT_BUFFER);
        if (strcmp(CLIENT_BUFFER, "END") == 0) {
            printf("Client ended connection.\n");
            close(client_sock);
            int ret = 1;
            pthread_exit(&ret);
        }

        printf("%s\n", CLIENT_BUFFER);
        char newbuf[strlen(CLIENT_BUFFER) + 1];
        strcpy(newbuf, CLIENT_BUFFER);

        char *command = strtok(newbuf, " ");
        if (strcmp(command, "LIST") == 0) {
            // printf("client: %s\n", CLIENT_BUFFER);
            sendOptions(strtok(NULL, " "), client_sock);
            memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
            recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            printf("Recieved: %s\n", CLIENT_BUFFER);
            strcpy(newbuf, CLIENT_BUFFER);
            char *ip = strtok(newbuf, " ");
            char *path = strtok(NULL, " ");
            struct node *found = 0;
            for (int i = 0; i < no_stores; i++) {
                if (strcmp(storages[i]->ip, ip) == 0) {
                    printf("Found storage server\n");
                    char **list = NULL;
                    int el = 0;
                    char *tok = strtok(path, "/");
                    tok = strtok(NULL, "/");
                    tok = strtok(NULL, "/");
                    while (tok != NULL) {
                        el += 1;
                        list = (char **)realloc(list, sizeof(char *) * el);
                        list[el -1] = (char *)malloc(sizeof(char) * strlen(tok));
                        strcpy(list[el - 1], tok);
                        tok = strtok(NULL, "/");
                    }
                    printf("Searching\n");
                    for (int j = 0; j < el; j++) {
                        printf("%s\n", list[j]);
                    }
                    found = absoluteSearch(list, el, storages[i]->root);
                    printf("Found: %s\n", found->name);
                    break;
                }
            }
            sendChildren(found, client_sock);
        }

        else if (strcmp(command, "GETINFO") == 0) {
            // printf("client: %s\n", CLIENT_BUFFER);
            sendOptions(strtok(NULL, " "), client_sock);
            memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
            recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            printf("Recieved: %s\n", CLIENT_BUFFER);
            strcpy(newbuf, CLIENT_BUFFER);
            char *ip = strtok(newbuf, " ");
            char *path = strtok(NULL, " ");
            struct node *found = 0;
            for (int i = 0; i < no_stores; i++) {
                if (strcmp(storages[i]->ip, ip) == 0) {
                    printf("Found storage server\n");
                    char **list = NULL;
                    int el = 0;
                    char *tok = strtok(path, "/");
                    tok = strtok(NULL, "/");
                    tok = strtok(NULL, "/");
                    while (tok != NULL) {
                        el += 1;
                        list = (char **)realloc(list, sizeof(char *) * el);
                        list[el -1] = (char *)malloc(sizeof(char) * strlen(tok));
                        strcpy(list[el - 1], tok);
                        tok = strtok(NULL, "/");
                    }
                    printf("Searching\n");
                    for (int j = 0; j < el; j++) {
                        printf("%s\n", list[i]);
                    }
                    found = absoluteSearch(list, el, storages[i]->root);
                    printf("Found: %s\n", found->name);
                    break;
                }
            }
            sendInfo(found, client_sock);
        }

        else if (strcmp(command, "READ") == 0) {
            // printf("client: %s\n", CLIENT_BUFFER);
            sendOptions(strtok(NULL, " "), client_sock);
            memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
            recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            printf("Recieved: %s\n", CLIENT_BUFFER);
            // strcpy(newbuf, CLIENT_BUFFER);
            // char *ip = strtok(newbuf, " ");
            // char *path = strtok(NULL, " ");
            // struct node *found = 0;
            // memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
            // recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            // if (strcmp(CLIENT_BUFFER, "YES") == 0) {
            //     for (int i = 0; i < no_stores; i++) {
            //         if (strcmp(storages[i]->ip, ip) == 0) {
            //             printf("Found storage server\n");
            //             char **list = NULL;
            //             int el = 0;
            //             char *tok = strtok(path, "/");
            //             tok = strtok(NULL, "/");
            //             tok = strtok(NULL, "/");
            //             while (tok != NULL) {
            //                 el += 1;
            //                 list = (char **)realloc(list, sizeof(char *) * el);
            //                 list[el -1] = (char *)malloc(sizeof(char) * strlen(tok));
            //                 strcpy(list[el - 1], tok);
            //                 tok = strtok(NULL, "/");
            //             }
            //             printf("Searching\n");
            //             for (int j = 0; j < el; j++) {
            //                 printf("%s\n", list[i]);
            //             }
            //             found = absoluteSearch(list, el, storages[i]->root);
            //             char tosend[strlen(found->name) + 64];
            //             snprintf(tosend, sizeof(tosend), "%s %s\n", );
            //             printf("Found: %s\n", found->name);
            //             break;
            //         }
            //     }
            // }
            // sendInfo(found, client_sock);
        }

        else if (strcmp(command, "WRITE") == 0) {
            // printf("client: %s\n", CLIENT_BUFFER);
            sendOptions(strtok(NULL, " "), client_sock);
            memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
            recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            printf("Recieved: %s\n", CLIENT_BUFFER);
            // strcpy(newbuf, CLIENT_BUFFER);
            // char *ip = strtok(newbuf, " ");
            // char *path = strtok(NULL, " ");
            // struct node *found = 0;
            // memset(CLIENT_BUFFER, '\0', sizeof(CLIENT_BUFFER));
            // recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            // if (strcmp(CLIENT_BUFFER, "YES") == 0) {
            //     for (int i = 0; i < no_stores; i++) {
            //         if (strcmp(storages[i]->ip, ip) == 0) {
            //             printf("Found storage server\n");
            //             char **list = NULL;
            //             int el = 0;
            //             char *tok = strtok(path, "/");
            //             tok = strtok(NULL, "/");
            //             tok = strtok(NULL, "/");
            //             while (tok != NULL) {
            //                 el += 1;
            //                 list = (char **)realloc(list, sizeof(char *) * el);
            //                 list[el -1] = (char *)malloc(sizeof(char) * strlen(tok));
            //                 strcpy(list[el - 1], tok);
            //                 tok = strtok(NULL, "/");
            //             }
            //             printf("Searching\n");
            //             for (int j = 0; j < el; j++) {
            //                 printf("%s\n", list[i]);
            //             }
            //             found = absoluteSearch(list, el, storages[i]->root);
            //             char tosend[strlen(found->name) + 64];
            //             snprintf(tosend, sizeof(tosend), "%s %s\n", );
            //             printf("Found: %s\n", found->name);
            //             break;
            //         }
            //     }
            // }
            // sendInfo(found, client_sock);
        }

        else if (strcmp(command, "CREATE") == 0) {
            char *ip = strtok(CLIENT_BUFFER, " ");
            char *path = strtok(NULL, " ");
            char *type = strtok(NULL, " ");
            int pos = 0;
            for (int i = 0; i < strlen(path); i++) if (path[i] == '/') pos = i;
            path[pos] = '\0';
            char name[strlen(&path[pos+1]) + 1];
            strcpy(name, &path[pos + 1]);
            struct node *found = 0;
            for (int i = 0; i < no_stores; i++) {
                char **list = NULL;
                int el = 0;
                char *tok = strtok(path, "/");
                tok = strtok(NULL, "/");
                tok = strtok(NULL, "/");
                while (tok != NULL) {
                    el += 1;
                    list = (char **)realloc(list, sizeof(char *) * el);
                    list[el -1] = (char *)malloc(sizeof(char) * strlen(tok));
                    strcpy(list[el - 1], tok);
                    tok = strtok(NULL, "/");
                }
                printf("Searching\n");
                found = absoluteSearch(list, el, storages[i]->root);
                if (found != NULL) break;
            }
            sendInfo(found, client_sock);
            recver(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
            sendInfo(found, client_sock);
        }
        // else if (strcmp(command, "CREATE") == 0) {
        //     //
        // }
        // else if (strcmp(command, "DELETE") == 0) {
        //     //
        // }
        // else if (strcmp(command, "COPY") == 0) {
        //     //
        // }
        // else if (strcmp(command, "READ") == 0) {
        //     //
        // }
        // else if (strcmp(command, "WRITE") == 0) {
        //     //
        // }
        // else if (strcmp(command, "GETINFO") == 0) {

        // }

        // handle all other transactions here
        // TODO: transaction
        // == begin transation ===
        // == end transaction ===

        // strcpy(CLIENT_BUFFER, "test");
        send(client_sock, CLIENT_BUFFER, CLIENT_BUFFER_LENGTH, 0);
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


// Function to initialize tree for a storage server
// and keep receiving its messages
void *storage_handler(void *sock) {
    int sockfd = *((int *) sock);
    int r;
    char buf[1024] = {0};
    int receiving = 1;
    int locked = 1;
    struct node *parent = NULL;

    // Create storage server entry
    struct storage *entry = (struct storage *) malloc(sizeof(struct storage));
    entry->id = no_stores;
    entry->red1 = -1;
    entry->red2 = -1;
    entry->ip = getip(sockfd);
    no_stores += 1;
    storages = (struct storage **) realloc(storages, sizeof(struct storage *) * no_stores);
    storages[no_stores - 1] = entry;

    printf("Sending %s\n", "TREE 1;");
    if ((send(sockfd, "TREE 1;", strlen("TREE 1;"), 0)) == -1) {
        fprintf(stderr, "Issues sending message: %d\n", errno);
    }
    printf("Sent\n");

    // Loop that receives and initializes tree
    while (receiving) {

        // Buffer recieved
        char buf[1024] = {0};
        if ((r = recv(sockfd, buf, 1024, 0)) == -1) {
            fprintf(stderr, "Issues recv reqs: %d\n", errno);
            exit(1);
        }
            // if not over, get data and make node

            // if not over, get data and make node
        else if (r > 0) {

            // printf("Buffer: %s\n\n", buf);

            // delimiter if multiple nodes in buffer
            char delim[] = ";";
            char *tok = strtok(buf, delim);
            char namebuf[1024];

            // for every node in buffer
            while (tok != NULL) {

                printf("Token: %s\n\n", tok);

                // Check if over
                if (strcmp(tok, "REQ -1 STOP") == 0) {
                    printf("Stopping\n");
                    receiving = 0;
                    locked = 0;
                    break;
                }

                // make node
                struct node *nd = (struct node *) malloc(sizeof(struct node));
                int temp;
                sscanf(tok, "REQ %d %d %o %lld %s", &temp, &nd->type, &nd->perms, &nd->size,
                       namebuf);
                nd->path = strdup(namebuf);
                int pos = 0, parentpos = 0;
                for (int i = 0; i < strlen(namebuf); i++) {
                    if (namebuf[i] == '/') {
                        parentpos = pos;
                        pos = i;
                    }
                }
                char *name = (char *) malloc(
                        sizeof(char) * strlen(&namebuf[pos + 1]) + 2);
                strcpy(name, &namebuf[pos + 1]);
                nd->name = name;
                nd->no_child = 0;
                nd->parent = NULL;
                nd->children = NULL;

                // find parent and assign
                while (1) {
                    if (parent == NULL) {
                        parent = nd;
                        entry->root = nd;
                        // printf("First parent %s\n", parent->name);
                        break;
                    } else {
                        // printf("Comparing: %s, %s, %d\n", parent->name, &(namebuf[parentpos + 1]),
                        //        strlen(parent->name));
                        if (strncmp(parent->name, &(namebuf[parentpos + 1]), strlen(parent->name)) == 0 &&
                            parent->type == 1) {
                            nd->parent = parent;
                            parent->no_child += 1;
                            parent->children = (struct node **) realloc(
                                    parent->children,
                                    sizeof(struct node *) * parent->no_child);
                            parent->children[parent->no_child - 1] = nd;
                            break;
                        } else
                            parent = parent->parent;
                    }
                }

                if (nd->type == 1)
                    parent = nd;
                else
                    parent = nd->parent;

                // if (nd->parent != NULL)
                //     printf("%d %o %lld %s Parent: %s\n", nd->type, nd->perms,
                //            nd->size, nd->name, nd->parent->name);
                // else
                //     printf("%d %o %lld %s Parent: NULL\n", nd->type, nd->perms,
                //            nd->size, nd->name);
                tok = strtok(NULL, delim);
            }
        }
    }
    printf("Recieved.\n");
    struct node *nod = storages[entry->id]->root;
    printf("Root: %s\n", nod->name);
    printf("ID: %d\nIP: %s\n\n", entry->id, entry->ip);
    printTree(storages[entry->id]->root, 0);

}

// struct node **searchServer(char *searchstr, int id) {

// }

int main() {
    int sfd_client = initserver(SERVER_ADDR, PORT_NSC);
    int sfd_storage = initserver(SERVER_ADDR, PORT_NSS);
    // keep accepting requests

    pthread_t storage_listen, client_listen;
    pthread_create(&storage_listen, NULL, storage_listener, &sfd_storage);
    pthread_create(&client_listen, NULL, client_listener, &sfd_client);
    pthread_join(storage_listen, NULL);
    pthread_join(client_listen, NULL);

    close(sfd_client);
    close(sfd_storage);
}
