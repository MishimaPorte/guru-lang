#include "../hashmap.h"
#include <stdint.h>
#include <stdio.h>
uint8_t hashmap_test() {
    struct hashmap hm;
    init_hashmap(&hm);
    set_val(&hm, "eek", 3, "au4");
    printf("val: %s\n", get_val(&hm, "eek", 3));
    set_val(&hm, "eek", 3, "au4au4");
    printf("val: %s\n", get_val(&hm, "eek", 3));
    unset_val(&hm, "eek", 3);
    printf("no_val: %d\n", get_val(&hm, "eek", 3) == NULL);
    return 0;
};
