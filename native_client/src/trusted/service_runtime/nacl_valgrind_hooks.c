/* Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"

void NaClFileMappingForValgrind(size_t vma, size_t size, size_t file_offset) {
  UNREFERENCED_PARAMETER(vma);
  UNREFERENCED_PARAMETER(size);
  UNREFERENCED_PARAMETER(file_offset);
}

void NaClFileNameForValgrind(const char* name) {
  UNREFERENCED_PARAMETER(name);
}

void NaClSandboxMemoryStartForValgrind(uintptr_t mem_start) {
  UNREFERENCED_PARAMETER(mem_start);
}
