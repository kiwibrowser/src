// Copyright (c) 2016 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <stdio.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/pll_loader/pll_loader.h"


typedef void (*start_func_t)(int argc, char **argv, char **envp,
                             Elf32_auxv_t *auxv);

int main(int argc, char **argv, char **envp) {
  if (argc != 3) {
    fprintf(stderr, "Usage: pll_loader <Directory path> <ELF file>\n");
    return 1;
  }

  ModuleSet modset;
  std::vector<std::string> search_path;
  search_path.push_back(argv[1]);
  modset.SetSonameSearchPath(search_path);

  modset.AddByFilename(argv[2]);
  modset.ResolveRefs();

  Elf32_auxv_t auxv[2];
  auxv[0].a_type = AT_SYSINFO;
  auxv[0].a_un.a_val = (uintptr_t) __nacl_irt_query;
  auxv[1].a_type = AT_NULL;
  auxv[1].a_un.a_val = 0;

  start_func_t start_func =
    (start_func_t) (uintptr_t) modset.GetSym("__libc_start");
  if (start_func == NULL) {
    fprintf(stderr, "Entry point symbol \"__libc_start\" not defined\n");
    return 1;
  }
  start_func(argc, argv, envp, auxv);

  return 0;
}
