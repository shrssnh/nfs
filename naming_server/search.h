#ifndef FINAL_PROJECT_37_SEARCH_H
#define FINAL_PROJECT_37_SEARCH_H

#include "./../helper.h"

typedef struct r_result_ {
    struct node *found;
    struct r_result_ *next;
} *rresult;


rresult rsearch(char *searchstr, struct node *root, rresult final);
struct node *absoluteSearch(char **searchStr, int elno, struct node *root);

#endif //FINAL_PROJECT_37_SEARCH_H
