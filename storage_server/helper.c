#include "../defs.h"
#include "storage.h"

// Function to find root directory and initialize root node
// Return 0 on success, 1 when no root, 2 when multiple root
//

// Function to read all files from root directory
// And send them to NMS
// 0 on success
int sendFilesRecursively(char *rootPath, int sockfd, int jobid) {
    DIR *dir = opendir(rootPath);

    struct dirent *entry;
    struct stat statbuf;

    while ((entry = readdir(dir)) != NULL) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", rootPath, entry->d_name);
        if (stat(path, &statbuf) == -1) {
            perror("Error getting file status");
            continue;
        }

        if (!(S_ISDIR(statbuf.st_mode))) {
            // Print the file path
            char *data = (char *)malloc(sizeof(char) * 1040);
            snprintf(data, strlen(path) + 32, "REQ %d 0 %o %lld %s;", jobid, statbuf.st_mode, (long long int)statbuf.st_size, path);     // 0 for file, size, path
            int check = 1;
            // printf("%s\n", data);
            while (check){  // Tries to send thrice
                check *= sendData(data, sockfd);
                if (check >= 8) {
                    free(data);
                    closedir(dir);
                    return 3; // Error code for being unable to send
                }
            }
            free(data);
        }
        else {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // Recursively list files in the subdirectory
                char *data = (char *)malloc(sizeof(char) * 1040);
                snprintf(data, strlen(path) + 32, "REQ %d 1 %o %lld %s;", jobid, statbuf.st_mode, (long long int)statbuf.st_size, path);     // 1 for folder, size, path
                int check = 1;
                // printf("%s\n", data);
                while (check){  // Tries to send thrice
                    check *= sendData(data, sockfd);
                    if (check >= 8) {
                        free(data);
                        closedir(dir);
                        return 3; // Error code for being unable to send
                    }
                }
                free(data);
                sendFilesRecursively(path, sockfd, jobid);
            }
        }
    }
    closedir(dir);
    return 0;
}

// Sends error messages with code
// 0 on failure, 1 on success
int sendError(int sockfd, int code, char *message) {
    int len = strlen(message);
    char buffer[len+16];
    sprintf(buffer, "ERROR %d %s\n", code, message);
    if ((send(sockfd, buffer, BUFFER, 0)) == -1) {
        fprintf(stderr, "Issues sending message: %d\n", errno);
        return 0;
    };
    return 1;
}

// Sends messages with any data
// 0 on failure, 1 on success
int sendData(char *data, int sockfd) {
    printf("Sending %s\n", data);
    if ((send(sockfd, data, strlen(data), 0)) == -1) {
        fprintf(stderr, "Issues sending message: %d\n", errno);
        return 2;
    }
    printf("Sent\n");
    return 0;
}

// Create a file or folder and return the size
long long int createEntity(char *filePath, int type) {

    struct stat statbuf;
    if (type) {
        mkdir(filePath, 0777);
        DIR *dir = opendir(filePath);
        if (dir == NULL) {
            perror("Error creating Directory...\n");
            return -1;
        }
        closedir(dir);
    }
    else {
        FILE *file = fopen(filePath, "w");
        if (file == NULL) {
            perror("Error creating File...\n");
            return -1;
        }
        fclose(file);
    }

    stat(filePath, &statbuf);

    return (long long int) statbuf.st_size; // Returning size
}


// Delete directory or file
int deleteDirectory(const char *path) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(path))) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char fullpath[1024];

        // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Recursive call to delete subdirectories
            if (!deleteDirectory(fullpath)) {
                closedir(dir);
                return 0;
            }
            else return -1;
        } else {
            // Delete regular files
            if (remove(fullpath) != 0) {
                perror("remove");
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);

    // Remove the directory itself
    if (rmdir(path) != 0) {
        perror("rmdir");
        return -1;
    }

    return 0;
}