#ifndef _GLCORE_PS_H
#define _GLCORE_PS_H

// TODO: implement
// these are here to define what will be implemented later
int fsOpen(char *path, int mode);
int fsClose(int fd);
int fsRead(int fd, void* buf, int count);
int fsWrite(int fd, void* buf, int count);
int fsSeek(int fd, int pos);

#endif