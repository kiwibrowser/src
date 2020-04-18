/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "mapi/mapi.h"

/* define vega_spec and vega_procs for use with mapi */
#define API_TMP_DEFINE_SPEC
#include "api.h"

static void api_init(void)
{
   static boolean initialized = FALSE;
   if (!initialized) {
      mapi_init(vega_spec);
      initialized = TRUE;
   }
}

struct mapi_table *api_create_dispatch(void)
{
   struct mapi_table *tbl;

   api_init();

   tbl = mapi_table_create();
   if (tbl)
      mapi_table_fill(tbl, vega_procs);

   return tbl;
}

void api_destroy_dispatch(struct mapi_table *tbl)
{
   mapi_table_destroy(tbl);
}

void api_make_dispatch_current(const struct mapi_table *tbl)
{
   mapi_table_make_current(tbl);
}

mapi_proc api_get_proc_address(const char *proc_name)
{
   if (!proc_name || proc_name[0] != 'v' || proc_name[1] != 'g')
      return NULL;
   proc_name += 2;

   api_init();
   return mapi_get_proc_address(proc_name);
}
