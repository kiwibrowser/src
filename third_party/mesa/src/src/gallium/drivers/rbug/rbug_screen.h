/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef RBUG_SCREEN_H
#define RBUG_SCREEN_H

#include "pipe/p_screen.h"
#include "pipe/p_defines.h"

#include "os/os_thread.h"

struct rbug_list {
   struct rbug_list *next;
   struct rbug_list *prev;
};


struct rbug_screen
{
   struct pipe_screen base;

   struct pipe_screen *screen;
   struct pipe_context *private_context;

   /* remote debugger */
   struct rbug_rbug *rbug;

   pipe_mutex list_mutex;
   int num_contexts;
   int num_resources;
   int num_surfaces;
   int num_transfers;
   struct rbug_list contexts;
   struct rbug_list resources;
   struct rbug_list surfaces;
   struct rbug_list transfers;
};

static INLINE struct rbug_screen *
rbug_screen(struct pipe_screen *screen)
{
   return (struct rbug_screen *)screen;
}

#define rbug_screen_add_to_list(scr, name, obj) \
   do {                                          \
      pipe_mutex_lock(scr->list_mutex);          \
      insert_at_head(&scr->name, &obj->list);    \
      scr->num_##name++;                         \
      pipe_mutex_unlock(scr->list_mutex);        \
   } while (0)

#define rbug_screen_remove_from_list(scr, name, obj) \
   do {                                               \
      pipe_mutex_lock(scr->list_mutex);               \
      remove_from_list(&obj->list);                   \
      scr->num_##name--;                              \
      pipe_mutex_unlock(scr->list_mutex);             \
   } while (0)



/**********************************************************
 * rbug_core.c
 */

struct rbug_rbug;

struct rbug_rbug *
rbug_start(struct rbug_screen *rb_screen);

void
rbug_stop(struct rbug_rbug *rbug);


#endif /* RBUG_SCREEN_H */
