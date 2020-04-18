/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/trusted/interval_multiset/nacl_interval_list.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_list_intern.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_range_tree.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_range_tree_intern.h"

struct NaClIntervalMultiset *NaClIntervalMultisetFactory(char const *kind) {
  struct NaClIntervalMultiset *widget = NULL;

#define MAKE(tag) do {                                             \
    if (0 == strcmp(#tag, kind)) {                                 \
      widget = (struct NaClIntervalMultiset *) malloc(             \
          sizeof(struct tag));                                     \
      if (NULL != widget && !tag ## Ctor((struct tag *) widget)) { \
        free(widget);                                              \
        widget = NULL;                                             \
      }                                                            \
      return widget;                                               \
    }                                                              \
  } while (0)

  MAKE(NaClIntervalListMultiset);
  MAKE(NaClIntervalRangeTree);

  return NULL;
}
