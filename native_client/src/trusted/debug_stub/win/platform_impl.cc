/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <exception>

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/abi.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

/*
 * Define the OS specific portions of IPlatform interface.
 */

 /*
  * Find files mappings and replaces them with memory which can be made
  * writable.  Works only with code regions where debugger need to set
  * breakpoints.
  */

static bool UnmapFiles(struct NaClApp *nap, void *ptr, uint32_t len) {
  DWORD old_flags;
  uintptr_t max_step;
  uintptr_t user_ptr = NaClSysToUser(nap, reinterpret_cast<uintptr_t>(ptr));
  if (user_ptr + len <= user_ptr || user_ptr + len > nap->dynamic_text_end) {
    return false;
  }
  uintptr_t user_ptr_end = user_ptr + len;
  uintptr_t start_page = user_ptr >> NACL_PAGESHIFT;
  uintptr_t end_page = ((user_ptr_end - 1) >> NACL_PAGESHIFT) + 1;
  uintptr_t page_len = end_page - start_page;
  uintptr_t current_page = start_page;
  char buf[0x10000];
  while (page_len > 0) {
    const NaClVmmapEntry *entry =
        NaClVmmapFindPage(&nap->mem_map, current_page);
    if (entry == NULL) {
      current_page++;
      page_len--;
      continue;
    }
    max_step = entry->npages - (current_page - entry->page_num);
    if (max_step > page_len) {
      max_step = page_len;
    }
    if (entry->flags != 0) {
      for (uintptr_t i = 0; i < max_step; i++) {
        void *addr = reinterpret_cast<void *>(
            NaClUserToSys(nap, (current_page + i) << NACL_PAGESHIFT));
        size_t size = 0x10000;
        nacl_off64_t file_size = entry->file_size -
            ((current_page + i - entry->page_num) << NACL_PAGESHIFT);
        if (static_cast<nacl_off64_t>(size) > file_size) {
          size = static_cast<size_t>(file_size);
        }
        // fill buffer with hlt.
        memset(buf, 0xf4, 0x10000);
        memcpy(buf, addr, size);
        if (!UnmapViewOfFile(addr)) {
          return false;
        }
        if (NULL == VirtualAlloc(addr, 0x10000,
                                 MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
          NaClLog(LOG_FATAL,
                  "UnmapFiles: VirtualAlloc failed with %d\n",
                  GetLastError());
        }
        memcpy(addr, buf, 0x10000);
        if (!VirtualProtect(addr, 0x10000,
                            PAGE_EXECUTE_READ, &old_flags)) {
          NaClLog(LOG_FATAL,
                  "UnmapFiles: VirtualProtect failed with %d\n",
                  GetLastError());
        }
      }
    }
    current_page += max_step;
    page_len -= max_step;
  }
  return true;
}

static bool CheckReadRights(void *ptr, uint32_t len) {
  MEMORY_BASIC_INFORMATION memory_info;
  SIZE_T offset;
  SIZE_T max_step;
  char *current_pointer = reinterpret_cast<char *>(ptr);
  SIZE_T current_len = len;
  // Check that all memory is accessible.
  while (current_len > 0) {
    if (!VirtualQuery(current_pointer, &memory_info, sizeof(memory_info))) {
      NaClLog(LOG_ERROR, "Reprotect: VirtualQuery failed with %d\n",
              GetLastError());
      return false;
    }
    if (memory_info.Protect == PAGE_NOACCESS ||
        memory_info.Protect == 0) {
      return false;
    }
    offset = current_pointer - reinterpret_cast<char*>(memory_info.BaseAddress);
    max_step = memory_info.RegionSize - offset;
    if (current_len <= max_step) {
      break;
    }
    current_len -= max_step;
    current_pointer = current_pointer + max_step;
  }
  return true;
}

static DWORD Reprotect(void *ptr, uint32_t len, DWORD newflags) {
  DWORD oldflags;

  if (!CheckReadRights(ptr, len)) {
    return (DWORD) -1;
  }

  if (!VirtualProtect(ptr, len, newflags, &oldflags)) {
    NaClLog(LOG_ERROR, "Reprotect: VirtualProtect failed with %d\n",
            GetLastError());
    return (DWORD) -1;
  }

  FlushInstructionCache(GetCurrentProcess(), ptr, len);
  return oldflags;
}

namespace port {

bool IPlatform::GetMemory(uint64_t virt, uint32_t len, void *dst) {
  if (!CheckReadRights(reinterpret_cast<void*>(virt), len)) return false;

  memcpy(dst, reinterpret_cast<void *>(virt), len);
  return true;
}

bool IPlatform::SetMemory(struct NaClApp *nap, uint64_t virt, uint32_t len,
                          void *src) {
  UNREFERENCED_PARAMETER(nap);
  uint32_t oldFlags = Reprotect(reinterpret_cast<void *>(virt),
                                len, PAGE_READWRITE);

  if (oldFlags == (DWORD) -1) {
    oldFlags = Reprotect(reinterpret_cast<void *>(virt), len,
                         PAGE_WRITECOPY);
    if (oldFlags == (DWORD) -1) {
      // Windows XP doesn't support PAGE_EXECUTE_WRITECOPY so we fallback to
      // unmapping files and mapping normal memory instead.
      if (UnmapFiles(nap, reinterpret_cast<void *>(virt), len)) {
        oldFlags = Reprotect(reinterpret_cast<void *>(virt), len,
                             PAGE_WRITECOPY);
      }
    }
  }

  if (oldFlags == (DWORD) -1) return false;

  memcpy(reinterpret_cast<void*>(virt), src, len);
  FlushInstructionCache(GetCurrentProcess(),
                        reinterpret_cast<void*>(virt), len);
  (void) Reprotect(reinterpret_cast<void*>(virt), len, oldFlags);
  return true;
}

}  // namespace port
