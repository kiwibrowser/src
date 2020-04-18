/**************************************************************************
 *
 * Copyright 2007 VMware, Inc.
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

/**
 * @file
 * Hash implementation.
 * 
 * This file provides a hash implementation that is capable of dealing
 * with collisions. It stores colliding entries in linked list. All
 * functions operating on the hash return an iterator. The iterator
 * itself points to the collision list. If there wasn't any collision
 * the list will have just one entry, otherwise client code should
 * iterate over the entries to find the exact entry among ones that
 * had the same key (e.g. memcmp could be used on the data to check
 * that)
 * 
 * @author Zack Rusin <zackr@vmware.com>
 */

#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>

#include "libdrm_macros.h"

struct util_hash;
struct util_node;

struct util_hash_iter {
	struct util_hash *hash;
	struct util_node *node;
};


drm_private struct util_hash *util_hash_create(void);
drm_private void util_hash_delete(struct util_hash *hash);


/**
 * Adds a data with the given key to the hash. If entry with the given
 * key is already in the hash, this current entry is instered before it
 * in the collision list.
 * Function returns iterator pointing to the inserted item in the hash.
 */
drm_private struct util_hash_iter
util_hash_insert(struct util_hash *hash, unsigned key, void *data);

/**
 * Removes the item pointed to by the current iterator from the hash.
 * Note that the data itself is not erased and if it was a malloc'ed pointer
 * it will have to be freed after calling this function by the callee.
 * Function returns iterator pointing to the item after the removed one in
 * the hash.
 */
drm_private struct util_hash_iter
util_hash_erase(struct util_hash *hash, struct util_hash_iter iter);

drm_private void *util_hash_take(struct util_hash *hash, unsigned key);


drm_private struct util_hash_iter util_hash_first_node(struct util_hash *hash);

/**
 * Return an iterator pointing to the first entry in the collision list.
 */
drm_private struct util_hash_iter
util_hash_find(struct util_hash *hash, unsigned key);


drm_private int util_hash_iter_is_null(struct util_hash_iter iter);
drm_private unsigned util_hash_iter_key(struct util_hash_iter iter);
drm_private void *util_hash_iter_data(struct util_hash_iter iter);


drm_private struct util_hash_iter
util_hash_iter_next(struct util_hash_iter iter);

#endif
