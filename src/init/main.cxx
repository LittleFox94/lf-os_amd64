#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
    std::cout << "Hello world! Forking .." << std::endl;

    for(int i = 0; i < 4; ++i) {
        int pid = fork();

        if(pid == 0) {
            std::cout << "I'm a fork (number " << i << ")" << std::endl;
            return EXIT_SUCCESS;
        }
        else if(pid > 0) {
            std::cout << "Forked to pid " << pid << std::endl;
        }
    }

    throw new std::exception();

    return EXIT_SUCCESS;
}
