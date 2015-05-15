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

#pragma D option destructive
#pragma D option bufsize=256m


pid$target::malloc:entry
{
    msize = arg0;
    malloc_fail = 1
}

pid$target::malloc:return
/msize/
{
    printf("malloc(%d) = %#p\n", msize, arg1);
    msize = 0;
    malloc_fail = 0;
}

pid$target::valloc:entry
{
    vsize = arg0;
    valloc_fail = 1;
}

pid$target::valloc:return
/vsize/
{
    printf("valloc(%d) = %#p\n", vsize, arg1);
    vsize = 0;
    valloc_fail = 0;
}

pid$target::calloc:entry
{
    ccount = arg0;
    csize = arg1;
    calloc_fail = 1;
}

pid$target::calloc:return
/csize/
{
    printf("calloc(%d, %d) = %#p\n", ccount, csize, arg1);
    ccount = 0;
    csize = 0;
    calloc_fail = 0;
}

pid$target::realloc:entry
{
    raddr = arg0;
    rsize = arg1;
    realloc_fail = 1;
}

pid$target::realloc:return
/rsize/
{
    printf("realloc(%#p, %d) = %#p\n", raddr, rsize, arg1);
    rsize = 0;
    raddr = 0;
    realloc_fail = 0;
}

pid$target::reallocf:entry
{
    rfaddr = arg0;
    rfsize = arg1;
    reallocf_fail = 1;
}

pid$target::reallocf:return
/rfsize/
{
    printf("reallocf(%#p, %d) = %#p\n", rfaddr, rfsize, arg1);
    rfaddr = 0;
    rfsize = 0;
    reallocf_fail = 0;
}

pid$target::free:entry
{
    printf("free(%#p) = <void>\n", arg0);
}

dtrace:::END
/malloc_fail == 1/
{
    printf("malloc(%d) = <error>\n", msize);
}

dtrace:::END
/valloc_fail == 1/
{
    printf("valloc(%d) = <error>\n", vsize);
}

dtrace:::END
/calloc_fail == 1/
{
    printf("calloc(%d, %d) = <error>\n", ccount, csize);
}

dtrace:::END
/realloc_fail == 1/
{
    printf("realloc(%#p, %d) = <error>\n", raddr, rsize);
}

dtrace:::END
/reallocf_fail == 1/
{
    printf("reallocf(%#p, %d) = <error>\n", rfaddr, rfsize);
}

