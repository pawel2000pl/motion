#include "find_ext_mem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>

#define STR_BUF_LEN 1024
#define END_DIR_NAME "MotionVideos"

int checkTrigger(const char* str)
{
    return 0==strncmp(str, EXT_MEM_TRIGGER, strlen(EXT_MEM_TRIGGER));    
}

int isDirectory(const char *path) 
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

int isFile(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int checkDevice(const char* dir)
{
    struct stat st = {0};
    if (stat(dir, &st) == -1) 
        mkdir(dir, 0777);
    
    char buf[STR_BUF_LEN] = {0};
    int i;
    for (i=0;dir[i];i++)
        buf[i] = dir[i];
    buf[i] = '/';
    strcat(buf, "/.testFile.tmp");
    FILE* testFile = fopen(buf, "w");
    if (testFile == NULL)
        return 0;
    
    fclose(testFile);
    remove(buf);
    return -1;
}

int findPathToDevice(const char* dirPath, char* foundPathBuf)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(dirPath);
    if (d) 
    {
        while ((dir = readdir(d)) != NULL) 
        {
            if (isDirectory(dir->d_name) == 0)
            {
                char buf[STR_BUF_LEN] = {0};
                int i;
                for (i=0;dirPath[i];i++)
                    buf[i] = dirPath[i];
                buf[i] = '/';
                strcat(buf, dir->d_name);
                strcat(buf, "/");
                strcat(buf, END_DIR_NAME);
                if (checkDevice(buf))
                {
                    for (i=0;i<STR_BUF_LEN;i++)
                        foundPathBuf[i] = 0;
                    for (i=0;i<buf[i];i++)
                        foundPathBuf[i] = buf[i];
                    foundPathBuf[i++] = '/';
                    foundPathBuf[i++] = 0;
                    closedir(d);
                    return -1;
                }
            }
        }
        closedir(d);
    }   
    return 0;
}

int findPathInMedia(char* foundPathBuf)
{
    DIR *d;
    struct dirent *dir;
    d = opendir("/media");
    if (d) 
    {
        while ((dir = readdir(d)) != NULL) 
        {
            if (isDirectory(dir->d_name) == 0)
            {
                char buf[STR_BUF_LEN] = {'/', 'm', 'e', 'd', 'i', 'a', '/', 0};
                strcat(buf, dir->d_name);
                if (findPathToDevice(buf, foundPathBuf))
                {
                    closedir(d);
                    return -1;
                }
            }
        }
        closedir(d);
    }  
    return 0;
}

int replacePath(const char** dir)
{
    char foundPathBuf[STR_BUF_LEN] = {0};
    int result = findPathInMedia(foundPathBuf);
    if (!result)
        for (int i=0;END_DIR_NAME[i];i++)
            foundPathBuf[i] = END_DIR_NAME[i];
    strcat(foundPathBuf, (*dir)+strlen(EXT_MEM_TRIGGER));         
    char* buf = malloc(strlen(foundPathBuf)+1);
    int i;
    for (i=0;foundPathBuf[i];i++)
       buf[i] = foundPathBuf[i];
    buf[i] = 0;
    free((void*)(*dir));
    (*dir) = buf;
    return result;
}


int replacePathIfTriggered(const char** dir)
{
    if (dir && (*dir) && checkTrigger(*dir))
        return replacePath(dir);
    return 0;
}

unsigned long long int getFreeSpaceOnDisk(const char* path)
{
    struct statfs sStats;
    if( statfs(path, &sStats) == -1 )
        return 0;
    return sStats.f_bsize * sStats.f_bavail;
}

unsigned long long int getFreeSpaceOnDisk2(const char* path)
{
    char buf[STR_BUF_LEN] = {0};
    int i = 0;
    for (i=0;path[i];i++)
        buf[i] = path[i];
    unsigned long long int size = 0;
    while (size == 0 && i > 0)
    {
        size = getFreeSpaceOnDisk(buf);
        if (!size)
        {
            for (;i && buf[i] != '/'; i--)
                buf[i] = 0;
            buf[i] = 0;
        }   
    }
    return size;
}

const char *get_filename_ext(const char *filename) 
{
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int validateMediaExt(const char* fileName)
{
    const char* allowedExtensions[] = {"avi", "swf", "flv", "mov", "mp4", "mkv", "webm", NULL};
    const char* ext = get_filename_ext(fileName);
    for (int i=0; allowedExtensions[i]; i++)
        if (strcmp(ext, allowedExtensions[i])==0)
            return -1;        
    return 0;
}

char oldest[STR_BUF_LEN] = {0};
time_t mtime = 0;

int check_if_older(const char *path, const struct stat *sb, int typeflag) 
{
    if (typeflag == FTW_F && (mtime == 0 || sb->st_mtime < mtime) && validateMediaExt(path)) 
    {
        mtime = sb->st_mtime;
        strncpy(oldest, path, STR_BUF_LEN);
    }
    return 0;
}

void clearSpaceInDir(const char* path, unsigned long long int neededSpace)
{
    while (getFreeSpaceOnDisk(path) < neededSpace)
    {
        for (int i=0;i<STR_BUF_LEN;i++)
            oldest[i] = 0;
        mtime = 0;
        ftw(path, check_if_older, 1); 
        if (!oldest[0])
            return;
        remove(oldest);
    }
}

void clearSpaceInDir2(const char* path, unsigned long long int neededSpace)
{
    char buf[STR_BUF_LEN] = {0};
    int i = 0;
    for (i=0;path[i];i++)
        buf[i] = path[i];
    
    while (!isDirectory(buf))
    {
        for (;i && buf[i] != '/'; i--)
                buf[i] = 0;
            buf[i] = 0;
    }
    
    clearSpaceInDir(buf, neededSpace);
}

/*
int main (int argc, char *argv[])
{
    printf("%llu\n", getFreeSpaceOnDisk2("/media/pawel24pl/PAWE/"));
    printf("%llu\n", getFreeSpaceOnDisk2("/media/pawel24pl/PAWE"));
    printf("%llu\n", getFreeSpaceOnDisk2("/media/pawel24pl/PAWE/NonExistingFile"));
    printf("%llu\n", getFreeSpaceOnDisk2("/home"));
    //clearSpaceInDir("/media/pawel24pl/VirualDevice/MotionVideos", 10); 
    
    printf("%d\n", validateMediaExt("test.tiff"));
    printf("%d\n", validateMediaExt("test.mp4"));
    printf("%d\n", validateMediaExt("test.webm"));

    return 0;
}
*/


