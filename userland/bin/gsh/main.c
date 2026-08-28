#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ps.h>

#define MAX_BUF 1024

char** parse_args(const char* cmdline, int* out_argc) {
    char* cmdline_copy = strdup(cmdline);
    if (!cmdline_copy) return NULL;

    char** argv = malloc(MAX_BUF * sizeof(char*));
    if (!argv) {
        free(cmdline_copy);
        return NULL;
    }

    int argc = 0;
    char* token = strtok(cmdline_copy, " \t\r\n");

    while (token != NULL && argc < MAX_BUF - 1) {
        argv[argc] = strdup(token); 
        argc++;
        token = strtok(NULL, " \t\r\n");
    }
    
    argv[argc] = NULL; 
    *out_argc = argc;

    free(cmdline_copy);
    return argv;
}

void free_args(char** argv, int argc) {
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

void cd(int argc, char** argv) {
    if (argc < 2) {
        printf("cd: not enough arguments");
    } else {
        if (chdir(argv[1]) == -ENOENT) {
            printf("cd: directory does not exist\n");
        }
    }
}

int main(void) {
    char *pathevar = malloc(MAX_BUF);
    char *pathentries[256];
    char *pathevarret = psGetEVar("PATH", pathevar, MAX_BUF);
    if (pathevarret == NULL) {
        printf("gsh: no PATH was set, using empty string as default");
        pathevar[0] = '\0';
    }

    int pathentriescnt = 0;
    char *token = strtok(pathevar, ";");
    while (token != NULL) {
        pathentries[pathentriescnt++] = token;
        if (pathentriescnt >= 256) { break; }
        token = strtok(NULL, ";");
    }

    char cwd[MAX_BUF] = {0};

    for (;;) {
        char buffer[MAX_BUF];
        char ch;
        char fullpath[MAX_BUF*2];
        size_t i = 0;

        getcwd(cwd, MAX_BUF);
        printf("%s$ ",cwd);

        // Read 1 byte at a time from STDIN_FILENO (0)
        while (i < MAX_BUF - 1) {
            ssize_t n = read(STDIN_FILENO, &ch, 1);
            if (n <= 0) {
                printf("Got 0 from read()!!\n");
                break; 
            } // Error or EOF

            if (ch == 0x7F) {
                if (!i) { continue; }
                putchar(ch);
                buffer[i] = 0;
                i--;
            } else if (ch == '\n') { 
                putchar(ch);
                break;
            } else {
                putchar(ch);
                buffer[i++] = ch;
            }
        }
        buffer[i] = '\0'; // Null-terminate string
        if (buffer[0] == '\0') { continue; }

        int pid = -1;
        char **cmdargv;
        int cmdargc = 0;

        cmdargv = parse_args(buffer, &cmdargc);

        int ispath = 0;
        for (int x = 0; x < i; x++) 
            if (cmdargv[0][x] == ':' || cmdargv[0][x] == '/')
                ispath = 1;

        if (ispath) {
            pid = psExecA(cmdargv[0], cmdargv);
            if (pid < 0) {
                if (pid == -ENOENT) {
                    printf("File %s does not exist\n", buffer);
                } else {
                    printf("Could not execute %s (%i)\n", buffer, pid);
                }
            }
        } else {
            if (!strcmp(cmdargv[0], "cd")) {
                cd(cmdargc, cmdargv);
            } else {
                for (int x = 0; x < pathentriescnt; x++) {
                    sprintf(fullpath, "%s/%s.elf", pathentries[x], cmdargv[0]);
                    pid = psExecA(fullpath, cmdargv);
                    if (pid != -ENOENT) {
                        break;
                    }
                }
                if (pid < 0) {
                    if (pid == -ENOENT) {
                        printf("Invalid command %s\n", buffer);
                    } else {
                        printf("Could not execute %s (%i)\n", buffer, pid);
                    }
                }
            }
        }
        if (pid > -1)
            psWait(pid);
        free_args(cmdargv, cmdargc);
    }
}