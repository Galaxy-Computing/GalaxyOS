#ifndef _UNISTD_H
#define _UNISTD_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

// these are not implemented, just here to make libgcc build without errors
pid_t fork(void);
int execv(const char *path, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
pid_t getpid(void);

#ifdef __cplusplus
}
#endif

#endif