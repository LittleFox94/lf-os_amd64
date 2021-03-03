#include <fstream>
#include <iostream>

#include <9p/helper/server_starter>

#include "tarfilesystem.h"

extern const char _binary_initramfs_tar_start,
                  _binary_initramfs_tar_end;

int lfos_initramfs_init() {
    std::cout << "LF OS init starting up" << std::endl;

    const std::string file_path = "/term";
    std::ifstream file;
    file.exceptions(std::ifstream::failbit);

    try {
        file.open(file_path);
    }
    catch(std::ios_base::failure& f) {
        std::cerr << f.what() << std::endl;
    }

    std::cout << file_path << " has " << (file ? "" : "not") << " been opened" << std::endl;
    while(true);
    return 0;
}

int main(int argc, char* argv[]) {
    auto fs = nineptar::TarFileSystem(
        std::string(
            &_binary_initramfs_tar_start,
            &_binary_initramfs_tar_end - &_binary_initramfs_tar_start
        )
    );

    uint64_t fsQueue = 0;
    lib9p::ServerStarter server(fsQueue, true);

    server.addFileSystem("", &fs);

    pid_t pid = fork();
    if(pid == 0) {
        return lfos_initramfs_init();
    }
    else if(pid > 0) {
        std::cout << "Forked for LF OS initramfs init to PID " << pid << std::endl;
    }
    else {
        std::cout << "Error forking for LF OS initramfs init: " << std::strerror(-pid) << std::endl;
        return -1;
    }

    std::cout << "Going into main-loop, listening on queue " << fsQueue << std::endl;
    server.loop();
}
