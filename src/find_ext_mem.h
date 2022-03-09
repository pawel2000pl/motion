#ifndef _FIND_EXT_MEM_
#define _FIND_EXT_MEM_

#define EXT_MEM_TRIGGER "$EXT_MEM"

int replacePath(const char** dir);
int replacePathIfTriggered(const char** dir);
unsigned long long int getFreeSpaceOnDisk(const char* path);
unsigned long long int getFreeSpaceOnDisk2(const char* path);
void clearSpaceInDir(const char* path, unsigned long long int neededSpace);
void clearSpaceInDir2(const char* path, unsigned long long int neededSpace);


#endif
