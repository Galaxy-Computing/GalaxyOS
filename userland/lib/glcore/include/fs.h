#ifndef _GLCORE_FS_H
#define _GLCORE_FS_H

#include <stddef.h>

int fsSize(int fd);
int fsList(const char* path, char* buf, size_t size, int dirs); // returns a list of files/directories in a directory, with '\n' being the delimiter

// TODO: implement
// these are here to define what will be implemented later
int fsOpen(char *path, int mode);
int fsClose(int fd);
int fsRead(int fd, void* buf, int count);
int fsWrite(int fd, void* buf, int count);
int fsSeek(int fd, int pos);

#endif