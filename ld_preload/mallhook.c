#define _GNU_SOURCE

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#define STR_NX(A) #A
#define STR(A) STR_NX(A)

typedef enum {
  MALLOC,
  CALLOC,
  REALLOC,
  FREE
} func;

static int outfile = -1;

static void *(*r_malloc)(size_t) = NULL;
static void *(*r_calloc)(size_t, size_t) = NULL;
static void *(*r_realloc)(void *, size_t) = NULL;
static void (*r_free)(void *) = NULL;

static void mtrace_init(func name) {

  switch (name) {
  case MALLOC:
    r_malloc = dlsym(RTLD_NEXT, "malloc");
    if (r_malloc == NULL)
      goto dl_error;
    break;

  case CALLOC:
    r_calloc = dlsym(RTLD_NEXT, "calloc");
    if (r_calloc == NULL)
      goto dl_error;
    break;

  case REALLOC:
    r_realloc = dlsym(RTLD_NEXT, "realloc");
    if (r_realloc == NULL)
      goto dl_error;
    break;

  case FREE:
    r_free = dlsym(RTLD_NEXT, "free");
    if (r_free == NULL) 
      goto dl_error;
    break;
  }

  if (outfile == -1) {
    char *outname = NULL;

    if ((outname = getenv("MALLHOOK_OUT")) == NULL)
      outname = "output";
    outfile = open(outname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    
    if (outfile == -1) {
      fprintf(stderr, "[Mallhook] failed to open output file\n");
    }
  }
  return;

 dl_error:
  fprintf(stderr, "[Mallhook] dlsym error: %s\n", dlerror());
  exit(EXIT_FAILURE);
}

void *malloc(size_t size) {
  int n = 0;
  char buf[256] = {};
  void *p = NULL;

  if (r_malloc == NULL) {
    mtrace_init(MALLOC);
  }
  n = snprintf(buf, 256, "malloc(%lu) = ", size);

  p = r_malloc(size);

  if (p != NULL)
    snprintf(buf + n, 256 - n, "%p\n", p);
  else
    snprintf(buf + n, 256 - n, "0\n");
  (void) write(outfile, buf, strlen(buf));
  return p;
}

void *calloc(size_t nmemb, size_t size) {
  int n = 0;
  char buf[256] = {};
  void *p = NULL;

  if (r_calloc == NULL) {
    mtrace_init(CALLOC);
  }
  n = snprintf(buf, 256, "calloc(%lu, %lu) = ", nmemb, size);

  p = r_calloc(nmemb, size);

  if (p != NULL)
    snprintf(buf + n, 256 - n, "%p\n", p);
  else
    snprintf(buf + n, 256 - n, "0\n");
  (void) write(outfile, buf, strlen(buf));
  return p;
}

void *realloc(void *ptr, size_t size) {
  int n = 0;
  char buf[256] = {};
  void *p = NULL;

  if (r_realloc == NULL) {
    mtrace_init(REALLOC);
  }
  if (ptr != NULL) /* just to display ltrace's style */
    n = snprintf(buf, 256,  "realloc(%p, %lu) = ", ptr, size);
  else
    n = snprintf(buf, 256,  "realloc(0, %lu) = ", size);

  p = r_realloc(ptr, size);

  if (p != NULL)
    snprintf(buf + n, 256 - n, "%p\n", p);
  else
    snprintf(buf + n, 256 - n, "0\n");
  (void) write(outfile, buf, strlen(buf));
  return p;
}

void free(void *ptr) {
  char buf[256] = {};

  if (r_free == NULL) {
    mtrace_init(FREE);
  }
  if (ptr != NULL)
    snprintf(buf, 256, "free(%p) = <void>\n", ptr);
  else
    snprintf(buf, 256, "free(0) = <void>\n");

  r_free(ptr);
  (void) write(outfile, buf, strlen(buf));
}

/*
  this is necessary to properly close the file
*/
void __attribute__((noreturn)) exit(int status) {
  close(outfile);
#ifdef __i386__
  __asm__("movl $1, %%eax\n\t"
      "movl %0, %%ebx\n\t"
      "int 0x80"
      ::"r"(status));
#elif defined __x86_64__
  __asm__("movq $60, %%rax\n\t"
      "movl %0, %%ebx\n\t"
      "syscall"
      ::"r"(status));
#endif
  while (1);
}
