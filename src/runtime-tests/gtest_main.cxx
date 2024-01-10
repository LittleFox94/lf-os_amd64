#include <cstdio>
#include <string>

#include "gtest/gtest.h"

#include <sys/syscalls.h>
#include <sys/io.h>

int main(int argc, char **argv) {
    uint64_t error = 0;
    // access to port where we write the result
    sc_do_hardware_ioperm(0x1F05, 1, true, &error);
    // access to keyboard controller to reset the system
    sc_do_hardware_ioperm(0x60, 5, true, &error);

    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);

    std::stringstream xml_report;
    testing::XmlUnitTestResultPrinter xml_printer(&xml_report);
    testing::UnitTest::GetInstance()->listeners().Append(&xml_printer);

    int ret = RUN_ALL_TESTS();

    std::string xml = xml_report.str();
    for(auto it = xml.cbegin(); it != xml.cend(); it++) {
        outb(0x1F05, *it);
    }

    // reset the system via keyboard controller
    while(inb(0x64) & 2);
    outb(0x64, 0xFE);

    return ret;
}
