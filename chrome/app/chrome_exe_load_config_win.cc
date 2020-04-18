// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

// The whole object of this file is to define |_load_config_used| which
// is a weakly-linked symbol that the linker knows about. The crt defines
// it as well it but this one will be used if present.
//
// The _load_config_used appears in the exe as a special member of the PE
// image format: the load config directory. This structure controls
// global parameters which are hard or impossible to set otherwise. This
// MSDN page https://goo.gl/zsGeVY documents some of the members.
//
// When making a changes to this stucture the only practical way to make
// sure they are honored is to run dumpbin.exe <exe-file> /loadconfig
// and inspect the results.

#if defined(_WIN64)
typedef struct {
  DWORD      Size;
  DWORD      TimeDateStamp;
  WORD       MajorVersion;
  WORD       MinorVersion;
  DWORD      GlobalFlagsClear;
  DWORD      GlobalFlagsSet;
  DWORD      CriticalSectionDefaultTimeout;
  ULONGLONG  DeCommitFreeBlockThreshold;
  ULONGLONG  DeCommitTotalFreeThreshold;
  ULONGLONG  LockPrefixTable;
  ULONGLONG  MaximumAllocationSize;
  ULONGLONG  VirtualMemoryThreshold;
  ULONGLONG  ProcessAffinityMask;
  DWORD      ProcessHeapFlags;
  WORD       CSDVersion;
  WORD       Reserved1;
  ULONGLONG  EditList;
  ULONGLONG  SecurityCookie;
  ULONGLONG  SEHandlerTable;
  ULONGLONG  SEHandlerCount;
} IMAGE_LOAD_CONFIG_DIRECTORY64_2;
typedef IMAGE_LOAD_CONFIG_DIRECTORY64_2 IMAGE_LOAD_CONFIG_DIRECTORY_2;
#else
typedef struct {
  DWORD       Size;
  DWORD       TimeDateStamp;
  WORD        MajorVersion;
  WORD        MinorVersion;
  DWORD       GlobalFlagsClear;
  DWORD       GlobalFlagsSet;
  DWORD       CriticalSectionDefaultTimeout;
  DWORD       DeCommitFreeBlockThreshold;
  DWORD       DeCommitTotalFreeThreshold;
  DWORD       LockPrefixTable;
  DWORD       MaximumAllocationSize;
  DWORD       VirtualMemoryThreshold;
  DWORD       ProcessHeapFlags;
  DWORD       ProcessAffinityMask;
  WORD        CSDVersion;
  WORD        Reserved1;
  DWORD       EditList;
  PUINT_PTR   SecurityCookie;
  PVOID       *SEHandlerTable;
  DWORD       SEHandlerCount;
} IMAGE_LOAD_CONFIG_DIRECTORY32_2;
typedef IMAGE_LOAD_CONFIG_DIRECTORY32_2 IMAGE_LOAD_CONFIG_DIRECTORY_2;
#endif

// What follows, except as indicated is verbatim (including casts) from
// <visual studio path>\VC\crt\src\intel\loadcfg.c. The default values
// are all 0 except for the last 3 members. See
// https://msdn.microsoft.com/en-us/library/9a89h429.aspx for an
// explanation of these members.
extern "C" UINT_PTR __security_cookie;
extern "C" PVOID __safe_se_handler_table[];
extern "C" BYTE __safe_se_handler_count;

// DeCommitTotalFreeThreshold: increase the decomit free threshold for
// the process heap, which is documented as "The size of the minimum
// total memory that must be freed in the process heap before it is
// freed (de-committed) in bytes".
//
// Set at 2 MiB what this does is prevent a lot of VirtualFree plus the
// following VirtualAlloc because the default value makes the heap too
// conservative on the ammount of memory it keeps for future allocations.

extern "C"
const IMAGE_LOAD_CONFIG_DIRECTORY_2  _load_config_used = {
  sizeof(IMAGE_LOAD_CONFIG_DIRECTORY_2),
  0,  // TimeDateStamp.
  0,  // MajorVersion.
  0,  // MinorVersion.
  0,  // GlobalFlagsClear.
  0,  // GlobalFlagsSet.
  0,  // CriticalSectionDefaultTimeout.
  0,  // DeCommitFreeBlockThreshold.
  2 * 1024 * 1024,  // DeCommitTotalFreeThreshold.
  0,  // LockPrefixTable.
  0,  // MaximumAllocationSize.
  0,  // VirtualMemoryThreshold.
  0,  // ProcessHeapFlags.
  0,  // ProcessAffinityMask.
  0,  // CSDVersion.
  0,  // Reserved1.
  0 , // EditList.
#ifdef _WIN64
  (ULONGLONG)&__security_cookie,
#else
  &__security_cookie,
  __safe_se_handler_table,
  (DWORD)(DWORD_PTR)&__safe_se_handler_count
#endif
};
