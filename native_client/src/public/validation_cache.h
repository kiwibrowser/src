/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_VALIDATION_CACHE_H_
#define NATIVE_CLIENT_SRC_PUBLIC_VALIDATION_CACHE_H_

#include <stddef.h>

struct NaClValidationMetadata;

/*
 * This interface allows the validator to query a database of validation results
 * while hiding details of how the database is implemented.
 *
 * query = cache->CreateQuery(cache->handle);
 * CreateQuery: create an opaque query object, given an opaque context object.
 * The context object contains persistent variables that will be used for all
 * queries, whereas the query object contains information relevant to a single
 * validation result.
 *
 * cache->AddData(query, data, data_size);
 * AddData: add a blob of binary data to the query.  Conceptually, the query
 * will concatenate all the binary data it is given, in the order it is given,
 * and use the concatenated blob as a key to look up validation results in a
 * database.  In practice, all of the data is hashed into a reasonabally sized
 * key.  The validation cache doesn't care what data it is given, it is the
 * responsibility of the validator to provide enough information to uniquely
 * identify the validation result.  This gives flexibility to use different
 * types of keys for different validators and different sources of code.
 *
 * validation_passed = cache->QueryKnownToValidate(query);
 * QueryKnownToValidate: the key is complete, query the validation status.
 * This should only be called once.  AddData must not be called after calling
 * this function.
 *
 * cache->SetKnownToValidate(query);
 * SetKnownToValidate: set the database entry for the given key.
 * QueryKnownToValidate must be called first.
 *
 * cache->DestroyQuery(query);
 * DestroyQuery: cleanup and deallocate the query object.
 */

struct NaClValidationCache {
  void *handle;
  void *(*CreateQuery)(void *handle);
  void (*AddData)(void *query, const unsigned char *data, size_t length);
  int (*QueryKnownToValidate)(void *query);
  void (*SetKnownToValidate)(void *query);
  void (*DestroyQuery)(void *query);
  int (*CachingIsInexpensive)(const struct NaClValidationMetadata *metadata);
};

#endif
