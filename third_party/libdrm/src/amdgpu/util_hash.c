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

 /*
  * Authors:
  *   Zack Rusin <zackr@vmware.com>
  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "util_hash.h"

#include <stdlib.h>
#include <assert.h>

#define MAX(a, b) ((a > b) ? (a) : (b))

static const int MinNumBits = 4;

static const unsigned char prime_deltas[] = {
	0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3,  9, 25,  3,
	1, 21,  3, 21,  7, 15,  9,  5,  3, 29, 15,  0,  0,  0,  0,  0
};

static int primeForNumBits(int numBits)
{
	return (1 << numBits) + prime_deltas[numBits];
}

/* Returns the smallest integer n such that
   primeForNumBits(n) >= hint.
*/
static int countBits(int hint)
{
	int numBits = 0;
	int bits = hint;

	while (bits > 1) {
		bits >>= 1;
		numBits++;
	}

	if (numBits >= (int)sizeof(prime_deltas)) {
		numBits = sizeof(prime_deltas) - 1;
	} else if (primeForNumBits(numBits) < hint) {
		++numBits;
	}
	return numBits;
}

struct util_node {
   struct util_node *next;
   unsigned key;
   void *value;
};

struct util_hash_data {
   struct util_node *fakeNext;
   struct util_node **buckets;
   int size;
   int nodeSize;
   short userNumBits;
   short numBits;
   int numBuckets;
};

struct util_hash {
   union {
      struct util_hash_data *d;
      struct util_node      *e;
   } data;
};

static void *util_data_allocate_node(struct util_hash_data *hash)
{
   return malloc(hash->nodeSize);
}

static void util_free_node(struct util_node *node)
{
   free(node);
}

static struct util_node *
util_hash_create_node(struct util_hash *hash,
                      unsigned akey, void *avalue,
                      struct util_node **anextNode)
{
   struct util_node *node = util_data_allocate_node(hash->data.d);

   if (!node)
      return NULL;

   node->key = akey;
   node->value = avalue;

   node->next = (struct util_node*)(*anextNode);
   *anextNode = node;
   ++hash->data.d->size;
   return node;
}

static void util_data_rehash(struct util_hash_data *hash, int hint)
{
   if (hint < 0) {
      hint = countBits(-hint);
      if (hint < MinNumBits)
         hint = MinNumBits;
      hash->userNumBits = (short)hint;
      while (primeForNumBits(hint) < (hash->size >> 1))
         ++hint;
   } else if (hint < MinNumBits) {
      hint = MinNumBits;
   }

   if (hash->numBits != hint) {
      struct util_node *e = (struct util_node *)(hash);
      struct util_node **oldBuckets = hash->buckets;
      int oldNumBuckets = hash->numBuckets;
      int  i = 0;

      hash->numBits = (short)hint;
      hash->numBuckets = primeForNumBits(hint);
      hash->buckets = malloc(sizeof(struct util_node*) * hash->numBuckets);
      for (i = 0; i < hash->numBuckets; ++i)
         hash->buckets[i] = e;

      for (i = 0; i < oldNumBuckets; ++i) {
         struct util_node *firstNode = oldBuckets[i];
         while (firstNode != e) {
            unsigned h = firstNode->key;
            struct util_node *lastNode = firstNode;
            struct util_node *afterLastNode;
            struct util_node **beforeFirstNode;
            
            while (lastNode->next != e && lastNode->next->key == h)
               lastNode = lastNode->next;

            afterLastNode = lastNode->next;
            beforeFirstNode = &hash->buckets[h % hash->numBuckets];
            while (*beforeFirstNode != e)
               beforeFirstNode = &(*beforeFirstNode)->next;
            lastNode->next = *beforeFirstNode;
            *beforeFirstNode = firstNode;
            firstNode = afterLastNode;
         }
      }
      free(oldBuckets);
   }
}

static void util_data_might_grow(struct util_hash_data *hash)
{
   if (hash->size >= hash->numBuckets)
      util_data_rehash(hash, hash->numBits + 1);
}

static void util_data_has_shrunk(struct util_hash_data *hash)
{
   if (hash->size <= (hash->numBuckets >> 3) &&
       hash->numBits > hash->userNumBits) {
      int max = MAX(hash->numBits-2, hash->userNumBits);
      util_data_rehash(hash,  max);
   }
}

static struct util_node *util_data_first_node(struct util_hash_data *hash)
{
   struct util_node *e = (struct util_node *)(hash);
   struct util_node **bucket = hash->buckets;
   int n = hash->numBuckets;
   while (n--) {
      if (*bucket != e)
         return *bucket;
      ++bucket;
   }
   return e;
}

static struct util_node **util_hash_find_node(struct util_hash *hash, unsigned akey)
{
   struct util_node **node;

   if (hash->data.d->numBuckets) {
      node = (struct util_node **)(&hash->data.d->buckets[akey % hash->data.d->numBuckets]);
      assert(*node == hash->data.e || (*node)->next);
      while (*node != hash->data.e && (*node)->key != akey)
         node = &(*node)->next;
   } else {
      node = (struct util_node **)((const struct util_node * const *)(&hash->data.e));
   }
   return node;
}

drm_private struct util_hash_iter
util_hash_insert(struct util_hash *hash, unsigned key, void *data)
{
   util_data_might_grow(hash->data.d);

   {
      struct util_node **nextNode = util_hash_find_node(hash, key);
      struct util_node *node = util_hash_create_node(hash, key, data, nextNode);
      if (!node) {
         struct util_hash_iter null_iter = {hash, 0};
         return null_iter;
      }

      {
         struct util_hash_iter iter = {hash, node};
         return iter;
      }
   }
}

drm_private struct util_hash *util_hash_create(void)
{
   struct util_hash *hash = malloc(sizeof(struct util_hash));
   if (!hash)
      return NULL;

   hash->data.d = malloc(sizeof(struct util_hash_data));
   if (!hash->data.d) {
      free(hash);
      return NULL;
   }

   hash->data.d->fakeNext = 0;
   hash->data.d->buckets = 0;
   hash->data.d->size = 0;
   hash->data.d->nodeSize = sizeof(struct util_node);
   hash->data.d->userNumBits = (short)MinNumBits;
   hash->data.d->numBits = 0;
   hash->data.d->numBuckets = 0;

   return hash;
}

drm_private void util_hash_delete(struct util_hash *hash)
{
   struct util_node *e_for_x = (struct util_node *)(hash->data.d);
   struct util_node **bucket = (struct util_node **)(hash->data.d->buckets);
   int n = hash->data.d->numBuckets;
   while (n--) {
      struct util_node *cur = *bucket++;
      while (cur != e_for_x) {
         struct util_node *next = cur->next;
         util_free_node(cur);
         cur = next;
      }
   }
   free(hash->data.d->buckets);
   free(hash->data.d);
   free(hash);
}

drm_private struct util_hash_iter
util_hash_find(struct util_hash *hash, unsigned key)
{
   struct util_node **nextNode = util_hash_find_node(hash, key);
   struct util_hash_iter iter = {hash, *nextNode};
   return iter;
}

drm_private unsigned util_hash_iter_key(struct util_hash_iter iter)
{
   if (!iter.node || iter.hash->data.e == iter.node)
      return 0;
   return iter.node->key;
}

drm_private void *util_hash_iter_data(struct util_hash_iter iter)
{
   if (!iter.node || iter.hash->data.e == iter.node)
      return 0;
   return iter.node->value;
}

static struct util_node *util_hash_data_next(struct util_node *node)
{
   union {
      struct util_node *next;
      struct util_node *e;
      struct util_hash_data *d;
   } a;
   int start;
   struct util_node **bucket;
   int n;

   a.next = node->next;
   if (!a.next) {
      /* iterating beyond the last element */
      return 0;
   }
   if (a.next->next)
      return a.next;

   start = (node->key % a.d->numBuckets) + 1;
   bucket = a.d->buckets + start;
   n = a.d->numBuckets - start;
   while (n--) {
      if (*bucket != a.e)
         return *bucket;
      ++bucket;
   }
   return a.e;
}

drm_private struct util_hash_iter
util_hash_iter_next(struct util_hash_iter iter)
{
   struct util_hash_iter next = {iter.hash, util_hash_data_next(iter.node)};
   return next;
}

drm_private int util_hash_iter_is_null(struct util_hash_iter iter)
{
   if (!iter.node || iter.node == iter.hash->data.e)
      return 1;
   return 0;
}

drm_private void *util_hash_take(struct util_hash *hash, unsigned akey)
{
   struct util_node **node = util_hash_find_node(hash, akey);
   if (*node != hash->data.e) {
      void *t = (*node)->value;
      struct util_node *next = (*node)->next;
      util_free_node(*node);
      *node = next;
      --hash->data.d->size;
      util_data_has_shrunk(hash->data.d);
      return t;
   }
   return 0;
}

drm_private struct util_hash_iter util_hash_first_node(struct util_hash *hash)
{
   struct util_hash_iter iter = {hash, util_data_first_node(hash->data.d)};
   return iter;
}

drm_private struct util_hash_iter
util_hash_erase(struct util_hash *hash, struct util_hash_iter iter)
{
   struct util_hash_iter ret = iter;
   struct util_node *node = iter.node;
   struct util_node **node_ptr;

   if (node == hash->data.e)
      return iter;

   ret = util_hash_iter_next(ret);
   node_ptr = (struct util_node**)(&hash->data.d->buckets[node->key % hash->data.d->numBuckets]);
   while (*node_ptr != node)
      node_ptr = &(*node_ptr)->next;
   *node_ptr = node->next;
   util_free_node(node);
   --hash->data.d->size;
   return ret;
}
