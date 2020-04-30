#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("Hello world! Forking ...\n");

    for(int i = 0; i < 4; ++i) {
        int pid = fork();

        if(pid) {
            printf("Forked to process %d\n", pid);
        } else {
            printf("I'm a fork (%d)\n", i);
            return 0;
        }
    }

    return 0;
}
