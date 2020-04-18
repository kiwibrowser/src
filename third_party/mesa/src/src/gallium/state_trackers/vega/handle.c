/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.  All Rights Reserved.
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


#include "handle.h"
#include "util/u_hash.h"
#include "util/u_hash_table.h"


/**
 * Hash keys are 32-bit VGHandles
 */

struct util_hash_table *handle_hash = NULL;


static unsigned next_handle = 1;


static unsigned
hash_func(void *key)
{
   /* XXX this kind of ugly */
   intptr_t ip = pointer_to_intptr(key);
   return (unsigned) (ip & 0xffffffff);
}


static int
compare(void *key1, void *key2)
{
   if (key1 < key2)
      return -1;
   else if (key1 > key2)
      return +1;
   else
      return 0;
}


void
init_handles(void)
{
   if (!handle_hash)
      handle_hash = util_hash_table_create(hash_func, compare);
}


void
free_handles(void)
{
   /* XXX destroy */
}


VGHandle
create_handle(void *object)
{
   VGHandle h = next_handle++;
   util_hash_table_set(handle_hash, intptr_to_pointer(h), object);
   return h;
}


void
destroy_handle(VGHandle h)
{
   util_hash_table_remove(handle_hash, intptr_to_pointer(h));
}

