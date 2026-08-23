#ifndef _GLCORE_PS_H
#define _GLCORE_PS_H

// path = path of file to execute
// args = array of arguments to the resulting process
// returns resulting pid, or -1 on error
int psExecA(const char* path, const char* args[]);

__attribute__((__noreturn__))
void psExit(int code);

// TODO: implement
// these are here to define what will be implemented later
int psExecS(const char* path, const char* args);
int psName(int pid, char* out);
int psNameN(int pid, char* out, int size);
int psWait(int pid); // wait for a process to exit, returns exit code
int psKill(int pid);

#endif