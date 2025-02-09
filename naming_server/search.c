#include "search.h"


rresult rsearch(char *searchstr, struct node *root, rresult final) {
    // printf("searching for: %s, root: %s\n", searchstr, root->path);
    if (root != NULL && strlen(root->path) > strlen(searchstr) && strstr(root->path, searchstr) != NULL) {
        rresult tmp = (rresult) malloc(sizeof(struct r_result_));
        if (tmp == NULL) {
            exit(EXIT_FAILURE);
        }

        tmp->found = root;
        tmp->next = NULL;

        if (final == NULL) {
            final = tmp;
        } else {
            rresult current = final;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = tmp;
        }
    }

    for (int i = 0; root != NULL && i < root->no_child; i++) {
        final = rsearch(searchstr, root->children[i], final);
    }

    return final;
}

struct node *absoluteSearch(char **searchStr, int elno, struct node *root) {
    if (strcmp(searchStr[0], root->name) == 0) {
        if (elno == 1) return root;
        struct node *test;
        for (int i = 0; i < root->no_child; i++) {
            test = absoluteSearch(&searchStr[1], elno - 1, root->children[i]);
            if (test != NULL) return test;
        }
    } else return NULL;
}

// void addNode(char *path, char *name, int type, long long int size, int perms) {
//     struct node *nd = (struct node *)malloc(sizeof(struct node));
//     nd->type = type;
//     nd->size = size;
//     nd->perms = perms;
//     nd->path = (char *)malloc(sizeof(char) * strlen(path) + strlen(name) + 2);
//     nd->name = (char *)malloc(sizeof(char) * strlen(name) + 2);
//     sprintf(nd->path, "%s/%s", path, name);
//     strcpy(nd->name, name);
//     nd->no_child = 0;

//     int elementss = 0;
//     for (int i = 0; i < strlen(path); i++) {
//         if (path[i] == '/') elementss += 1;
//     }
//     char **list = malloc();
//     struct node *parent = absoluteSearch();
// }
