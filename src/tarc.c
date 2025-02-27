#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "dllist.h"
#include "jval.h"
#include "jrb.h"

//./bin/mrd 2 1 15 y 50 ex1
//./bin/tarc ex1 

struct stat getStats(const char *fileItem){
    struct stat fileItemStats;
    
    if(stat(fileItem, &fileItemStats) != 0){
        fprintf(stderr, "%s cannot be accessed", fileItem);
        exit(1);
    }

    return fileItemStats;
}

typedef struct file{
    int nameSize;
    char *name;
    long iNodeValue;
    int fileMode;
    long secondsOfLastMod;
    long fileSize;
} File;

File* initializeFileItem(){
    File *file = malloc(sizeof(File));

    file->nameSize = 0;
    file->name = NULL;
    file->iNodeValue = 0;
    file->fileMode = 0;
    file->secondsOfLastMod = 0;
    file->fileSize = 0;

    return file;
}

int isDirectory(struct stat stats){
    

    if(S_ISDIR(stats.st_mode)){
        //printf("this is a directory\n");
        return 1;
    }else{
        //printf("This is not a directory\n");
        return 0;
    }
}

int compareJvalLong(Jval a, Jval b) {
    if (a.l < b.l){ 
        return -1;
    }  
    if (a.l > b.l) {
        return 1;
    }
    return 0;
}

void printFileDetails(File *thisFile) {
    if (thisFile == NULL) return; // Check for NULL pointer

    printf("\n----- File Details -----\n");

    if (thisFile->name != NULL) {
        printf("Name          : %s\n", thisFile->name);
    }

    if (thisFile->nameSize != 0) {
        printf("Name Size     : %d\n", thisFile->nameSize);
    }

    if (thisFile->iNodeValue != 0) {
        printf("INode Value   : %ld\n", thisFile->iNodeValue);
    }

    if (thisFile->fileMode != 0) {
        printf("File Mode     : %d\n", thisFile->fileMode);
    }

    if (thisFile->secondsOfLastMod != 0) {
        printf("Last Modified : %ld\n", thisFile->secondsOfLastMod);
    }

    if (thisFile->fileSize != 0) {
        printf("File Size     : %ld bytes\n", thisFile->fileSize);
    }

    printf("------------------------\n");
}



//for each file and directory
// - the size of the files name, 4 byte intger
// - the file's name no null character
// - the files idnodes, 8 byte long
// - the files mode, 4 byte int
// - the files last modification time, secs
void openItem(const char *fileItemName, JRB jrbTree){
    struct stat directoryStats = getStats(fileItemName);
    printf("%s\n", fileItemName);
    struct dirent *fileItem;

    File *thisFile = initializeFileItem();

    thisFile->fileMode = directoryStats.st_mode;
    thisFile->fileSize = directoryStats.st_size;
    thisFile->nameSize = strlen(fileItemName);
    thisFile->name = strdup(fileItemName);
    Jval iNodeValue = new_jval_l(directoryStats.st_ino);
    
    if(jrb_find_gen(jrbTree, iNodeValue, compareJvalLong)==0){
        jrb_insert_gen(jrbTree, iNodeValue, new_jval_l(directoryStats.st_ino), compareJvalLong);
        thisFile->iNodeValue = directoryStats.st_ino;
        thisFile->secondsOfLastMod = directoryStats.st_mtime;
    }else{
        //printf("repeating Inodes\n");
    }
    printFileDetails(thisFile);
    if(isDirectory(directoryStats) == 1){
        DIR *directory = opendir(fileItemName);
        
        while((fileItem = readdir(directory)) != NULL){

            if (strcmp(fileItem->d_name, ".") == 0 || strcmp(fileItem->d_name, "..") == 0) {
                continue;
            }
            int previousPathSize = strlen(fileItemName);
            int fileNameSize = strlen(fileItem->d_name);
            int filePathSize = previousPathSize + fileNameSize+2;
            char *filePath = malloc(filePathSize);
            snprintf(filePath, filePathSize,"%s/%s", fileItemName, fileItem->d_name);

            openItem(filePath, jrbTree);
        }
    }
}

//for each file
// - files size, 8 byte long
// - files bytes
void openFile(char *fileName){

}

int main(int argc, char const *argv[])
{
    if(argc != 2){
        fprintf(stderr, "Usage: ./bin/tarc <directory name>");
        exit(1);
    }

    JRB iNodes = make_jrb();


    openItem(argv[1], iNodes);
    

    return 0;
}
