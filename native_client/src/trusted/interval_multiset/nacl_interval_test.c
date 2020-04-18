/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_list.h"

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"

#define MAX_INTERVAL_TREE_KINDS     8
#define MAX_INPUT_LINE_LENGTH       4096
#define DEFAULT_COUNT_WHEN_RANDOM   8192

#define OP_ADD      0
#define OP_REMOVE   1
#define OP_PROBE    2
/*
 * Input/Output format:
 *
 * If a line contains #, it and all characters following to the
 * newline are discarded.  Lines containing only space and tabs are
 * ignored.  (Manually generated test inputs can have comments.)
 * Maximum line length is MAX_INPUT_LINE_LENGTH.
 *
 * All other lines should be four integers of the form:
 *
 *   op first_val last_val expected_output
 *
 * "op" specifies which virtual function to invoke and is interpreted
 * as per the OP_* defines above.
 *
 * For Add and Remove, "expected_output" is don't-care (ignored). For
 * OverlapsWith, "expected_output" is the expected return value (since
 * it's C-style bool, encoded as 0 or 1).  When random operations are
 * generated, we encode "unknown" or "don't care" as -1 in
 * "expected_output" for use OverlapsWith, since we don't otherwise
 * compute separately what the expected output should be.
 *
 * For the output case, e.g., generated with randomly chosen calls and
 * parameters, "expected_output" is the actual return value from
 * OverlapsWith.  An initial comment line contains the random seed
 * used.
 */

uint32_t g_max_random_region_size = 8 << 20;

struct Op {
  int opcode;
  uint32_t first_val;
  uint32_t last_val;
  int expected_output;
};

void StripComment(char *line) {
  char *comment;
  if (NULL != (comment = strchr(line, '#'))) {
    *comment = '\0';
  }
}

int LineContainsOnlyWhitespace(char *line) {
  while ('\0' != *line) {
    switch (*line) {
      case ' ':
      case '\t':
      case '\n':
        ++line;
        continue;
      default:
        return 0;
    }
  }
  return 1;
}

int ReadOp(struct Op *op, FILE *iob) {
  char line[MAX_INPUT_LINE_LENGTH];
  int  line_num = 0;

  for (;;) {
    ++line_num;
    if (!fgets(line, sizeof line, iob)) {
      return 0;
    }
    StripComment(line);
    if (LineContainsOnlyWhitespace(line)) {
      continue;
    }
    /*
     * we cheat -- NACL_SCNu32 macros are not defined, and we know
     * that the printf format specifier won't work for scanf since on
     * windows we use the I32 notation for windows printf, but windows
     * scanf doesn't understand that notation.
     */
    if (4 == sscanf(line, "%d%u%u%d",
                    &op->opcode, &op->first_val, &op->last_val,
                    &op->expected_output)) {
      return 1;
    }
    fprintf(stderr, "op syntax error in line %d, ignoring\n", line_num);
    exit(1);
  }
}

void WriteOp(FILE *iob, struct Op *op) {
  fprintf(iob, "%d %"NACL_PRIu32" %"NACL_PRIu32" %d\n",
          op->opcode, op->first_val, op->last_val, op->expected_output);
}

struct Op *extant_intervals = NULL;
size_t num_extant_intervals = 0;
size_t size_extant_intervals = 0;

void AddInterval(struct Op const *op) {
  size_t target_num;

  if (num_extant_intervals == size_extant_intervals) {
    target_num = 2 * size_extant_intervals;
    if (target_num == 0) {
      target_num = 32;
    }
    extant_intervals = (struct Op *) realloc(
        extant_intervals,
        target_num * sizeof *extant_intervals);
    if (NULL == extant_intervals) {
      NaClLog(LOG_FATAL,
              "nacl_interval_test: no memory for extant intervals\n");
    }
    size_extant_intervals = target_num;
  }
  NaClLog(2, ("adding at %"NACL_PRIuS" [%u,%u], size %"NACL_PRIuS"\n"),
          num_extant_intervals,
          op->first_val, op->last_val,
          size_extant_intervals);
  extant_intervals[num_extant_intervals++] = *op;
}

void RemoveIntervalAtIndex(size_t ix) {
  NaClLog(2, "removing ix %"NACL_PRIuS", contents [%u,%u]\n",
          ix, extant_intervals[ix].first_val, extant_intervals[ix].last_val);
  extant_intervals[ix] = extant_intervals[--num_extant_intervals];
}

void GenerateRandomOp(struct Op *op) {
  switch (rand() % 5) {
    case 0:
      op->opcode = OP_ADD;
      break;
    case 1:
      op->opcode = OP_REMOVE;
      break;
    case 2:
    case 3:
    case 4:
      op->opcode = OP_PROBE;
      break;
  }
  if (op->opcode == OP_REMOVE && num_extant_intervals == 0) {
    op->opcode = OP_ADD;
  }
  if (op->opcode != OP_REMOVE) {
    op->expected_output = -1;
    do {
      op->first_val = rand();
      op->last_val = op->first_val + (rand() % g_max_random_region_size);
    } while (op->first_val > op->last_val);
  }
  if (op->opcode == OP_ADD) {
    AddInterval(op);
  } else if (op->opcode == OP_REMOVE) {
    size_t ix = rand() % num_extant_intervals;
    op->first_val = extant_intervals[ix].first_val;
    op->last_val = extant_intervals[ix].last_val;
    op->expected_output = -1;
    RemoveIntervalAtIndex(ix);
  }
  NaClLog(3, "Gen (%d, %u, %u, %d)\n",
          op->opcode, op->first_val, op->last_val, op->expected_output);
}

int main(int ac, char **av) {
  int count = -1;  /* until EOF if a file is specified */
  unsigned seed = (unsigned) time((time_t *) NULL);
  FILE *out_file = NULL;
  FILE *in_file = NULL;
  char const *default_kinds[] = {
    "NaClIntervalListMultiset",
    "NaClIntervalRangeTree",
  };
  char const *kind[MAX_INTERVAL_TREE_KINDS];
  size_t num_kinds = 0;
  int opt;
  size_t ix;
  int iter;
  struct Op oper;
  size_t error_count = 0;
  struct NaClIntervalMultiset *nis[MAX_INTERVAL_TREE_KINDS];
  int result;

  NaClPlatformInit();

  CHECK(NACL_ARRAY_SIZE(default_kinds) <= NACL_ARRAY_SIZE(kind));

  while (-1 != (opt = getopt(ac, av, "c:i:k:m:o:s:v"))) {
    switch (opt) {
      case 'c':
        count = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'i':
        in_file = fopen(optarg, "r");
        if (NULL == in_file) {
          perror("nacl_interval_test");
          fprintf(stderr, "Could not open \"%s\" as input file\n", optarg);
          return 1;
        }
        break;
      case 'k':
        if (num_kinds == MAX_INTERVAL_TREE_KINDS) {
          fprintf(stderr, "too many interval tree kinds specified\n");
          return 2;
        }
        kind[num_kinds++] = optarg;
        break;
      case 'm':
        g_max_random_region_size = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'o':
        out_file = fopen(optarg, "w");
        if (NULL == out_file) {
          perror("nacl_interval_test");
          fprintf(stderr, "Could not open \"%s\" as output file\n", optarg);
          return 3;
        }
        break;
      case 's':
        seed = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'v':
        NaClLogIncrVerbosity();
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_interval_test [-s seed] [-c count]\n"
                "         [-i test_input_file] [-o test_output_file]\n");
        return 4;
    }
  }

  if (0 == num_kinds) {
    for (ix = 0; ix < NACL_ARRAY_SIZE(default_kinds); ++ix) {
      kind[ix] = default_kinds[ix];
    }
    num_kinds = NACL_ARRAY_SIZE(default_kinds);
  }

  /*
   * When there are bugs that corrupt recursive data structures and
   * printing them involve recursive routines that do not flush
   * buffers (even if line buffered, do not always output a newline),
   * it is a good idea to unbuffer output so that the output just
   * before a crash is visible.  This is a general debugging strategy
   * -- this test code served both as a debugging aid when bringing up
   * new data structures and as a unit / regression test, and while
   * its role as a unit / regression test probably doesn't require
   * unbuffered output, we leave this in.
   */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  srand(seed);
  if (NULL == in_file && -1 == count) {
    count = DEFAULT_COUNT_WHEN_RANDOM;
  }
  if (NULL != out_file) {
    fprintf(out_file, "# seed %u\n", seed);
  }
  /* always print to stdout so test framework can capture and log it */
  if (NULL == in_file) {
    printf("# seed %u\n", seed);
  } else {
    printf("# input file used, not randomly generated test cases.\n");
  }
  fflush(stdout);
  /* ensure seed is always visible, even if setvbuf above is later deleted */

  for (ix = 0; ix < num_kinds; ++ix) {
    nis[ix] = NaClIntervalMultisetFactory(kind[ix]);
    if (NULL == nis[ix]) {
      fprintf(stderr,
              "NaClIntervalMultisetFactory(%s) yielded NULL\n", kind[ix]);
      return 5;
    }
  }

  for (iter = 0; (-1 == count) || (iter < count); ++iter) {
    if (NULL == in_file) {
      GenerateRandomOp(&oper);
    } else {
      if (!ReadOp(&oper, in_file)) {
        break;
      }
    }
    NaClLog(3, "Processing (%d, %u, %u, %d)\n",
          oper.opcode, oper.first_val, oper.last_val, oper.expected_output);
    for (ix = 0; ix < num_kinds; ++ix) {
      switch (oper.opcode) {
        case OP_ADD:
          (*nis[ix]->vtbl->AddInterval)(nis[ix],
                                        oper.first_val,
                                        oper.last_val);
          break;
        case OP_REMOVE:
          (*nis[ix]->vtbl->RemoveInterval)(nis[ix],
                                           oper.first_val,
                                           oper.last_val);
          break;
        case OP_PROBE:
          result = (*nis[ix]->vtbl->OverlapsWith)(nis[ix],
                                                  oper.first_val,
                                                  oper.last_val);
          if (oper.expected_output != -1 &&
              oper.expected_output != result) {
            printf("OverlapsWith(%d,%d)=%d, expected %d\n",
                   oper.first_val, oper.last_val, result, oper.expected_output);
            printf("FAIL\n");
            if (0 == ++error_count) ++error_count;
          }
          oper.expected_output = result;
          break;
        default:
          WriteOp(stderr, &oper);
          NaClLog(LOG_FATAL, "nacl_interval_test: internal error\n");
      }
    }
    if (NULL != out_file) {
      WriteOp(out_file, &oper);
    }
  }

  printf("%"NACL_PRIuS" errors\n", error_count);

  for (ix = 0; ix < num_kinds; ++ix) {
    NaClIntervalMultisetDelete(nis[ix]);
    nis[ix] = NULL;
  }

  NaClPlatformFini();
  return error_count != 0;
}
