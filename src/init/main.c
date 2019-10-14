#include <stdio.h>

int main() {
    printf("Hello world! Forking ...");

    for(int i = 0; i < 4; ++i) {
        int pid = fork();

        if(pid) {
            printf("Forked to process %d", pid);
        } else {
            printf("I'm a fork (%d)", i);
            return 0;
        }
    }

    while(1);

    return 2;
}
