/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/service_runtime/env_cleanser.h"
#include "native_client/src/trusted/service_runtime/env_cleanser_test.h"

static char const *const kBogusEnvs[] = {
  "FOOBAR",
  "QUUX",
  "USER=bsy",
  "HOME=/home/bsy",
  "PATH=/home/bsy/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin",
  "LD_LIBRARY_PATH=.:/usr/bsy/lib",
  NULL,
};

static char const *const kValidEnvs[] = {
  "LANG=en_us.UTF-8",
  "LC_MEASUREMENT=en_US.UTF-8",
  "LC_PAPER=en_US.UTF-8@legal",
  "LC_TIME=%a, %B %d, %Y",
  "NACLVERBOSITY",
  NULL,
};

static char const *const kMurkyEnv[] = {
  "FOOBAR",
  "LC_TIME=%a, %B %d, %Y",
  "QUUX",
  "USER=bsy",
  "LC_PAPER=en_US.UTF-8@legal",
  "HOME=/home/bsy",
  "PATH=/home/bsy/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin",
  "LANG=en_us.UTF-8",
  "LC_MEASUREMENT=en_US.UTF-8",
  "LD_LIBRARY_PATH=.:/usr/bsy/lib",
  "NACLENV_LD_PRELOAD=libvalgrind.so",
  "NACLENV_SHELL=/bin/sh",
  NULL,
};

static char const *const kFilteredEnv[] = {
  "LANG=en_us.UTF-8",
  "LC_MEASUREMENT=en_US.UTF-8",
  "LC_PAPER=en_US.UTF-8@legal",
  "LC_TIME=%a, %B %d, %Y",
  "LD_PRELOAD=libvalgrind.so",
  "SHELL=/bin/sh",
  NULL,
};

static char const *const kFilteredEnvWithoutWhitelist[] = {
  "LD_PRELOAD=libvalgrind.so",
  "SHELL=/bin/sh",
  NULL,
};

static char const *const kFilteredEnvWithPassthrough[] = {
  "FOOBAR",
  "LC_TIME=%a, %B %d, %Y",
  "QUUX",
  "USER=bsy",
  "LC_PAPER=en_US.UTF-8@legal",
  "HOME=/home/bsy",
  "PATH=/home/bsy/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin",
  "LANG=en_us.UTF-8",
  "LC_MEASUREMENT=en_US.UTF-8",
  "LD_LIBRARY_PATH=.:/usr/bsy/lib",
  "LD_PRELOAD=libvalgrind.so",
  "SHELL=/bin/sh",
  NULL,
};


static char const *const kOverrideEnv[] = {
  "LD_LIBRARY_PATH=nacl-sdk/lib",
  NULL,
};

static char const *const kFilteredEnvWithPassthroughAndOverride[] = {
  "FOOBAR",
  "LC_TIME=%a, %B %d, %Y",
  "QUUX",
  "USER=bsy",
  "LC_PAPER=en_US.UTF-8@legal",
  "HOME=/home/bsy",
  "PATH=/home/bsy/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin",
  "LANG=en_us.UTF-8",
  "LC_MEASUREMENT=en_US.UTF-8",
  "LD_LIBRARY_PATH=nacl-sdk/lib",
  "LD_PRELOAD=libvalgrind.so",
  "SHELL=/bin/sh",
  NULL,
};


int StrInStrTbl(char const *str, char const *const *tbl) {
  int i;

  for (i = 0; NULL != tbl[i]; ++i) {
    if (!strcmp(str, tbl[i])) {
      return 1;
    }
  }
  return 0;
}

int StrTblsHaveSameEntries(char const *const *tbl1, char const *const *tbl2) {
  int i;
  int num_left;
  int num_right;

  if (NULL == tbl1) {
    num_left = 0;
  } else {
    for (num_left = 0; NULL != tbl1[num_left]; ++num_left) {
    }
  }
  if (NULL == tbl2) {
    num_right = 0;
  } else {
    for (num_right = 0; NULL != tbl2[num_right]; ++num_right) {
    }
  }
  if (num_left != num_right) {
    return 0;
  }
  if (0 == num_left) {
    return 1;
  }
  for (i = 0; NULL != tbl1[i]; ++i) {
    if (!StrInStrTbl(tbl1[i], tbl2)) {
      return 0;
    }
  }
  for (i = 0; NULL != tbl2[i]; ++i) {
    if (!StrInStrTbl(tbl2[i], tbl1)) {
      return 0;
    }
  }
  return 1;
}

void PrintStrTbl(char const *name, char const *const *tbl) {
  printf("\n%s\n", name);
  printf("--------\n");
  for (; NULL != *tbl; ++tbl) {
    printf("%s\n", *tbl);
  }
  printf("--------\n");
}

int main(void) {
  int errors = 0;
  int i;
  struct NaClEnvCleanser nec;

  printf("Environment Cleanser Test\n\n");
  printf("\nWhitelist self-check\n\n");
  for (i = 0; NULL != kNaClEnvWhitelist[i]; ++i) {
    printf("Checking %s\n", kNaClEnvWhitelist[i]);
    if (0 == NaClEnvInWhitelist(kNaClEnvWhitelist[i])) {
      ++errors;
      printf("ERROR\n");
    } else {
      printf("OK\n");
    }
  }

  printf("\nNon-whitelisted entries\n\n");

  for (i = 0; NULL != kBogusEnvs[i]; ++i) {
    printf("Checking %s\n", kBogusEnvs[i]);
    if (0 != NaClEnvInWhitelist(kBogusEnvs[i])) {
      ++errors;
      printf("ERROR\n");
    } else {
      printf("OK\n");
    }
  }

  printf("\nValid environment entries\n\n");

  for (i = 0; NULL != kValidEnvs[i]; ++i) {
    printf("Checking %s\n", kValidEnvs[i]);
    if (0 == NaClEnvInWhitelist(kValidEnvs[i])) {
      ++errors;
      printf("ERROR\n");
    } else {
      printf("OK\n");
    }
  }

  printf("\nEnvironment Filtering\n");
  NaClEnvCleanserCtor(&nec, 1, 0);
  if (!NaClEnvCleanserInit(&nec, kMurkyEnv, NULL)) {
    printf("FAILED: NaClEnvCleanser Init failed\n");
    ++errors;
  } else {
    if (!StrTblsHaveSameEntries(NaClEnvCleanserEnvironment(&nec),
                                kFilteredEnv)) {
      printf("ERROR: filtered env wrong\n");
      ++errors;

      PrintStrTbl("Original environment", kMurkyEnv);
      PrintStrTbl("Filtered environment", NaClEnvCleanserEnvironment(&nec));
      PrintStrTbl("Expected environment", kFilteredEnv);
    } else {
      printf("OK\n");
    }
  }
  NaClEnvCleanserDtor(&nec);

  printf("\nEnvironment Filtering (without whitelist)\n");
  NaClEnvCleanserCtor(&nec, 0, 0);
  if (!NaClEnvCleanserInit(&nec, kMurkyEnv, NULL)) {
    printf("FAILED: NaClEnvCleanser Init failed\n");
    ++errors;
  } else {
    if (!StrTblsHaveSameEntries(NaClEnvCleanserEnvironment(&nec),
                                kFilteredEnvWithoutWhitelist)) {
      printf("ERROR: filtered env wrong\n");
      ++errors;

      PrintStrTbl("Original environment", kMurkyEnv);
      PrintStrTbl("Filtered environment", NaClEnvCleanserEnvironment(&nec));
      PrintStrTbl("Expected environment", kFilteredEnvWithoutWhitelist);
    } else {
      printf("OK\n");
    }
  }
  NaClEnvCleanserDtor(&nec);

  printf("\nEnvironment Filtering (with passthrough)\n");
  NaClEnvCleanserCtor(&nec, 0, 1);
  if (!NaClEnvCleanserInit(&nec, kMurkyEnv, NULL)) {
    printf("FAILED: NaClEnvCleanser Init failed\n");
    ++errors;
  } else {
    if (!StrTblsHaveSameEntries(NaClEnvCleanserEnvironment(&nec),
                                kFilteredEnvWithPassthrough)) {
      printf("ERROR: filtered env wrong\n");
      ++errors;

      PrintStrTbl("Original environment", kMurkyEnv);
      PrintStrTbl("Filtered environment", NaClEnvCleanserEnvironment(&nec));
      PrintStrTbl("Expected environment", kFilteredEnvWithPassthrough);
    } else {
      printf("OK\n");
    }
  }
  NaClEnvCleanserDtor(&nec);

  printf("\nEnvironment Filtering (with passthrough and override)\n");
  NaClEnvCleanserCtor(&nec, 0, 1);
  if (!NaClEnvCleanserInit(&nec, kMurkyEnv, kOverrideEnv)) {
    printf("FAILED: NaClEnvCleanser Init failed\n");
    ++errors;
  } else {
    if (!StrTblsHaveSameEntries(NaClEnvCleanserEnvironment(&nec),
                                kFilteredEnvWithPassthroughAndOverride)) {
      printf("ERROR: filtered env wrong\n");
      ++errors;

      PrintStrTbl("Original environment", kMurkyEnv);
      PrintStrTbl("Filtered environment", NaClEnvCleanserEnvironment(&nec));
      PrintStrTbl("Expected environment",
                  kFilteredEnvWithPassthroughAndOverride);
    } else {
      printf("OK\n");
    }
  }
  NaClEnvCleanserDtor(&nec);

  printf("%s\n", (0 == errors) ? "PASSED" : "FAILED");
  return 0 != errors;
}
