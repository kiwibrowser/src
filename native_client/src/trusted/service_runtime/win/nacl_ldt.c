/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl local descriptor table manipulation support.
 */

#include "native_client/src/include/portability.h"
#include <windows.h>
#include <errno.h>
#include <stdio.h>
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
/* for LDT_ENTRIES */
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"

/*
 * A static helper for finding unused LDT entry.
 */
static int NaClFindUnusedEntryNumber(void);

/* struct LdtEntry is a structure that is laid out exactly as the segment
 * descriptors described in the Intel reference manuals.  This is needed
 * because Mac and Windows use this representation in the methods used to
 * set and get entries in the local descriptor table (LDT), but use different
 * names for the members.  Linux uses a different representation to set, but
 * also reads entries back in this format.  It needs to be laid out packed,
 * bitwise, little endian.
 */
struct LdtEntry {
  unsigned int limit_00to15 : 16;
  unsigned int base_00to15 : 16;
  unsigned int base_16to23 : 8;
  unsigned int type : 5;
  unsigned int descriptor_privilege : 2;
  unsigned int present : 1;
  unsigned int limit_16to19 : 4;
  unsigned int available : 1;
  unsigned int code_64_bit : 1;
  unsigned int op_size_32 : 1;
  unsigned int granularity : 1;
  unsigned int base_24to31 : 8;
};

/*
 * Type used to communicate with query_information_process and
 * set_information process.
 */
typedef struct {
  DWORD byte_offset;
  DWORD size;
  struct LdtEntry entries[1];
} LdtInfo;

/*
 * The module initializer and finalizer gather a collection of Windows entry
 * points used to manipulate the LDT and set up a mutex used to guard LDT
 * manipulation.
 */
typedef LONG (NTAPI *NTSETLDT)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
typedef LONG (NTAPI *NTSETINFO)(HANDLE, DWORD, const VOID*, ULONG);
typedef LONG (WINAPI *NTQUERY)(HANDLE, DWORD, VOID*, DWORD, DWORD*);

static NTQUERY query_information_process;
static NTSETINFO set_information_process;
static NTSETLDT set_ldt_entries;

static struct NaClMutex nacl_ldt_mutex;

int NaClLdtInitPlatformSpecific(void) {
  HMODULE hmod = GetModuleHandleA("ntdll.dll");
  /*
   * query_information_process is used to examine LDT entries to find a free
   * selector, etc.
   */
  query_information_process =
      (NTQUERY)(GetProcAddress(hmod, "NtQueryInformationProcess"));
  if (query_information_process == 0) {
    /*
     * Unable to get query_information_process, which is needed for querying
     * the LDT.
     */
    return 0;
  }

  /*
   * set_information_process is one of the methods used to update an LDT
   * entry for a given selector.
   */
  set_information_process =
      (NTSETINFO)(GetProcAddress(hmod, "ZwSetInformationProcess"));

  /*
   * set_ldt_entries is the other method used to update an LDT entry for a
   * given selector.
   */
  set_ldt_entries = (NTSETLDT)(GetProcAddress(hmod, "NtSetLdtEntries"));
  if (NULL == set_ldt_entries) {
    set_ldt_entries = (NTSETLDT)(GetProcAddress(hmod, "ZwSetLdtEntries"));
  }
  if ((NULL == set_ldt_entries) && (NULL == set_information_process)) {
    /*
     * Unable to locate either method for setting the LDT.
     */
    return 0;
  }

  if (!NaClMutexCtor(&nacl_ldt_mutex)) {
    return 0;
  }

  /*
   * Allocate the last LDT entry to force the LDT to grow to its maximum size.
   */
  return NaClLdtAllocateSelector(LDT_ENTRIES - 1, 0,
                                 NACL_LDT_DESCRIPTOR_DATA, 0, 0, 0);
}

void NaClLdtFiniPlatformSpecific(void) {
  NaClMutexDtor(&nacl_ldt_mutex);
}

/*
 * Find and allocate an available selector, inserting an LDT entry with the
 * appropriate permissions.
 */
uint16_t NaClLdtAllocateSelector(int entry_number,
                                 int size_is_in_pages,
                                 NaClLdtDescriptorType type,
                                 int read_exec_only,
                                 void* base_addr,
                                 uint32_t size_minus_one) {
  int retval;
  struct LdtEntry ldt;

  retval = 0;
  NaClXMutexLock(&nacl_ldt_mutex);

  if (-1 == entry_number) {
    entry_number = NaClFindUnusedEntryNumber();
    if (-1 == entry_number) {
      /*
       * No free entries were available.
       */
      NaClXMutexUnlock(&nacl_ldt_mutex);
      return 0;
    }
  }

  switch (type) {
   case NACL_LDT_DESCRIPTOR_DATA:
    if (read_exec_only) {
      ldt.type = 0x10;  /* Data read only */
    } else {
      ldt.type = 0x12;  /* Data read/write */
    }
    break;
   case NACL_LDT_DESCRIPTOR_CODE:
    if (read_exec_only) {
      ldt.type = 0x18;  /* Code execute */
    } else {
      ldt.type = 0x1a;  /* Code execute/read */
    }
    break;
   default:
    NaClXMutexUnlock(&nacl_ldt_mutex);
    return 0;
  }
  ldt.descriptor_privilege = 3;
  ldt.present = 1;
  ldt.available = 1;   /* TODO(dcs) */
  ldt.code_64_bit = 0;
  ldt.op_size_32 = 1;

  if (size_is_in_pages && ((uintptr_t) base_addr & 0xfff)) {
    /*
     * The base address needs to be page aligned.
     */
    NaClXMutexUnlock(&nacl_ldt_mutex);
    return 0;
  };
  ldt.base_00to15 = ((uintptr_t) base_addr) & 0xffff;
  ldt.base_16to23 = (((uintptr_t) base_addr) >> 16) & 0xff;
  ldt.base_24to31 = (((uintptr_t) base_addr) >> 24) & 0xff;

  if (size_minus_one > 0xfffff) {
    /*
     * If the size is in pages, no more than 2**20 pages can be protected.
     * If the size is in bytes, no more than 2**20 bytes can be protected.
     */
    NaClXMutexUnlock(&nacl_ldt_mutex);
    return 0;
  }
  ldt.limit_00to15 = size_minus_one & 0xffff;
  ldt.limit_16to19 = (size_minus_one >> 16) & 0xf;
  ldt.granularity = size_is_in_pages;

  /*
   * Install the LDT entry.
   */
  if (NULL != set_ldt_entries) {
    union {
      struct LdtEntry ldt;
      DWORD dwords[2];
    } u;
    u.ldt = ldt;

    retval = (*set_ldt_entries)((entry_number << 3) | 0x7,
                                u.dwords[0],
                                u.dwords[1],
                                0,
                                0,
                                0);
  }

  if ((NULL == set_ldt_entries) || (0 != retval)) {
    LdtInfo info;
    info.byte_offset = entry_number << 3;
    info.size = sizeof(struct LdtEntry);
    info.entries[0] = ldt;
    retval = (*set_information_process)((HANDLE)-1, 10, (void*)&info, 16);
  }

  if (0 != retval) {
    NaClXMutexUnlock(&nacl_ldt_mutex);
    return 0;
  }

  /*
   * Return an LDT selector with a requested privilege level of 3.
   */
  NaClXMutexUnlock(&nacl_ldt_mutex);
  return (uint16_t)((entry_number << 3) | 0x7);
}

/*
 * Allocates a selector whose size is specified in pages.
 * Page granular protection requires that the start address be page-aligned.
 */
uint16_t NaClLdtAllocatePageSelector(NaClLdtDescriptorType type,
                                     int read_exec_only,
                                     void* base_addr,
                                     uint32_t size_in_pages) {
  return NaClLdtAllocateSelector(-1, 1, type, read_exec_only, base_addr,
                                 size_in_pages - 1);
}

/*
 * Allocates a selector whose size is specified in bytes.
 */
uint16_t NaClLdtAllocateByteSelector(NaClLdtDescriptorType type,
                                     int read_exec_only,
                                     void* base_addr,
                                     uint32_t size_in_bytes) {
  return NaClLdtAllocateSelector(-1, 0, type, read_exec_only, base_addr,
                                 size_in_bytes - 1);
}

/*
 * Change a selector whose size is specified in pages.
 * Page granular protection requires that the start address be page-aligned.
 */
uint16_t NaClLdtChangePageSelector(int32_t entry_number,
                                   NaClLdtDescriptorType type,
                                   int read_exec_only,
                                   void* base_addr,
                                   uint32_t size_in_pages) {
  if ((uint32_t) entry_number >= LDT_ENTRIES) {
    return 0;
  }
  return NaClLdtAllocateSelector(entry_number, 1, type, read_exec_only,
                                 base_addr, size_in_pages - 1);
}

/*
 * Change a selector whose size is specified in bytes.
 */
uint16_t NaClLdtChangeByteSelector(int32_t entry_number,
                                   NaClLdtDescriptorType type,
                                   int read_exec_only,
                                   void* base_addr,
                                   uint32_t size_in_bytes) {
  if ((uint32_t) entry_number >= LDT_ENTRIES) {
    return 0;
  }
  return NaClLdtAllocateSelector(entry_number, 0, type, read_exec_only,
                                 base_addr, size_in_bytes - 1);
}

void NaClLdtPrintSelector(uint16_t selector) {
  /* type_name converts the segment type into print name */
  static const char* type_name[] = {
    "data read only",
    "data read_only accessed",
    "data read write",
    "data read write accessed",
    "data read expand",
    "data read expand accessed",
    "data read write expand",
    "data read write expand accessed",
    "code execute",
    "code execute accessed",
    "code execute read",
    "code execute read accessed",
    "code execute conforming",
    "code execute conforming accessed",
    "code execute read conforming",
    "code execute read conforming accessed"
  };

  struct LdtEntry entry;
  int retval;

  /*
   * Try the first method, using GetThreadSelectorEntry.
   */
  retval = GetThreadSelectorEntry(GetCurrentThread(), selector,
                                  (LPLDT_ENTRY)&entry);
  if (0 != retval) {
    /*
     * Failed to get the entry.  Try using query_information_process.
     */
    DWORD len;
    LdtInfo info;
    memset(&info, 0, sizeof(LdtInfo));
    info.byte_offset = selector & ~0x7;
    info.size = sizeof(struct LdtEntry);
    retval = query_information_process((HANDLE)-1, 10, (void*)&info, 16, &len);
    if (0 != retval) {
      return;
    }
    entry = info.entries[0];
  }

  printf("DESCRIPTOR for selector %04x\n", selector);
  /* create functions to do base, limit, and type */
  printf("  base        %08x\n", (entry.base_24to31 << 24)
         | (entry.base_16to23 << 16) | entry.base_00to15);
  printf("  limit       %08x%s\n",
         (((entry.limit_16to19 << 16) | entry.limit_00to15)
          << (entry.granularity ? 12 : 0)),
         (entry.granularity ? " (page granularity)" : ""));
  printf("  type        %s, %s\n", ((entry.type & 0x10) ? "user" : "system"),
         type_name[entry.type & 0xf]);
  printf("  privilege   %d\n", entry.descriptor_privilege);
  printf("  present %s\n", (entry.present ? "yes" : "no"));
  printf("  available %s\n", (entry.available ? "yes" : "no"));
  printf("  64-bit code %s\n", (entry.code_64_bit ? "yes" : "no"));
  printf("  op size %s\n", (entry.op_size_32 ? "32" : "16"));
}

/*
 * Mark a selector as available for future reuse.
 */
void NaClLdtDeleteSelector(uint16_t selector) {
  int retval;
  union {
    struct LdtEntry entry;
    DWORD dwords[2];
  } u;
  retval = 0;
  u.entry.base_00to15 = 0;
  u.entry.base_16to23 = 0;
  u.entry.base_24to31 = 0;
  u.entry.limit_00to15 = 0;
  u.entry.limit_16to19 = 0;
  u.entry.type = 0x10;
  u.entry.descriptor_privilege = 3;
  u.entry.present = 0;
  u.entry.available = 0;
  u.entry.code_64_bit = 0;
  u.entry.op_size_32 = 1;
  u.entry.granularity = 1;

  NaClXMutexLock(&nacl_ldt_mutex);
  if (NULL != set_ldt_entries) {
    retval = (*set_ldt_entries)(selector, u.dwords[0], u.dwords[1], 0, 0, 0);
  }

  if ((NULL == set_ldt_entries) || (0 != retval)) {
    LdtInfo info;
    info.byte_offset = selector & ~0x7;
    info.size = sizeof(struct LdtEntry);
    info.entries[0] = u.entry;
    retval = (*set_information_process)((HANDLE)-1, 10, (void*)&info, 16);
  }
  NaClXMutexUnlock(&nacl_ldt_mutex);
}

/*
 * Find a free selector.  Always invoked while holding nacl_ldt_mutex.
 */
static int NaClFindUnusedEntryNumber(void) {
  int index;
  int retval;

  /*
   * Windows doesn't appear to allow setting the first element of the
   * LDT to a user specified value.  Therefore we start scanning at 1.
   */
  for (index = 1; index < LDT_ENTRIES; ++index) {
    DWORD len;
    LdtInfo info;
    memset(&info, 0, sizeof(LdtInfo));
    info.byte_offset = index << 3;
    info.size = sizeof(struct LdtEntry);
    /* TODO(sehr): change parameters to allow only one call to query_info... */
    retval = query_information_process((HANDLE)-1, 10, (void*)&info, 16, &len);
    if ((0 == retval) && !info.entries[0].present) {
      return index;
    }
  }
  return -1;
}
