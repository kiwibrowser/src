/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "intel_resolve_map.h"

#include <assert.h>
#include <stdlib.h>

/**
 * \brief Set that the miptree slice at (level, layer) needs a resolve.
 *
 * If a map element already exists with the given key, then the value is
 * changed to the given value of \c need.
 */
void
intel_resolve_map_set(struct intel_resolve_map *head,
		      uint32_t level,
		      uint32_t layer,
		      enum gen6_hiz_op need)
{
   struct intel_resolve_map **tail = &head->next;
   struct intel_resolve_map *prev = head;

   while (*tail) {
      if ((*tail)->level == level && (*tail)->layer == layer) {
         (*tail)->need = need;
	 return;
      }
      prev = *tail;
      tail = &(*tail)->next;
   }

   *tail = malloc(sizeof(**tail));
   (*tail)->prev = prev;
   (*tail)->next = NULL;
   (*tail)->level = level;
   (*tail)->layer = layer;
   (*tail)->need = need;
}

/**
 * \brief Get an element from the map.
 * \return null if element is not contained in map.
 */
struct intel_resolve_map*
intel_resolve_map_get(struct intel_resolve_map *head,
		      uint32_t level,
		      uint32_t layer)
{
   struct intel_resolve_map *item = head->next;

   while (item) {
      if (item->level == level && item->layer == layer)
	 break;
      else
	 item = item->next;
   }

   return item;
}

/**
 * \brief Remove and free an element from the map.
 */
void
intel_resolve_map_remove(struct intel_resolve_map *elem)
{
   if (elem->prev)
      elem->prev->next = elem->next;
   if (elem->next)
      elem->next->prev = elem->prev;
   free(elem);
}

/**
 * \brief Remove and free all elements of the map.
 */
void
intel_resolve_map_clear(struct intel_resolve_map *head)
{
   struct intel_resolve_map *next = head->next;
   struct intel_resolve_map *trash;

   while (next) {
      trash = next;
      next = next->next;
      free(trash);
   }

   head->next = NULL;
}
