#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "guru.h"
#include "bytecode.h"
#include "value.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
    // struct chunk *c = test_ch();
    // run(vm, c);

    struct guru_vm *vm = vm_init();

    if (argc == 1)
        execf(vm, "main.guru");
        // repl(vm);
    else if (argc == 2) 
        execf(vm, argv[1]);
    else {
        fprintf(stderr, "usage: guru [script]");
        exit(64);
    }
    vm_free(vm);
    return 0;
}
