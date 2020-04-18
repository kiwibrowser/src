
#ifndef INLINE_DEBUG_HELPER_H
#define INLINE_DEBUG_HELPER_H

#include "pipe/p_compiler.h"
#include "util/u_debug.h"


/* Helper function to wrap a screen with
 * one or more debug driver: rbug, trace.
 */

#ifdef DEBUG

#ifdef GALLIUM_TRACE
#include "trace/tr_public.h"
#endif

#ifdef GALLIUM_RBUG
#include "rbug/rbug_public.h"
#endif

#ifdef GALLIUM_GALAHAD
#include "galahad/glhd_public.h"
#endif

#ifdef GALLIUM_NOOP
#include "noop/noop_public.h"
#endif

#endif /* DEBUG */

static INLINE struct pipe_screen *
debug_screen_wrap(struct pipe_screen *screen)
{
#ifdef DEBUG

#if defined(GALLIUM_RBUG)
   screen = rbug_screen_create(screen);
#endif

#if defined(GALLIUM_TRACE)
   screen = trace_screen_create(screen);
#endif

#if defined(GALLIUM_GALAHAD)
   screen = galahad_screen_create(screen);
#endif

#if defined(GALLIUM_NOOP)
   screen = noop_screen_create(screen);
#endif

#endif /* DEBUG */

   return screen;
}

#endif
