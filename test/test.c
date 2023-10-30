#include "../hashmap.h"
#include <stdio.h>
int main() {
    struct hashmap hm;
    init_hashmap(&hm);

    set_val(&hm, "kek", 3, "au1");
    set_val(&hm, "qek", 3, "au2");
    set_val(&hm, "wek", 3, "au3");
    set_val(&hm, "eek", 3, "au4");
    set_val(&hm, "rek", 3, "au5");
    printf("val: %s\n", get_val(&hm, "kek", 3));
}
