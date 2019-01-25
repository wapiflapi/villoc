#include "dr_api.h"
#include "dr_tools.h"
#include "drwrap.h"
#include "drmgr.h"

#define BUF_SIZE (1024)


int tls_idx;
file_t logfile;

#define LOGPRINTF(...) dr_fprintf(logfile, __VA_ARGS__)


void reset_buf(char *buf)
{
    for (size_t i = 0; i < BUF_SIZE; i++)
        buf[i] = 0;
}


void pre_malloc(void *wrapctx, OUT void **buf_ptr)
{
    void *drc = dr_get_current_drcontext();
    char *buf = drmgr_get_tls_field(drc, tls_idx);

    // another print is in buf, so ignoring this call
    if (buf[0] != 0)
    {
        *buf_ptr = NULL;
        return;
    }
    else
        *buf_ptr = buf;

    dr_snprintf(buf, BUF_SIZE, "malloc(%d", drwrap_get_arg(wrapctx, 0));
}


void post_malloc(void *wrapctx, void *buf_ptr)
{
    if (buf_ptr == NULL)
        return;

    LOGPRINTF("%s) = %p\n", buf_ptr, drwrap_get_retval(wrapctx));
    reset_buf(buf_ptr);
}


void pre_calloc(void *wrapctx, OUT void **buf_ptr)
{
    void *drc = dr_get_current_drcontext();
    char *buf = drmgr_get_tls_field(drc, tls_idx);

    // another print is in buf, so ignoring this call
    if (buf[0] != 0)
    {
        *buf_ptr = NULL;
        return;
    }
    else
        *buf_ptr = buf;

    dr_snprintf(buf, BUF_SIZE, "calloc(%zu, %zu", drwrap_get_arg(wrapctx, 0), drwrap_get_arg(wrapctx, 1));
}


void post_calloc(void *wrapctx, void *buf_ptr)
{
    if (buf_ptr == NULL)
        return;

    LOGPRINTF("%s) = %p\n", buf_ptr, drwrap_get_retval(wrapctx));
    reset_buf(buf_ptr);
}


void pre_realloc(void *wrapctx, OUT void **buf_ptr)
{
    void *drc = dr_get_current_drcontext();
    char *buf = drmgr_get_tls_field(drc, tls_idx);

    // another print is in buf, so ignoring this call
    if (buf[0] != 0)
    {
        *buf_ptr = NULL;
        return;
    }
    else
        *buf_ptr = buf;

    dr_snprintf(buf, BUF_SIZE, "realloc(%p, %zu", drwrap_get_arg(wrapctx, 0), drwrap_get_arg(wrapctx, 1));
}


void post_realloc(void *wrapctx, void *buf_ptr)
{
    if (buf_ptr == NULL)
        return;

    LOGPRINTF("%s) = %p\n", buf_ptr, drwrap_get_retval(wrapctx));
    reset_buf(buf_ptr);
}


void pre_reallocarray(void *wrapctx, OUT void **buf_ptr)
{
    void *drc = dr_get_current_drcontext();
    char *buf = drmgr_get_tls_field(drc, tls_idx);

    // another print is in buf, so ignoring this call
    if (buf[0] != 0)
    {
        *buf_ptr = NULL;
        return;
    }
    else
        *buf_ptr = buf;

    dr_snprintf(buf, BUF_SIZE, "reallocarray(%p, %zu, %zu", drwrap_get_arg(wrapctx, 0), drwrap_get_arg(wrapctx, 1), drwrap_get_arg(wrapctx, 2));
}


void post_reallocarray(void *wrapctx, void *buf_ptr)
{
    if (buf_ptr == NULL)
        return;

    LOGPRINTF("%s) = %p\n", buf_ptr, drwrap_get_retval(wrapctx));
    reset_buf(buf_ptr);
}


void pre_free(void *wrapctx, OUT void **buf_ptr)
{
    void *drc = dr_get_current_drcontext();
    char *buf = drmgr_get_tls_field(drc, tls_idx);

    // another print is in buf, so ignoring this call
    if (buf[0] != 0)
    {
        *buf_ptr = NULL;
        return;
    }
    else
        *buf_ptr = buf;

    dr_snprintf(buf, BUF_SIZE, "free(%p", drwrap_get_arg(wrapctx, 0));
}

void post_free(void *wrapctx, void *buf_ptr)
{
    if (buf_ptr == NULL)
        return;

    LOGPRINTF("%s) = <void>\n", buf_ptr, drwrap_get_retval(wrapctx));
    reset_buf(buf_ptr);
}


// If we use this its when we have access to sscanf,
// we assume we'll have strcmp also.
int strcmp(const char *s1, const char *s2);


void pre_sscanf(void *wrapctx, OUT void **buf_ptr)
{
    (void) buf_ptr;

    char const *str = drwrap_get_arg(wrapctx, 0);
    char const *fmt = drwrap_get_arg(wrapctx, 1);

    char const *arg1 = drwrap_get_arg(wrapctx, 2);
    char const *arg2 = drwrap_get_arg(wrapctx, 3);
    char const *arg3 = drwrap_get_arg(wrapctx, 4);
    char const *arg4 = drwrap_get_arg(wrapctx, 5);

    if (strcmp(fmt, "@villoc")) {
        return;
    }

    // This is probably a call for us to monitor.
    // Our format string is passed through sscanf's str since that
    // way it's not interpreted by the original call.

    LOGPRINTF("%s(", fmt);
    LOGPRINTF(str, arg1, arg2, arg3, arg4);
    LOGPRINTF(") = <void>\n");
}


void load_event(__attribute__((unused))void *drcontext,
                const module_data_t *mod,
                __attribute__((unused))bool loaded)
{
    app_pc malloc = (app_pc)dr_get_proc_address(mod->handle, "__libc_malloc");
    app_pc calloc = (app_pc)dr_get_proc_address(mod->handle, "__libc_calloc");
    app_pc realloc = (app_pc)dr_get_proc_address(mod->handle, "__libc_realloc");
    app_pc reallocarray = (app_pc)dr_get_proc_address(mod->handle, "__libc_reallocarray");
    app_pc free = (app_pc)dr_get_proc_address(mod->handle, "__libc_free");
    app_pc sscanf = (app_pc)dr_get_proc_address(mod->handle, "__isoc99_sscanf");

    if (!sscanf)
        sscanf = (app_pc)dr_get_proc_address(mod->handle, "sscanf");

    if (malloc)
        DR_ASSERT(drwrap_wrap(malloc, pre_malloc, post_malloc));

    if (calloc)
        DR_ASSERT(drwrap_wrap(calloc, pre_calloc, post_calloc));

    if (realloc)
        DR_ASSERT(drwrap_wrap(realloc, pre_realloc, post_realloc));

    if (free)
        DR_ASSERT(drwrap_wrap(free, pre_free, post_free));

    if (reallocarray)
        DR_ASSERT(drwrap_wrap(reallocarray, pre_reallocarray, post_reallocarray));

    if (sscanf)
        DR_ASSERT(drwrap_wrap(sscanf, pre_sscanf, NULL));

}


void thread_init_event(void *drc)
{
    char *buf = dr_global_alloc(BUF_SIZE);

    drmgr_set_tls_field(drc, tls_idx, buf);
    reset_buf(buf);
}


void thread_exit_event(void *drc)
{
    char *buf = drmgr_get_tls_field(drc, tls_idx);

    if (buf[0] != 0)
        LOGPRINTF("%s <no return ...>\n", buf);
    dr_global_free(buf, BUF_SIZE);
}


void exit_event(void)
{
    drwrap_exit();
    drmgr_exit();
}


DR_EXPORT void dr_client_main(client_id_t __attribute__((unused))id,
                              int argc, const char *argv[])
{
    drmgr_priority_t      p = {
        sizeof(p),
        "Print allocation trace for Villoc",
        NULL,
        NULL,
        0};

    dr_set_client_name("Villoc_dbi", "ampotos@gmail.com");

    if (argc != 2) {
        LOGPRINTF("Usage: %s LOGFILE\n", argv[0]);
        dr_abort();
    }

    logfile = dr_open_file(argv[1], DR_FILE_WRITE_OVERWRITE);

    drwrap_init();
    drmgr_init();

    dr_register_exit_event(&exit_event);
    if (!drmgr_register_module_load_event(&load_event))
        DR_ASSERT_MSG(false, "Can't register module load event handler\n");
    if (!drmgr_register_thread_init_event(&thread_init_event))
        DR_ASSERT_MSG(false, "Can't register thread init event handler\n");
    if (!drmgr_register_thread_exit_event(&thread_exit_event))
        DR_ASSERT_MSG(false, "Can't register thread init exit handler\n");

    // register a slot per thread for logging
    tls_idx = drmgr_register_tls_field();
    DR_ASSERT_MSG(tls_idx != -1, "Can't register tls field\n");
}
