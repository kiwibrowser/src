/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include "util/u_memory.h"

#define DEFAULT_ARRAY_SIZE 256

struct array {
   VGint          datatype_size;
   void          *data;
   VGint          size;
   VGint          num_elements;
};

static INLINE struct array *array_create(VGint datatype_size)
{
   struct array *array = CALLOC_STRUCT(array);
   array->datatype_size = datatype_size;

   array->size = DEFAULT_ARRAY_SIZE;
   array->data = malloc(array->size * array->datatype_size);

   return array;
}


static INLINE struct array *array_create_size(VGint datatype_size, VGint size)
{
   struct array *array = CALLOC_STRUCT(array);
   array->datatype_size = datatype_size;

   array->size = size;
   array->data = malloc(array->size * array->datatype_size);

   return array;
}

static INLINE void array_destroy(struct array *array)
{
   if (array)
      free(array->data);
   FREE(array);
}

static INLINE void array_resize(struct array *array, int num)
{
   VGint size = array->datatype_size * num;
   void *data = malloc(size);
   memcpy(data, array->data, array->size * array->datatype_size);
   free(array->data);
   array->data = data;
   array->size = num;
   array->num_elements = (array->num_elements > num) ? num :
                         array->num_elements;
}

static INLINE void array_append_data(struct array *array,
                              const void *data, int num_elements)
{
   VGbyte *adata;

   while (array->num_elements + num_elements > array->size) {
      array_resize(array, (array->num_elements + num_elements) * 1.5);
   }
   adata = (VGbyte *)array->data;
   memcpy(adata + (array->num_elements * array->datatype_size), data,
          num_elements * array->datatype_size);
   array->num_elements += num_elements;
}

static INLINE void array_change_data(struct array *array,
                              const void *data,
                              int start_idx,
                              int num_elements)
{
   VGbyte *adata = (VGbyte *)array->data;
   memcpy(adata + (start_idx * array->datatype_size), data,
          num_elements * array->datatype_size);
}

static INLINE void array_remove_element(struct array *array,
                                        int idx)
{
   VGbyte *adata = (VGbyte *)array->data;
   memmove(adata + (idx * array->datatype_size),
           adata + ((idx + 1) * array->datatype_size),
           (array->num_elements - idx - 1) * array->datatype_size);
   --array->num_elements;
}

static INLINE void array_reset(struct array *array)
{
   array->num_elements = 0;
}

#endif
