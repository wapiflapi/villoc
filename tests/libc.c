#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char **argv) {

    void *a, *b, *c, *d;

    a = malloc(1024);
    b = strdup("some string");
    c = fopen("/dev/null", "r");
    d = getcwd(NULL, 4096);

    free(a);
    free(b);
    c = realloc(c, 0);
    d = realloc(d, 0);

    return 0;

}
