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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * For an overview of the HiZ operations, see the following sections of the
 * Sandy Bridge PRM, Volume 1, Part2:
 *   - 7.5.3.1 Depth Buffer Clear
 *   - 7.5.3.2 Depth Buffer Resolve
 *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
 *
 * Of these, two get entered in the resolve map as needing to be done to the
 * buffer: depth resolve and hiz resolve.
 */
enum gen6_hiz_op {
   GEN6_HIZ_OP_DEPTH_CLEAR,
   GEN6_HIZ_OP_DEPTH_RESOLVE,
   GEN6_HIZ_OP_HIZ_RESOLVE,
   GEN6_HIZ_OP_NONE,
};

/**
 * \brief Map of miptree slices to needed resolves.
 *
 * The map is implemented as a linear doubly-linked list.
 *
 * In the intel_resolve_map*() functions, the \c head argument is not
 * inspected for its data. It only serves as an anchor for the list.
 *
 * \par Design Discussion
 *
 *     There are two possible ways to record which miptree slices need
 *     resolves. 1) Maintain a flag for every miptree slice in the texture,
 *     likely in intel_mipmap_level::slice, or 2) maintain a list of only
 *     those slices that need a resolve.
 *
 *     Immediately before drawing, a full depth resolve performed on each
 *     enabled depth texture. If design 1 were chosen, then at each draw call
 *     it would be necessary to iterate over each miptree slice of each
 *     enabled depth texture in order to query if each slice needed a resolve.
 *     In the worst case, this would require 2^16 iterations: 16 texture
 *     units, 16 miplevels, and 256 depth layers (assuming maximums for OpenGL
 *     2.1).
 *
 *     By choosing design 2, the number of iterations is exactly the minimum
 *     necessary.
 */
struct intel_resolve_map {
   uint32_t level;
   uint32_t layer;
   enum gen6_hiz_op need;

   struct intel_resolve_map *next;
   struct intel_resolve_map *prev;
};

void
intel_resolve_map_set(struct intel_resolve_map *head,
		      uint32_t level,
		      uint32_t layer,
		      enum gen6_hiz_op need);

struct intel_resolve_map*
intel_resolve_map_get(struct intel_resolve_map *head,
		      uint32_t level,
		      uint32_t layer);

void
intel_resolve_map_remove(struct intel_resolve_map *elem);

void
intel_resolve_map_clear(struct intel_resolve_map *head);

#ifdef __cplusplus
} /* extern "C" */
#endif

