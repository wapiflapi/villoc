#!/usr/sbin/dtrace -s

/*
memtrace.d - DTrace-based memory allocation tracer
Andrzej Dyjak <dyjakan@sigsegv.pl>

Based on ltrace output style. Compatible with villoc [1].

$ sudo dtrace ./memtrace.d -p <pid>
$ sudo dtrace ./memtrace.d -c <application>

NOTE: For bigger programs you may stumble upon data drops. This can be mitigated
to a certain degree via DTrace tuning [2] [3].

[1] https://github.com/wapiflapi/villoc
[2] https://wikis.oracle.com/display/DTrace/Buffers+and+Buffering
[3] https://wikis.oracle.com/display/DTrace/Options+and+Tunables
*/

/* Re-enable to see potential data drops */
#pragma D option quiet

/* Globals for alloc functions args. Explicitly typed to avoid errors. */
size_t malloc_size;
size_t valloc_size;
size_t calloc_count;
size_t calloc_size;
int64_t realloc_addr;
size_t realloc_size;
int64_t reallocf_addr;
size_t reallocf_size;
int64_t free_addr;

BEGIN
{
    /* Flags for failure inside of the functions */
    /* However, we're using them also for predicates. This is a hack. */
    malloc_fail = 0;
    valloc_fail = 0;
    calloc_fail = 0;
    realloc_fail = 0;
    reallocf_fail = 0;
    free_fail = 0;
}


pid$target::malloc:entry
{
    malloc_size = arg0;
    malloc_fail = 1;
}

pid$target::malloc:return
/malloc_fail/
{
    printf("malloc(%d) = %#p\n", malloc_size, arg1);
    malloc_fail = 0;
}

pid$target::valloc:entry
{
    valloc_size = arg0;
    valloc_fail = 1;
}

pid$target::valloc:return
/valloc_fail/
{
    printf("valloc(%d) = %#p\n", valloc_size, arg1);
    valloc_fail = 0;
}

pid$target::calloc:entry
{
    calloc_count = arg0;
    calloc_size = arg1;
    calloc_fail = 1;
}

pid$target::calloc:return
/calloc_fail/
{
    printf("calloc(%d, %d) = %#p\n", calloc_count, calloc_size, arg1);
    calloc_fail = 0;
}

pid$target::realloc:entry
{
    realloc_addr = arg0;
    realloc_size = arg1;
    realloc_fail = 1;
}

pid$target::realloc:return
/realloc_fail/
{
    printf("realloc(%#p, %d) = %#p\n", realloc_addr, realloc_size, arg1);
    realloc_fail = 0;
}

pid$target::reallocf:entry
{
    reallocf_addr = arg0;
    reallocf_size = arg1;
    reallocf_fail = 1;
}

pid$target::reallocf:return
/reallocf_fail/
{
    printf("reallocf(%#p, %d) = %#p\n", reallocf_addr, reallocf_size, arg1);
    reallocf_fail = 0;
}

pid$target::free:entry
{
    free_addr = arg0;
    printf("free(%#p) = <void>\n", free_addr);
}

END
/malloc_fail/
{
    printf("malloc(%d) = <error>\n", malloc_size);
}

END
/valloc_fail/
{
    printf("valloc(%d) = <error>\n", valloc_size);
}

END
/calloc_fail/
{
    printf("calloc(%d, %d) = <error>\n", calloc_count, calloc_size);
}

END
/realloc_fail/
{
    printf("realloc(%#p, %d) = <error>\n", realloc_addr, realloc_size);
}

END
/reallocf_fail/
{
    printf("realloc(%#p, %d) = <error>\n", reallocf_addr, reallocf_size);
}
