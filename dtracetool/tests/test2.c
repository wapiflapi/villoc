#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0

int 
main(void)
{
    int *cptr, *mptr, *vptr, *rptr, *rfptr;

    /* Failing calloc */
    cptr = calloc(1337, 0xFFFFFFFFFFFF);
    if(DEBUG)
        printf("cptr = %p\n", cptr);

    /* Succeeding calloc */
    cptr = calloc(8, 32);
    if(DEBUG)
        printf("cptr = %p\n", cptr);

    /* Failing malloc */
    mptr = malloc(0xFFFFFFFFFFFFFF);
    if(DEBUG)
        printf("mptr = %p\n", mptr);

    /* Succeeding malloc */
    mptr = malloc(64);
    if(DEBUG)
        printf("mptr = %p\n", mptr);

    /* Failing realloc, we want to test if mptr is still valid */
    rptr = realloc(mptr, 0xFFFFFFFFFFFF);
    if(DEBUG)
        printf("mptr = %p\n", mptr);

    /* Succeeding realloc */
    mptr = realloc(mptr, 128);
    if(DEBUG)
        printf("mptr = %p\n", mptr);

    /* Failing valloc */
    vptr = valloc(0xFFFFFFFFFFFF);
    if(DEBUG)
        printf("vptr = %p\n", vptr);

    /* Succeeding valloc */
    vptr = valloc(64);
    if(DEBUG)
        printf("vptr = %p\n", vptr);

    /* Succeeding reallocf */
    vptr = reallocf(vptr, 128);
    if(DEBUG)
        printf("vptr = %p\n", vptr);

    /* Failing reallocf, we want to test if vptr is still valid */
    rfptr = reallocf(vptr, 0xFFFFFFFFFFFF);
    if(DEBUG)
        printf("vptr = %p\n", vptr);

    free(cptr);
    free(mptr);
//    free(vptr);

    return 0;
}

