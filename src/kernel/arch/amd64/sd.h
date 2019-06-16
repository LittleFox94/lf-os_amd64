#ifndef _SD_H_INCLUDED
#define _SD_H_INCLUDED

#include "process.h"

enum ServiceType {
    ServiceTypeBlockDevice,
    ServiceTypeCharDevice,
    ServiceTypeFilesystem,
};

void init_sd();

#endif
