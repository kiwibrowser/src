/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_LOCK_MANAGER_NACL_TEST_UTIL_SEXP_H_
#define NATIVE_CLIENT_TESTS_LOCK_MANAGER_NACL_TEST_UTIL_SEXP_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

void NaClSexpSetVerbosity(int v);
void NaClSexpIncVerbosity(void);

enum NaClSexpNodeType {
  kNaClSexpCons = 0,
  kNaClSexpInteger = 1,
  kNaClSexpToken = 2,
};

struct NaClSexpCons;

struct NaClSexpNode {
  enum NaClSexpNodeType type;
  union {
    struct NaClSexpCons *cval;
    int64_t ival;
    char *tval;
  } u;
};

struct NaClSexpCons {
  struct NaClSexpNode *car;
  struct NaClSexpCons *cdr;  /* no dotted pairs */
};

struct NaClSexpIo {
  int line_num;
  FILE *iob;
  int ungetbuf[2];
  int num_unget;
  void (*on_error)(struct NaClSexpIo *p, char const *reason);
};

/* does NOT take ownership of iob */
void NaClSexpIoCtor(struct NaClSexpIo *self, FILE *iob,
                    void (*err_func)(struct NaClSexpIo *self, char const *r));

/* does NOT take ownership of iob */
void NaClSexpIoSetIob(struct NaClSexpIo *self, FILE *iob);

void NaClSexpPrintCons(FILE *iob, struct NaClSexpCons *c);

void NaClSexpPrintNode(FILE *iob, struct NaClSexpNode *n);

struct NaClSexpNode *NaClSexpReadSexp(struct NaClSexpIo *p);

struct NaClSexpCons *NaClSexpReadList(struct NaClSexpIo *p);

struct NaClSexpNode *NaClSexpDupNode(struct NaClSexpNode *n);

struct NaClSexpCons *NaClSexpDupCons(struct NaClSexpCons *n);

struct NaClSexpCons *NaClSexpConsCons(struct NaClSexpNode *n,
                                      struct NaClSexpCons *c);

struct NaClSexpCons *NaClSexpAppend(struct NaClSexpCons *first,
                                    struct NaClSexpCons *second);

void NaClSexpFreeNode(struct NaClSexpNode *n);

void NaClSexpFreeCons(struct NaClSexpCons *c);

int NaClSexpConsp(struct NaClSexpNode *n);

int NaClSexpIntp(struct NaClSexpNode *n);

int NaClSexpTokenp(struct NaClSexpNode *n);

struct NaClSexpCons* NaClSexpNodeToCons(struct NaClSexpNode *n);

int64_t NaClSexpNodeToInt(struct NaClSexpNode const *n);

char const *NaClSexpNodeToToken(struct NaClSexpNode const *n);

struct NaClSexpNode *NaClSexpNodeWrapCons(struct NaClSexpCons *c);

struct NaClSexpNode *NaClSexpNodeWrapInt(int64_t n);

struct NaClSexpNode *NaClSexpNodeWrapToken(char *t);

struct NaClSexpCons *NaClSexpConsWrapNode(struct NaClSexpNode *n);

size_t NaClSexpListLength(struct NaClSexpCons const *c);

EXTERN_C_END

#endif
