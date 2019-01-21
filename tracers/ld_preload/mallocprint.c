#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

void *(*o_malloc)(size_t size);
void (*o_free)(void *ptr);
void *(*o_calloc)(size_t nmemb, size_t size);
void *(*o_realloc)(void *ptr, size_t size);
void *(*o_reallocarray)(void *ptr, size_t nmemb, size_t size);

__attribute__((constructor)) void test_malloc()
{
    o_malloc = dlsym(RTLD_NEXT, "malloc");
    o_free = dlsym(RTLD_NEXT, "free");
    o_calloc = dlsym(RTLD_NEXT, "calloc");
    o_realloc = dlsym(RTLD_NEXT, "realloc");
    o_reallocarray = dlsym(RTLD_NEXT, "reallocarray");
}

/* Not using fprintf because it can call malloc. */
void write_str(char const *str) {
    size_t len = strlen(str);
    while (len) {
        ssize_t done = write(1, str, len);
        if (done < 0) {
            exit(-1);
        }
        len -= done;
    }
}

#define NORETPROXY(NAME, FMT, ...)                                      \
    do {                                                                \
        char log[256];                                                  \
                                                                        \
        snprintf(log, sizeof log, #NAME "(" FMT ") = ", __VA_ARGS__);   \
        write_str(log);                                                 \
                                                                        \
        o_ ## NAME(__VA_ARGS__);                                        \
                                                                        \
        write_str("<void>\n");                                          \
        return;                                                         \
    } while (0)

#define RETPROXY(NAME, FMT, ...)                                        \
    do {                                                                \
        char log[256];                                                  \
        void *ret = 0;                                                  \
                                                                        \
        snprintf(log, sizeof log, #NAME "(" FMT ") = ", __VA_ARGS__);   \
        write_str(log);                                                 \
                                                                        \
        ret = o_ ## NAME(__VA_ARGS__);                                  \
                                                                        \
        snprintf(log, sizeof log, "%p\n", ret);                         \
        write_str(log);                                                 \
                                                                        \
        return ret;                                                     \
    } while (0)

void free(void *ptr) {
    NORETPROXY(free, "%p", ptr);
}

void *malloc(size_t size) {
    RETPROXY(malloc, "%zd", size);
}

void *calloc(size_t nmemb, size_t size) {
    RETPROXY(calloc, "%zd, %zd", nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    RETPROXY(realloc, "%p, %zd", ptr, size);
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
    RETPROXY(reallocarray, "%p, %zd, %zd", ptr, nmemb, size);
}
