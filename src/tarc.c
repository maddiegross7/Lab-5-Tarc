//Lab 5: Tarc
//Madelyn Gross
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

//just creates the stat struct and does the checking so I do not have to
struct stat getStats(const char *fileItem){
    struct stat fileItemStats;
    
    if(stat(fileItem, &fileItemStats) != 0){
        fprintf(stderr, "%s cannot be accessed", fileItem);
        exit(1);
    }
    return fileItemStats;
}

//This will be the information that I need for each of the files,
//I do not think it needs to be here necessarily but it helps me to keep it organized
typedef struct file{
    int nameSize;
    char *name;
    long iNodeValue;
    int fileMode;
    long secondsOfLastMod;
    long fileSize;
    int isNewInode;
    char *relativePath;//because this is annoying
} File;

File* initializeFileItem(){
    File *file = malloc(sizeof(File));

    file->nameSize = 0;
    file->name = NULL;
    file->iNodeValue = 0;
    file->fileMode = 0;
    file->secondsOfLastMod = 0;
    file->fileSize = 0;
    file->isNewInode = 0;
    file->relativePath = NULL;

    return file;
}

void freeFileItem(File *file){
    if(file == NULL){
        return;
    }
    if(file->name){
        free(file->name);
    }
    if(file->relativePath){
        free(file->relativePath);
    }
    free(file);
}

//this was just made because I needed to be able to close the directory 
//and this was the only necessary info
typedef struct pathInfo{
    char *realPath;
    char *relativePath;
}PathInfo;

PathInfo* initializePathInfo(){
    PathInfo *pathInfo = malloc(sizeof(PathInfo));

    pathInfo->realPath = NULL;
    pathInfo->relativePath = NULL;

    return pathInfo;
}

void freePathInfo(PathInfo *path){
    if(path == NULL){
        return;
    }
    if(path->realPath){
        free(path->realPath);
    }
    if(path->relativePath){
        free(path->relativePath);
    }
    free(path);
}

//just checks that the file is a directory or not
int isDirectory(struct stat stats){
    if(S_ISDIR(stats.st_mode)){
        return 1;
    }else{
        return 0;
    }
}

//I use this to insert the long into the jrb tree since there is not already made
//function
int compareJvalLong(Jval a, Jval b) {
    if (a.l < b.l){ 
        return -1;
    }  
    if (a.l > b.l) {
        return 1;
    }
    return 0;
}

//this prints all the stuff we can get from struct dirent or stats struct
void printFileDetails(File *thisFile) {
    if (thisFile == NULL) return; 

    fwrite(&thisFile->nameSize, sizeof(int), 1, stdout);
    fwrite(thisFile->relativePath, thisFile->nameSize, 1, stdout);
    fwrite(&thisFile->iNodeValue, sizeof(long), 1, stdout);

    if(thisFile->isNewInode == 1){
        fwrite(&thisFile->fileMode, sizeof(int), 1, stdout);
        fwrite(&thisFile->secondsOfLastMod, sizeof(long), 1, stdout);
    }

}

//this is my recursive function
void openItem(const char *fileItemName, JRB jrbTree, const char *relativePath){
    struct stat directoryStats = getStats(fileItemName);
    struct dirent *fileItem;

    //ignoring symbolic links
    if(S_ISLNK(directoryStats.st_mode)){
        return;
    }

    //this is just assigning values to the struct that can be gotten from stats struct
    File *thisFile = initializeFileItem();

    thisFile->fileMode = directoryStats.st_mode;
    thisFile->fileSize = directoryStats.st_size;
    thisFile->nameSize = strlen(relativePath);
    thisFile->name = strdup(fileItemName);
    thisFile->relativePath = strdup(relativePath);
    thisFile->iNodeValue = directoryStats.st_ino;

    //does this inode exist in the jrb tree yet?
    //No? insert it and get the seconds of last mod mark as new
    Jval iNodeValue = new_jval_l(directoryStats.st_ino);
    if(jrb_find_gen(jrbTree, iNodeValue, compareJvalLong)==0){
        jrb_insert_gen(jrbTree, iNodeValue, new_jval_l(directoryStats.st_ino), compareJvalLong);
        thisFile->secondsOfLastMod = directoryStats.st_mtime;
        thisFile->isNewInode = 1;
    }
    //print the file details
    printFileDetails(thisFile);

    //if the fileItem is a directory you will open it as well
    if(isDirectory(directoryStats) == 1){
        DIR *directory = opendir(fileItemName);
        if(directory == NULL){
            fprintf(stderr, "Failed to open directory");
            exit(1);
        }
        //create a list so you can get all the files and close the directory
        Dllist filesInDirectory = new_dllist();
        while((fileItem = readdir(directory)) != NULL){
            //ignoring . and ..
            if (strcmp(fileItem->d_name, ".") == 0 || strcmp(fileItem->d_name, "..") == 0) {
                continue;
            }

            //getting the path for the file, this is a bitch and annoying af
            int previousPathSize = strlen(fileItemName);
            int previousRelativePathSize = strlen(relativePath);
            int fileNameSize = strlen(fileItem->d_name);
            int filePathSize = previousPathSize + fileNameSize + 2;
            int relativePathSize = previousPathSize + fileNameSize + 2;
            PathInfo *path = initializePathInfo();
            path->realPath = malloc(filePathSize);
            path->relativePath = malloc(relativePathSize);
            snprintf(path->realPath, filePathSize,"%s/%s", fileItemName, fileItem->d_name);
            snprintf(path->relativePath, relativePathSize,"%s/%s", relativePath, fileItem->d_name);
            dll_append(filesInDirectory, new_jval_v((void *) path));
        }
        //closing the directory since we are limited and do not want to have extras open
        closedir(directory);
        Dllist ptr;
        //go through the tree and recursively call the function so that we can open the next ones
        //make sure you free each of the paths because you need to
        dll_traverse(ptr, filesInDirectory){
            PathInfo *thisPath = (PathInfo *) ptr->val.v;
            openItem(thisPath->realPath, jrbTree, thisPath->relativePath);
            freePathInfo(thisPath);
        }
        //freeing dllist every time
        free_dllist(filesInDirectory);
    }else{
        //if this is a file we are going to open the file and print out its size and contents
        //then we have to free all that stuff and make sure to close the file again
        if(thisFile->isNewInode == 1){
            FILE *theFile = fopen(thisFile->name, "rb");
            if(!theFile){
                fprintf(stderr, "Error opening file: %s", thisFile->name);
                exit(1);
            }

            char *buffer = malloc(thisFile->fileSize);
            size_t bytesRead = fread(buffer, 1, thisFile->fileSize, theFile);
            fwrite(&thisFile->fileSize, sizeof(long), 1, stdout);
            fwrite(buffer, sizeof(char), thisFile->fileSize, stdout);
            
            free(buffer);
            fclose(theFile);
        }
    }
    //free item
    freeFileItem(thisFile);
}


int main(int argc, char const *argv[])
{
    if(argc != 2){
        fprintf(stderr, "Usage: ./bin/tarc <directory name>");
        exit(1);
    }
    //this is where we will store the iNodes
    JRB iNodes = make_jrb();

    //this is the relative path it is what you will print in tar file
    const char *relativePath = strrchr(argv[1], '/');
    
    if(relativePath){
        relativePath++;
    }else{
        relativePath = argv[1];
    }
    //the first call
    openItem(argv[1], iNodes, relativePath);
    
    jrb_free_tree(iNodes);

    return 0;
}
