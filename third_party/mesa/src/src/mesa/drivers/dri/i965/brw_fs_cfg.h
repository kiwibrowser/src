/*
 * Copyright © 2012 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_fs.h"

class fs_bblock_link : public exec_node {
public:
   fs_bblock_link(fs_bblock *block)
      : block(block)
   {
   }

   fs_bblock *block;
};

class fs_bblock {
public:
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   fs_bblock_link *make_list(void *mem_ctx);

   fs_bblock();

   void add_successor(void *mem_ctx, fs_bblock *successor);

   fs_inst *start;
   fs_inst *end;

   int start_ip;
   int end_ip;

   exec_list parents;
   exec_list children;
   int block_num;
};

class fs_cfg {
public:
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   fs_cfg(fs_visitor *v);
   ~fs_cfg();
   fs_bblock *new_block();
   void set_next_block(fs_bblock *block);
   void make_block_array();

   /** @{
    *
    * Used while generating the block list.
    */
   fs_bblock *cur;
   int ip;
   /** @} */

   void *mem_ctx;

   /** Ordered list (by ip) of basic blocks */
   exec_list block_list;
   fs_bblock **blocks;
   int num_blocks;
};
