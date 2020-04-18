#include "main/glheader.h"
#include "main/compiler.h"
#include "glapi/glapi.h"

/* This is just supposed to make sure we get a reference to
   the driver entry symbol that the compiler doesn't optimize away */

extern char __driDriverExtensions[];

/* provide glapi symbols */

#if defined(GLX_USE_TLS)

PUBLIC __thread struct _glapi_table * _glapi_tls_Dispatch
    __attribute__((tls_model("initial-exec")));

PUBLIC __thread void * _glapi_tls_Context
    __attribute__((tls_model("initial-exec")));

PUBLIC const struct _glapi_table *_glapi_Dispatch;
PUBLIC const void *_glapi_Context;

#else

PUBLIC struct _glapi_table *_glapi_Dispatch;
PUBLIC void *_glapi_Context;

#endif

PUBLIC void
_glapi_check_multithread(void)
{}

PUBLIC void
_glapi_set_context(void *context)
{}

PUBLIC void *
_glapi_get_context(void)
{
	return 0;
}

PUBLIC void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{}

PUBLIC struct _glapi_table *
_glapi_get_dispatch(void)
{
	return 0;
}

PUBLIC int
_glapi_add_dispatch( const char * const * function_names,
		     const char * parameter_signature )
{
	return 0;
}

PUBLIC GLint
_glapi_get_proc_offset(const char *funcName)
{
	return 0;
}

PUBLIC _glapi_proc
_glapi_get_proc_address(const char *funcName)
{
	return 0;
}

PUBLIC GLuint
_glapi_get_dispatch_table_size(void)
{
	return 0;
}

PUBLIC unsigned long
_glthread_GetID(void)
{
   return 0;
}

int main(int argc, char** argv)
{
   void* p = __driDriverExtensions;
   return (int)(unsigned long)p;
}
