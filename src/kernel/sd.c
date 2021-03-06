#include "sd.h"

struct ServiceRegistration {
    enum ServiceType type;

    char                        displayName[255];
    char                        name[255];
    struct ServiceRegistration* parent;

    pid_t process;
};

// TODO: use better data structures
struct ServiceRegistration services[128] = {
    { .type = ServiceTypeFilesystem, .displayName = "init", .name = "/", .parent = 0, .process = 0 },
};

void init_sd() {
}
