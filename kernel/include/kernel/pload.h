#ifndef _KERNEL_PLOAD_H
#define _KERNEL_PLOAD_H

int pload_create_process_file(const char* path, char** args, int nodup);
int pload_create_process(const char* data, const uint8_t privilege, const char* name, char** args, int nodup);
int pload_load_process(int pid, int tid);

#endif