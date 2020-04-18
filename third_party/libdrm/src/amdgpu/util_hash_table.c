/**************************************************************************
 *
 * Copyright 2008 VMware, Inc.
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
 * General purpose hash table implementation.
 * 
 * Just uses the util_hash for now, but it might be better switch to a linear
 * probing hash table implementation at some point -- as it is said they have 
 * better lookup and cache performance and it appears to be possible to write 
 * a lock-free implementation of such hash tables . 
 * 
 * @author José Fonseca <jfonseca@vmware.com>
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "util_hash_table.h"
#include "util_hash.h"

#include <stdlib.h>
#include <assert.h>

struct util_hash_table
{
	struct util_hash *head;

	/** Hash function */
	unsigned (*make_hash)(void *key);

	/** Compare two keys */
	int (*compare)(void *key1, void *key2);
};

struct util_hash_table_item
{
	void *key;
	void *value;
};


static struct util_hash_table_item *
util_hash_table_item(struct util_hash_iter iter)
{
	return (struct util_hash_table_item *)util_hash_iter_data(iter);
}

drm_private struct util_hash_table *
util_hash_table_create(unsigned (*hash)(void *key),
		       int (*compare)(void *key1, void *key2))
{
	struct util_hash_table *ht;

	ht = malloc(sizeof(struct util_hash_table));
	if(!ht)
		return NULL;

	ht->head = util_hash_create();
	if(!ht->head) {
		free(ht);
		return NULL;
	}

	ht->make_hash = hash;
	ht->compare = compare;

	return ht;
}

static struct util_hash_iter
util_hash_table_find_iter(struct util_hash_table *ht,
			  void *key, unsigned key_hash)
{
	struct util_hash_iter iter;
	struct util_hash_table_item *item;

	iter = util_hash_find(ht->head, key_hash);
	while (!util_hash_iter_is_null(iter)) {
		item = (struct util_hash_table_item *)util_hash_iter_data(iter);
		if (!ht->compare(item->key, key))
			break;
		iter = util_hash_iter_next(iter);
	}

	return iter;
}

static struct util_hash_table_item *
util_hash_table_find_item(struct util_hash_table *ht,
                          void *key, unsigned key_hash)
{
	struct util_hash_iter iter;
	struct util_hash_table_item *item;

	iter = util_hash_find(ht->head, key_hash);
	while (!util_hash_iter_is_null(iter)) {
		item = (struct util_hash_table_item *)util_hash_iter_data(iter);
		if (!ht->compare(item->key, key))
			return item;
		iter = util_hash_iter_next(iter);
	}

	return NULL;
}

drm_private void
util_hash_table_set(struct util_hash_table *ht, void *key, void *value)
{
	unsigned key_hash;
	struct util_hash_table_item *item;
	struct util_hash_iter iter;

	assert(ht);
	if (!ht)
		return;

	key_hash = ht->make_hash(key);

	item = util_hash_table_find_item(ht, key, key_hash);
	if(item) {
		/* TODO: key/value destruction? */
		item->value = value;
		return;
	}

	item = malloc(sizeof(struct util_hash_table_item));
	if(!item)
		return;

	item->key = key;
	item->value = value;

	iter = util_hash_insert(ht->head, key_hash, item);
	if(util_hash_iter_is_null(iter)) {
		free(item);
		return;
	}
}

drm_private void *util_hash_table_get(struct util_hash_table *ht, void *key)
{
	unsigned key_hash;
	struct util_hash_table_item *item;

	assert(ht);
	if (!ht)
		return NULL;

	key_hash = ht->make_hash(key);

	item = util_hash_table_find_item(ht, key, key_hash);
	if(!item)
		return NULL;

	return item->value;
}

drm_private void util_hash_table_remove(struct util_hash_table *ht, void *key)
{
	unsigned key_hash;
	struct util_hash_iter iter;
	struct util_hash_table_item *item;

	assert(ht);
	if (!ht)
		return;

	key_hash = ht->make_hash(key);

	iter = util_hash_table_find_iter(ht, key, key_hash);
	if(util_hash_iter_is_null(iter))
		return;

	item = util_hash_table_item(iter);
	assert(item);
	free(item);

	util_hash_erase(ht->head, iter);
}

drm_private void util_hash_table_clear(struct util_hash_table *ht)
{
	struct util_hash_iter iter;
	struct util_hash_table_item *item;

	assert(ht);
	if (!ht)
		return;

	iter = util_hash_first_node(ht->head);
	while (!util_hash_iter_is_null(iter)) {
		item = (struct util_hash_table_item *)util_hash_take(ht->head, util_hash_iter_key(iter));
		free(item);
		iter = util_hash_first_node(ht->head);
	}
}

drm_private void util_hash_table_foreach(struct util_hash_table *ht,
			void (*callback)(void *key, void *value, void *data),
			void *data)
{
	struct util_hash_iter iter;
	struct util_hash_table_item *item;

	assert(ht);
	if (!ht)
		return;

	iter = util_hash_first_node(ht->head);
	while (!util_hash_iter_is_null(iter)) {
		item = (struct util_hash_table_item *)util_hash_iter_data(iter);
		callback(item->key, item->value, data);
		iter = util_hash_iter_next(iter);
	}
}

drm_private void util_hash_table_destroy(struct util_hash_table *ht)
{
	struct util_hash_iter iter;
	struct util_hash_table_item *item;

	assert(ht);
	if (!ht)
		return;

	iter = util_hash_first_node(ht->head);
	while (!util_hash_iter_is_null(iter)) {
		item = (struct util_hash_table_item *)util_hash_iter_data(iter);
		free(item);
		iter = util_hash_iter_next(iter);
	}

	util_hash_delete(ht->head);
	free(ht);
}
