#include <stdio.h>
#include <stdlib.h>
__attribute__((noreturn)) static void panic_message(const char* message) { fprintf(stderr, "Panic() called: %s\n", message); exit(-1); }
__attribute__((noreturn)) static void panic() { panic_message("Unknown error"); }
