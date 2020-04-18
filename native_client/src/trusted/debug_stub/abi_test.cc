/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/debug_stub/abi.h"

using gdb_rsp::Abi;

int VerifyAbi(const char *name, uint32_t regs) {
  int errs = 0;

  const Abi *abi = Abi::Find(name);
  if (NULL != abi) {
    uint32_t regCnt = abi->GetRegisterCount();
    uint32_t byteCnt = abi->GetContextSize();
    uint32_t bytes = 0;
    uint32_t loop = 0;

    if (strcmp(abi->GetName(), name)) {
      printf("Incorrect name for ABI %s.\n", name);
      errs++;
    }

    if (regCnt != regs) {
      printf("Incorrect number of registers for ABI %s.\n", name);
      errs++;
    }

    if (abi->GetRegisterDef(regs) != NULL) {
      printf("Unexpected register for ABI %s.\n", name);
      errs++;
    }

    for (loop = 0; loop < regs; loop++) {
      const Abi::RegDef *def = abi->GetRegisterDef(loop);
      if (NULL == def) {
        printf("Missing register for ABI %s, reg %d.\n", name, loop);
        errs++;
        break;
      }

      if (NULL == def->name_) {
        printf("Missing register name for ABI %s, reg %d.\n", name, loop);
        errs++;
        break;
      }

      if (loop != def->index_) {
        printf("Index mismatch for ABI %s, reg %d.\n", name, loop);
        errs++;
        break;
      }

      if ((1 > def->bytes_) || (def->bytes_ > byteCnt)) {
        printf("Bad register size for ABI %s, reg %d.\n", name, loop);
        errs++;
        break;
      }

      if ((def->type_ < Abi::GENERAL) || (def->type_ >= Abi::REG_TYPE_CNT)) {
        printf("Illegal register type ABI %s, reg %d.\n", name, loop);
        errs++;
        break;
      }

      if (def->offset_ != bytes) {
        printf("Offset mismatch in ABI %s, reg %d.\n", name, loop);
        errs++;
        break;
      }

      bytes += def->bytes_;
    }

    if (bytes != byteCnt) {
      printf("Context size mismatch for ABI %s.\n", name);
      errs++;
    }

    if (abi->GetInstPtrDef() == NULL) {
      printf("Missing instruction pointer for ABI %s.\n", name);
      errs++;
    }
  } else {
    printf("Could not find ABI %s.\n", name);
    errs++;
  }
  return errs;
}

int TestAbi() {
  int errs = 0;

  // TODO(cbiffle) Figure out how to REALLY detect ARM
  errs += VerifyAbi("iwmmxt", 17);
  errs += VerifyAbi("i386", 16);
  errs += VerifyAbi("i386:x86-64", 24);
  errs += VerifyAbi("mips", 33);

  // Get the default ABI
  const Abi *abi = Abi::Get();
  if (NULL == abi) {
    printf("Failed to get default ABI.\n");
    errs++;
  }

  if (NULL != Abi::Find("non-existant")) {
    printf("Found 'non-existant' ABI.\n");
    errs++;
  }

  return errs;
}

