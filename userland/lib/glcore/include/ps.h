#ifndef _GLCORE_PS_H
#define _GLCORE_PS_H

#define INTERFACE_FLAG_PRIVATE 1

#include <stddef.h>

// path = path of file to execute
// args = array of arguments to the resulting process
// returns resulting pid, or -1 on error
int psExecA(const char* path, char* args[]);

int psWait(int pid); // wait for a process to exit, returns exit code
char* psGetEVar(const char* name, char* buf, size_t size);
int psSetEVar(const char* name, const char* val, size_t size);

int psThreadFork(void); // fork for threads, will return 0 in the spawned thread, but the thread id in the spawning thread

__attribute__((__noreturn__))
void psExit(int code);

int psInterfaceCreate(const char* name, int flags);
int psInterfaceDelete(const char* name);
int psInterfaceSend(const char* name, int pid, void* buf, size_t size); // if pid is -1, any process can read the message, and if pid is 0 only the process that created the interface can read the message
int psInterfaceRecieve(const char* name, void* buf, size_t size, size_t* sizeout); // returns the pid of the sender process, will block if there are no messages
int psInterfaceRecieveNB(const char* name, void* buf, size_t size, size_t* sizeout); // will not block, instead will return an error if there are no messages
int psInterfaceWait(const char* name); // will block until a message that this process can recieve is in the queue

// TODO: implement
// these are here to define what will be implemented later
int psExecS(const char* path, const char* args);
int psName(int pid, char* out);
int psNameN(int pid, char* out, int size);

int psKill(int pid);

#endif