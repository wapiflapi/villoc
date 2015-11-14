/* Testing for each alloc function from libc */

#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0

int 
main(void)
{
    int *cptr, *mptr, *vptr;

    cptr = calloc(8, 32);
    if(DEBUG)
        printf("cptr = %p\n", cptr);

    mptr = malloc(64);
    if(DEBUG)
        printf("mptr = %p\n", mptr);

    mptr = realloc(mptr, 128);
    if(DEBUG)
        printf("mptr = %p\n", mptr);

    vptr = valloc(64);
    if(DEBUG)
        printf("vptr = %p\n", vptr);

    vptr = reallocf(vptr, 128);
    if(DEBUG)
        printf("vptr = %p\n", vptr);

    free(cptr);
    free(mptr);
    free(vptr);

    return 0;
}

