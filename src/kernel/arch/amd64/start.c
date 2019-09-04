#include "../../../loader/loader.h"

void main(LoaderStruct* loaderStruct);

void _start(LoaderStruct* loaderStruct) {
    // remove stack base before loader for clean stack traces
    //   (invalid anyway)
    asm("mov $0, %rbp");

    main(loaderStruct);
}
