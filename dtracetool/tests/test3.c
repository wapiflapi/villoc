/* Testing for a crash inside of the malloc() */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  (void) argc, (void) argv;

  void *a, *b, *c;

  a = malloc(48);
  b = malloc(48);
  c = malloc(48);

  free(a);

  // Let's say we have an underflow on c. This
  // is far from the only way to trigger a crash.
  memset(c-128, 0xff, 0x200);

  printf("A crash in malloc will be triggered *after* this print.\n");
  malloc(48);

  printf("This shouldn't be reached but it is on OS X (unlike Ubuntu)\n");
  malloc(48);

  printf("However, this isn't reached\n");
  malloc(1337);

  return 0;
}

