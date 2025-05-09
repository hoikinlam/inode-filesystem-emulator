#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// inode struct
typedef struct{
    uint32_t inode; // inode identifier
    uint32_t parentInode; // parent inode 
    char type; // directory (d) or file (f)
    char name[32]; // name of file
} Inode;

Inode inodeList[1024]; // array of inodes
size_t inodeCount = 0; // number of inodes
uint32_t currentInode = 0; // current inode

// function declarations
void loadInodeList(const char *path);
void saveInodeList(const char *path);
void changeDirectory(const char *name);
void listContents();
void createDirectory(const char *name);
void createFile(const char *name);

// loading inodes
void loadInodeList(const char *path) {
    // reading binary file
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Error opening file: %s\n", path);
        exit(1);
    }

    // reading inodes
    inodeCount = fread(inodeList, sizeof(Inode), 1024, fp);
    fclose(fp);

    // validating inodes
    size_t validCount = 0;
    for (size_t i = 0; i < inodeCount; i++) {
        if (inodeList[i].inode >= 1024) {
            printf("Error: Invalid inode number");
        } else if (inodeList[i].type != 'd' && inodeList[i].type != 'f') {
            printf("Error: Invalid inode type");
        } else {
            // Ensure the name is null terminated.
            inodeList[i].name[31] = '\0';
            inodeList[validCount++] = inodeList[i];
        }
    }
    
    inodeCount = validCount;

    /* this is wrong
    // Check that inode 0 is directory or exists
    if (inodeCount == 0) {
        printf("Error: No inodes loaded\n");
        exit(1);
    }
    if (inodeList[0].type != 'd') {
        printf("Error: inode 0 is not a directory\n");
        exit(1);
    } */
}

// saving inodes
void saveInodeList(const char *path) {
    FILE *fp = fopen(path, "wb"); //open binary file in write mode
    if(fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    // write inode content to list
    fwrite(inodeList, sizeof(Inode), inodeCount, fp);
    fclose(fp);
}

// changing directory
void changeDirectory(const char *name) {
    // implement .. and ./.. to go back to previous directory
    if (strcmp(name, "..") == 0 || strcmp(name, "./..") == 0) {
        if (currentInode == 0) {
            printf("Already at root directory\n");
        } else {
            currentInode = inodeList[currentInode].parentInode;
        }
        return;
    }

    // iterating inode list 
    for (size_t i = 0; i < inodeCount; i++) {
        // check if inode name exists
        if (inodeList[i].parentInode == currentInode && strcmp(inodeList[i].name, name) == 0) {
            if (inodeList[i].type != 'd') {
                printf("Error: Not a directory\n");
                return;
            }
            // updating inode
            currentInode = inodeList[i].inode;
            return;
        }
    }
    
    printf("Error: directory not found\n");
}

// list contents
void listContents() {
    // iterating inode list
    for (size_t i = 0; i < inodeCount; i++){
        if (inodeList[i].parentInode == currentInode){
            // printing inode data
            printf("inode: %u, type: %c, name: %s\n", inodeList[i].inode, inodeList[i].type, inodeList[i].name);
        }
    }

    return;
}

// creating new directories
void createDirectory(const char *name) {
    // truncating name
    char truncatedName[33];
    strncpy(truncatedName, name, 32);
    truncatedName[32] = '\0';

    // iterating inode list
    for (size_t i = 0; i < inodeCount; i++) {
        // check if name exists
        if (inodeList[i].parentInode == currentInode && strcmp(inodeList[i].name, truncatedName) == 0) {
            printf("name already exists\n");
            return;
        }
    }

    // check if inode limit reached
    if (inodeCount >= 1024) {
        printf("inode maxed\n");
        exit(1);
    }

    // creating new inode of directory type
    inodeList[inodeCount].inode = inodeCount;
    inodeList[inodeCount].parentInode = currentInode;
    inodeList[inodeCount].type = 'd';
    strncpy(inodeList[inodeCount].name, truncatedName, 32);
    inodeList[inodeCount].name[31] = '\0';
    inodeCount++;

    // create a file with its name as the inode number
    char newfile[32];
    snprintf(newfile, 32, "%zu", inodeCount - 1);
    FILE *fp = fopen(newfile, "w");
    if (fp == NULL) {
        printf("Error opening file\n");
        return;
    }

    // write into this file its . and .. inode values
    fprintf(fp, "%zu\n %u\n", inodeCount - 1, currentInode);
    fclose(fp);
}

void createFile(const char *name) {
    // truncated name
    char truncatedName[33];
    strncpy(truncatedName, name, 32);
    truncatedName[32] = '\0';

    // iterate list
    for (size_t i = 0; i < inodeCount; i++) {
        // check if name exists
        if (inodeList[i].parentInode == currentInode && strcmp(inodeList[i].name, truncatedName) == 0) {
            printf("name already exists\n");
            return;
        }
    }

    // check inode limit
    if (inodeCount >= 1024) {
        printf("inode maxed\n");
        exit(1);
    }

    // create new inode of type f
    inodeList[inodeCount].inode = inodeCount;
    inodeList[inodeCount].parentInode = currentInode;
    inodeList[inodeCount].type = 'f';
    strncpy(inodeList[inodeCount].name, truncatedName, 32);
    inodeList[inodeCount].name[31] = '\0';
    inodeCount++;

    // create a file with its name as the inode number
    char newfile[32];
    snprintf(newfile, 32, "%zu", inodeCount - 1);
    FILE *fp = fopen(newfile, "w");
    if (fp == NULL) {
        printf("Error opening file\n");
        return;
    }

    // write into this file its filename
    fprintf(fp, "%s\n", truncatedName);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    // check correct number of arguments
    if (argc != 2) {
        printf("Error: Invalid Number of Arguments\n");
        return 1;
    }

    // change to file directory
    if (chdir(argv[1]) != 0) {
        perror("Error: Fail to change directory");
        return 1;
    }

    // load inode list from file
    loadInodeList("inodes_list");

    // dynamic buffers for strings
    char *input = NULL;
    size_t inputSize = 0;
    char *cmd = NULL;
    char *args = NULL; 

    while (1) {
        printf("> ");

        // allocate input
        ssize_t bytesRead = getline(&input, &inputSize, stdin);
        if (bytesRead == -1) {
            break;
        }

        // removing new line character
        char *newline = strchr(input, '\n');
        if (newline) {
            *newline = '\0'; // Replace '\n' with null terminator
        }

        // command string
        char *space = strchr(input, ' '); 
        if (space) {
            *space = '\0'; // Terminate command string
            args = strdup(space + 1); // copy string after space
        } else {
            args = NULL;
        }

        cmd = strdup(input);

        // if statements for file commands
        if (cmd && strcmp(cmd, "cd") == 0 && args) {
            changeDirectory(args);
        }
        else if (cmd && strcmp(cmd, "mkdir") == 0 && args) {
            createDirectory(args);
        }
        else if (cmd && strcmp(cmd, "touch") == 0 && args) {
            createFile(args);
        }
        else if (cmd && strcmp(cmd, "ls") == 0) {
            listContents();
        }
        else if (cmd && strcmp(cmd, "exit") == 0) {
            free(input);
            free(cmd);
            free(args);
            saveInodeList("inodes_list");
            return 0;
        }
        else {
            printf("Invalid Input\n");
        }

        free(cmd); 
        free(args);
    }

    free(input);
    saveInodeList("inodes_list");

    return 0;
}
