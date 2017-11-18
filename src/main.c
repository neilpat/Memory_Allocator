#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

    sf_mem_init();

    void *x = sf_malloc(1200);
    void *y = sf_realloc(x, 1200);
    sf_snapshot();
    if(y);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
