#ifndef _GLCORE_PS_H
#define _GLCORE_PS_H

#include <stddef.h>

// path = path of file to execute
// args = array of arguments to the resulting process
// returns resulting pid, or -1 on error
int psExecA(const char* path, char* args[]);

int psWait(int pid); // wait for a process to exit, returns exit code
char* psGetEVar(const char* name, char* buf, size_t size);
int psSetEVar(const char* name, const char* val, size_t size);

__attribute__((__noreturn__))
void psExit(int code);

// TODO: implement
// these are here to define what will be implemented later
int psExecS(const char* path, const char* args);
int psName(int pid, char* out);
int psNameN(int pid, char* out, int size);

int psKill(int pid);

#endif