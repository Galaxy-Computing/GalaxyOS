#ifndef _UNISTD_H
#define _UNISTD_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int brk(void *addr);
void *sbrk(intptr_t increment);

ssize_t write(int fd, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);

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