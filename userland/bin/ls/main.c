#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fs.h>

#define MAX_BUF 1024

int main(int argc, char* argv[]) {
    char dirbuf[MAX_BUF];
    char fbuf[MAX_BUF];

    int dirs = 0;
    int files = 0;
    
    if (argc > 1) {
        dirs = fsList(argv[1], dirbuf, MAX_BUF-1, 1);
        files = fsList(argv[1], fbuf, MAX_BUF-1, 0);
    } else {
        char cwdbuf[MAX_BUF];
        getcwd(cwdbuf, MAX_BUF);
        dirs = fsList(cwdbuf, dirbuf, MAX_BUF-1, 1);
        files = fsList(cwdbuf, fbuf, MAX_BUF-1, 0);
    }

    char *token = NULL;
    if (dirs) {
        printf("dirs: ");
        token = strtok(dirbuf, "\n");
        while (token != NULL) {
            printf("%s ", token);
            token = strtok(NULL, "\n");
        }
        putchar('\n');
    }

    if (files) {
        printf("files: ");
        token = strtok(fbuf, "\n");
        while (token != NULL) {
            printf("%s ", token);
            token = strtok(NULL, "\n");
        }
        putchar('\n');
    }
    
    return 0;
}