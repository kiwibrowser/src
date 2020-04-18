/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_list.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_list_intern.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"

struct NaClIntervalNode {
  struct NaClIntervalNode *next;
  uint32_t range_left;
  uint32_t range_right;
};

struct NaClIntervalMultisetVtbl const kNaClIntervalListMultisetVtbl;  /* fwd */

int NaClIntervalListMultisetCtor(struct NaClIntervalListMultiset *self) {
  self->intervals = NULL;
  self->base.vtbl = &kNaClIntervalListMultisetVtbl;
  return 1;
}

static void NaClIntervalListMultisetDtor(struct NaClIntervalMultiset *vself) {
  struct NaClIntervalListMultiset *self = (struct NaClIntervalListMultiset *)
      vself;
  struct NaClIntervalNode *interval;
  struct NaClIntervalNode *next_interval;

  for (interval = self->intervals; NULL != interval; interval = next_interval) {
    next_interval = interval->next;
    free(interval);
  }
  self->intervals = NULL;

  /* no base class dtor */
  self->base.vtbl = NULL;
}

static void NaClIntervalListMultisetAddInterval(
    struct NaClIntervalMultiset *vself,
    uint32_t first_val,
    uint32_t last_val) {
  struct NaClIntervalListMultiset *self = (struct NaClIntervalListMultiset *)
      vself;
  struct NaClIntervalNode *interval;

  interval = malloc(sizeof *interval);
  if (NULL == interval) {
    NaClLog(LOG_FATAL, "No memory in NaClIntervalListSetAdd\n");
  }
  interval->range_left = first_val;
  interval->range_right = last_val;
  interval->next = self->intervals;
  self->intervals = interval;
}

static void NaClIntervalListMultisetRemoveInterval(
    struct NaClIntervalMultiset *vself,
    uint32_t first_val,
    uint32_t last_val) {
  struct NaClIntervalListMultiset *self = (struct NaClIntervalListMultiset *)
      vself;
  struct NaClIntervalNode *p;
  struct NaClIntervalNode **pp;

  for (pp = &self->intervals; NULL != *pp; pp = &p->next) {
    p = *pp;
    if (p->range_left == first_val && last_val == p->range_right) {
      *pp = p->next;
      free(p);
      return;
    }
  }
  NaClLog(LOG_FATAL, "NaClIntervalListMultisetRemove: [%u,%u] not found\n",
          first_val, last_val);
}

static int NaClIntervalListMultisetOverlapsWith(
    struct NaClIntervalMultiset *vself,
    uint32_t first_val,
    uint32_t last_val) {
  struct NaClIntervalListMultiset *self = (struct NaClIntervalListMultiset *)
      vself;
  struct NaClIntervalNode *p;

  for (p = self->intervals; NULL != p; p = p->next) {
    if (p->range_left <= last_val && first_val <= p->range_right) {
      return 1;
    }
  }
  return 0;
}

struct NaClIntervalMultisetVtbl const kNaClIntervalListMultisetVtbl = {
  NaClIntervalListMultisetDtor,
  NaClIntervalListMultisetAddInterval,
  NaClIntervalListMultisetRemoveInterval,
  NaClIntervalListMultisetOverlapsWith,
};
