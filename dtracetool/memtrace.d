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


pid$target::malloc:entry
{
    self->msize = arg0;
}

pid$target::malloc:return
/self->msize/
{
    printf("malloc(%d) = %#p\n", self->msize, arg1);
    self->msize = 0;
}

pid$target::valloc:entry
{
    self->vsize = arg0;
}

pid$target::valloc:return
/self->vsize/
{
    printf("valloc(%d) = %#p\n", self->vsize, arg1);
    self->vsize = 0;
}

pid$target::calloc:entry
{
    self->ccount = arg0;
    self->csize = arg1;
}

pid$target::calloc:return
/self->csize/
{
    printf("calloc(%d, %d) = %#p\n", self->ccount, self->csize, arg1);
    self->ccount = 0;
    self->csize = 0;
}

pid$target::realloc:entry
{
    self->raddr = arg0;
    self->rsize = arg1;
}

pid$target::realloc:return
/self->rsize/
{
    printf("realloc(%#p, %d) = %#p\n", self->raddr, self->rsize, arg1);
    self->rsize = 0;
    self->raddr = 0;
}

pid$target::reallocf:entry
{
    self->rfaddr = arg0;
    self->rfsize = arg1;
}

pid$target::reallocf:return
/self->rfsize/
{
    printf("reallocf(%#p, %d) = %#p\n", self->rfaddr, self->rfsize, arg1);
    self->rfaddr = 0;
    self->rfsize = 0;
}

pid$target::free:entry
{
    printf("free(%#p) = <void>\n", arg0);
}

