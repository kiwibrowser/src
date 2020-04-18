/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * An implementation of CPU whitelisting for x86's CPUID identification scheme.
 */

#include "native_client/src/include/portability.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/platform_qualify/nacl_cpuwhitelist.h"


static int idcmp(const void *s1, const void *s2) {
  return strncmp((char *)s1, *(char **)s2, kCPUIDStringLength);
}

/* NOTE: The blacklist must be in alphabetical order. */
static const char* const kNaClCpuBlacklist[] = {
/*          1         2        */
/* 12345678901234567890 + '\0' */
  " FakeEntry0000000000",
  " FakeEntry0000000001",
  NACL_BLACKLIST_TEST_ENTRY,
  "zFakeEntry0000000000",
};

/* NOTE: The whitelist must be in alphabetical order. */
static const char* const kNaClCpuWhitelist[] = {
/*          1         2        */
/* 12345678901234567890 + '\0' */
  " FakeEntry0000000000",
  " FakeEntry0000000001",
  " FakeEntry0000000002",
  " FakeEntry0000000003",
  " FakeEntry0000000004",
  " FakeEntry0000000005",
  "GenuineIntel00000f43",
  "zFakeEntry0000000000",
};



static int VerifyCpuList(const char* const cpulist[], int n) {
  int i;
  /* check lengths */
  for (i = 0; i < n; i++) {
    if (strlen(cpulist[i]) != kCPUIDStringLength - 1) {
      return 0;
    }
  }

  /* check order */
  for (i = 1; i < n; i++) {
    if (strncmp(cpulist[i-1], cpulist[i], kCPUIDStringLength) >= 0) {
      return 0;
    }
  }

  return 1;
}


int NaCl_VerifyBlacklist(void) {
  return VerifyCpuList(kNaClCpuBlacklist, NACL_ARRAY_SIZE(kNaClCpuBlacklist));
}


int NaCl_VerifyWhitelist(void) {
  return VerifyCpuList(kNaClCpuWhitelist, NACL_ARRAY_SIZE(kNaClCpuWhitelist));
}


static int IsCpuInList(const char *myid, const char* const cpulist[], int n) {
  return (NULL != bsearch(myid, cpulist, n, sizeof(char*), idcmp));
}


/* for testing */
int NaCl_CPUIsWhitelisted(const char *myid) {
  return IsCpuInList(myid,
                     kNaClCpuWhitelist,
                     NACL_ARRAY_SIZE(kNaClCpuWhitelist));
}

/* for testing */
int NaCl_CPUIsBlacklisted(const char *myid) {
  return IsCpuInList(myid,
                     kNaClCpuBlacklist,
                     NACL_ARRAY_SIZE(kNaClCpuBlacklist));
}


int NaCl_ThisCPUIsWhitelisted(void) {
  NaClCPUData data;
  const char* myid;
  NaClCPUDataGet(&data);
  myid = GetCPUIDString(&data);
  return IsCpuInList(myid,
                     kNaClCpuWhitelist,
                     NACL_ARRAY_SIZE(kNaClCpuWhitelist));
}

int NaCl_ThisCPUIsBlacklisted(void) {
  NaClCPUData data;
  const char* myid;
  NaClCPUDataGet(&data);
  myid = GetCPUIDString(&data);
  return IsCpuInList(myid,
                     kNaClCpuBlacklist,
                     NACL_ARRAY_SIZE(kNaClCpuBlacklist));
}
