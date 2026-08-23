#ifndef _KERNEL_PLOAD_H
#define _KERNEL_PLOAD_H

int pload_create_process_file(const char* path, char** args);
int pload_load_process(int pid, int tid);

#endif