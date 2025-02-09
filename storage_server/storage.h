struct file {
    char path[1024];
    unsigned short size;
    int type;
};

int sendFilesRecursively(char *, int, int);
int sendData(char *, int);
int sendError(int, int, char *);