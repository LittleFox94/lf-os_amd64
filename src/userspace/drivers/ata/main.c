/**
 * This file contains mostly generic helper functions used for ATA Disk 
 * initializion & management, but no eg. disk reads or writes here.
 *
 * Be aware. that these functions require you to have IO permissions set
 * so that you can talk to ATA devices.
 *
 * License: MIT
 * Author: k4m1  <k4m1\protonmail.com>
 */

#include <sys/syscalls.h>
#include <sys/io.h>

#include <stdio.h>
#include <stdint.h>

#include "ata.h"

int main() {
    detect_ata_disks();

    do {} while (1);
}


