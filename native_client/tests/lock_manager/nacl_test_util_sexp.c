/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/tests/lock_manager/nacl_test_util_sexp.h"

#include "native_client/src/shared/platform/nacl_check.h"

/*
 * A basic lisp parser.  The execution part is separate (see
 * nacl_file_lock-test.c) since the execution model wires in special
 * forms that are non-standard (event matchers).
 */

static int g_NaClSexpVerbosity = 0;

void NaClSexpSetVerbosity(int v) {
  g_NaClSexpVerbosity = v;
}

void NaClSexpIncVerbosity(void) {
  ++g_NaClSexpVerbosity;
}

static void NaClSexpIoUngetc(struct NaClSexpIo *p, int ch) {
  if (p->num_unget == 2) {
    fprintf(stderr, "NaClSexpIoUngetc: internal error: too many ugetc\n");
    abort();
  }
  p->ungetbuf[p->num_unget++] = ch;
}

static int NaClSexpIoReadCharSkipComments(struct NaClSexpIo *p) {
  int ch;

  if (p->num_unget > 0) {
    return p->ungetbuf[--p->num_unget];
  }
  ch = getc(p->iob);
  if (EOF == ch || ';' != ch) {
    return ch;
  }
  while ((EOF != (ch = getc(p->iob))) && '\n' != ch) {
    continue; /* skip */
  }
  return ch;
}

void NaClSexpIoCtor(struct NaClSexpIo *self, FILE *iob,
                    void (*err_func)(struct NaClSexpIo *self, char const *r)) {
  self->line_num = 1;
  self->iob = iob;
  self->num_unget = 0;
  self->on_error = err_func;
}

void NaClSexpIoSetIob(struct NaClSexpIo *self, FILE *iob) {
  self->iob = iob;
}

static int NaClSexpIoReadCharSkipSpaces(struct NaClSexpIo *p) {
  int ch;
  while (EOF != (ch = NaClSexpIoReadCharSkipComments(p)) && isspace(ch)) {
    if ('\n' == ch) {
      ++p->line_num;
    }
  }
  return ch;
}

static char *ReadToken(struct NaClSexpIo *p) {
  size_t space = 10;
  char *str = malloc(space);
  size_t used = 0;
  int ch;

  CHECK(NULL != str);
  while (EOF != (ch = NaClSexpIoReadCharSkipComments(p)) &&
         !isspace(ch) && ')' != ch) {
    if (used == space) {
      char *new_str = realloc(str, 2 * space);
      if (NULL == new_str) {
        (*p->on_error)(p, "out of memory reading string token");
      }
      space *= 2;
      str = new_str;
    }
    str[used++] = ch;
  }
  if (EOF != ch) {
    NaClSexpIoUngetc(p, ch);
  }
  if (used == space) {
    char *new_str = realloc(str, space+1);
    if (NULL == new_str) {
      (*p->on_error)(p,
                     "out-of-memory reading string token (null termination)");
    }
    str = new_str;
    ++space;
  }
  str[used++] = '\0';
  return str;
}

static int ReadInteger(struct NaClSexpIo *p) {
  int base = -1;  /* unknown */
  int ch;
  int val = 0;
  int negative = 0;

  while (EOF != (ch = NaClSexpIoReadCharSkipComments(p))) {
    if (-1 == base && '-' == ch) {
      negative = 1;
      base = 0;
    } else if (base <= 0 && '0' == ch) {
      base = 8;
    } else if (base <= 0 && isdigit(ch)) {
      base = 10;
      val = ch - '0';
    } else if (8 == base && 'x' == ch) {
      base = 16;
    } else if (16 == base && isxdigit(ch)) {
      val *= 16;
      if (isalpha(ch) && isupper(ch)) {
        ch = tolower(ch);
      }
      if (isalpha(ch)) {
        val += ch - 'a' + 10;
      } else {
        val += ch - '0';
      }
    } else if (10 == base && isdigit(ch)) {
      val *= 10;
      val += ch - '0';
    } else if (8 == base && isdigit(ch) && ch < '8') {
      val *= 8;
      val += ch - '0';
    } else {
      NaClSexpIoUngetc(p, ch);
      break;
    }
  }
  return negative ? -val : val;
}

void NaClSexpPrintConsIntern(FILE *iob, struct NaClSexpCons *c);

void NaClSexpPrintCons(FILE *iob, struct NaClSexpCons *c) {
  putc('(', iob);
  NaClSexpPrintConsIntern(iob, c);
}

void NaClSexpPrintNode(FILE *iob, struct NaClSexpNode *n) {
  if (NULL == n) {
    fprintf(iob, "nil");
    return;
  }
  switch (n->type) {
    case kNaClSexpCons:
      putc('(', iob);
      if (g_NaClSexpVerbosity) {
        fprintf(iob, "[cons]");
      }
      NaClSexpPrintConsIntern(iob, n->u.cval);
      break;
    case kNaClSexpInteger:
      if (g_NaClSexpVerbosity) {
        fprintf(iob, "[int]");
      }
      fprintf(iob, "%"NACL_PRId64, n->u.ival);
      break;
    case kNaClSexpToken:
      if (g_NaClSexpVerbosity) {
        fprintf(iob, "[token]");
      }
      fprintf(iob, "%s", n->u.tval);
      break;
  }
}

void NaClSexpPrintConsIntern(FILE *iob, struct NaClSexpCons *c) {
  if (NULL == c) {
    putc(')', iob);
    return;
  }
  NaClSexpPrintNode(iob, c->car);
  if (NULL == c->cdr) {
    putc(')', iob);
  } else {
    putc(' ', iob);
    NaClSexpPrintConsIntern(iob, c->cdr);
  }
}

struct NaClSexpCons *NaClSexpReadList(struct NaClSexpIo *p) {
  struct NaClSexpCons *c;
  int ch = NaClSexpIoReadCharSkipSpaces(p);
  if (EOF == ch) {
    (*p->on_error)(p, "Premature end of list");
  }
  if (')' == ch) {
    return NULL;
  }
  c = malloc(sizeof *c);
  CHECK(NULL != c);
  NaClSexpIoUngetc(p, ch);
  c->car = NaClSexpReadSexp(p);
  c->cdr = NaClSexpReadList(p);
  return c;
}

struct NaClSexpNode *NaClSexpReadSexp(struct NaClSexpIo *p) {
  int ch = NaClSexpIoReadCharSkipSpaces(p);
  struct NaClSexpNode *n;
  int is_int;

  if (EOF == ch) {
    return NULL;
  }
  n = malloc(sizeof *n);
  CHECK(NULL != n);
  if ('(' == ch) {
    n->type = kNaClSexpCons;
    n->u.cval = NaClSexpReadList(p);
    return n;
  }
  is_int = isdigit(ch);
  if ('-' == ch) {
    int next_ch = NaClSexpIoReadCharSkipComments(p);
    if (isdigit(next_ch)) {
      is_int = 1;
    } else {
      /* is token! */
    }
    NaClSexpIoUngetc(p, next_ch);
  }
  if (is_int) {
    NaClSexpIoUngetc(p, ch);
    n->type = kNaClSexpInteger;
    n->u.ival = ReadInteger(p);
    return n;
  }
  NaClSexpIoUngetc(p, ch);
  n->type = kNaClSexpToken;
  n->u.tval = ReadToken(p);
  if (strlen(n->u.tval) == 0) {
    printf("bad parse\n");
    free(n);
    return NULL;
  }
  return n;
}

static char *NaClSexpDupToken(char const *t) {
  char *nt = strdup(t);
  return nt;
}

struct NaClSexpNode *NaClSexpDupNode(struct NaClSexpNode *n) {
  struct NaClSexpNode *nn;
  if (NULL == n) {
    return NULL;
  }
  nn = malloc(sizeof *nn);
  CHECK(NULL != nn);
  nn->type = n->type;
  switch (nn->type) {
    case kNaClSexpCons:
      nn->u.cval = NaClSexpDupCons(n->u.cval);
      break;
    case kNaClSexpInteger:
      nn->u.ival = n->u.ival;
      break;
    case kNaClSexpToken:
      nn->u.tval = NaClSexpDupToken(n->u.tval);
      break;
  }
  return nn;
}

struct NaClSexpCons *NaClSexpDupCons(struct NaClSexpCons *c) {
  struct NaClSexpCons *nc;

  if (NULL == c) {
    return NULL;
  }
  nc = malloc(sizeof *nc);
  CHECK(NULL != nc);
  nc->car = NaClSexpDupNode(c->car);
  nc->cdr = NaClSexpDupCons(c->cdr);
  return nc;
}

struct NaClSexpCons *NaClSexpConsCons(struct NaClSexpNode *n,
                                      struct NaClSexpCons *c) {
  struct NaClSexpCons *cell;

  cell = malloc(sizeof *cell);
  CHECK(NULL != cell);
  cell->car = n;
  cell->cdr = c;
  return cell;
}

static struct NaClSexpCons *NaClSexpAppendIntern(struct NaClSexpCons *f,
                                                 struct NaClSexpCons *rest) {
  if (f == NULL) {
    return rest;
  }
  rest = NaClSexpAppendIntern(f->cdr, rest);

  return NaClSexpConsCons(NaClSexpDupNode(f->car), rest);
}

struct NaClSexpCons *NaClSexpAppend(struct NaClSexpCons *first,
                                    struct NaClSexpCons *second) {
  struct NaClSexpCons *n = NaClSexpDupCons(second);

  return NaClSexpAppendIntern(first, n);
}

void NaClSexpFreeNode(struct NaClSexpNode *n);

void NaClSexpFreeCons(struct NaClSexpCons *c) {
  if (NULL == c) {
    return;
  }
  NaClSexpFreeNode(c->car);
  NaClSexpFreeCons(c->cdr);
}

void NaClSexpFreeNode(struct NaClSexpNode *n) {
  if (NULL == n) {
    return;
  }
  switch (n->type) {
    case kNaClSexpCons:
      NaClSexpFreeCons(n->u.cval);
      break;
    case kNaClSexpInteger:
      break;
    case kNaClSexpToken:
      free(n->u.tval);
      break;
  }
}

int NaClSexpConsp(struct NaClSexpNode *n) {
  return NULL != n && kNaClSexpCons == n->type;
}

int NaClSexpIntp(struct NaClSexpNode *n) {
  return NULL != n && kNaClSexpInteger == n->type;
}

int NaClSexpTokenp(struct NaClSexpNode *n) {
  return NULL != n && kNaClSexpToken == n->type;
}

struct NaClSexpCons *NaClSexpNodeToCons(struct NaClSexpNode *n) {
  return n->u.cval;
}

int64_t NaClSexpNodeToInt(struct NaClSexpNode const *n) {
  return n->u.ival;
}

char const *NaClSexpNodeToToken(struct NaClSexpNode const *n) {
  return n->u.tval;
}

struct NaClSexpNode *NaClSexpNodeWrapCons(struct NaClSexpCons *c) {
  struct NaClSexpNode *n = malloc(sizeof *n);

  CHECK(NULL != n);
  n->type = kNaClSexpCons;
  n->u.cval = c;
  return n;
}

struct NaClSexpNode *NaClSexpNodeWrapInt(int64_t num) {
  struct NaClSexpNode *n = malloc(sizeof *n);

  CHECK(NULL != n);
  n->type = kNaClSexpInteger;
  n->u.ival = num;
  return n;
}

struct NaClSexpNode *NaClSexpNodeWrapToken(char *t) {
  struct NaClSexpNode *n = malloc(sizeof *n);

  CHECK(NULL != n);
  n->type = kNaClSexpToken;
  n->u.tval = t;
  return n;
}

struct NaClSexpCons *NaClSexpConsWrapNode(struct NaClSexpNode *n) {
  struct NaClSexpCons *c = malloc(sizeof *c);

  CHECK(NULL != c);
  c->car = n;
  c->cdr = NULL;
  return c;
}

size_t NaClSexpListLength(struct NaClSexpCons const *c) {
  size_t len;

  for (len = 0; c != NULL; c = c->cdr)
    ++len;
  return len;
}
