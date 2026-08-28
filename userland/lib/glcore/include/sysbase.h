#ifndef _GLCORE_SYSBASE_H
#define _GLCORE_SYSBASE_H

int sysVersion(char* out); // returns full version string
int sysVersionN(char* out, int size); // returns full version string with size limit

// TODO: implement
// these are here to define what will be implemented later
int sysVersionBuild(void); // returns build number
int sysVersionMajor(void); // returns major version
int sysVersionMinor(void); // returns minor version
int sysSetProcErrHandler(void* ptr); // low level function, probably shouldn't be called by normal code

#endif