#include <unistd.h>
#include <stdio.h>
#include <ps.h>

#define MAX_BUF 1024

int main(void) {
    for (;;) {
        char buffer[MAX_BUF];
        char ch;
        size_t i = 0;

        printf("gsh$ ");

        // Read 1 byte at a time from STDIN_FILENO (0)
        while (i < MAX_BUF - 1) {
            ssize_t n = read(STDIN_FILENO, &ch, 1);
            if (n <= 0) {
                printf("Got 0 from read()!!\n");
                break; 
            } // Error or EOF

            if (ch == '\n') break; // Stop when Enter is pressed

            buffer[i++] = ch;
        }
        buffer[i] = '\0'; // Null-terminate string

        printf("%s\n", buffer);
        int pid = psExecA(buffer, NULL);
        if (pid < 0) {
            printf("Could not execute %s\n", buffer);
        }
        psWait(pid);
    }
}