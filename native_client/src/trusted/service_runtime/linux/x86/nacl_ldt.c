/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl local descriptor table manipulation support.
 */

#include <asm/ldt.h>
#include <stdio.h>

#include "native_client/src/include/build_config.h"

#if NACL_ANDROID
#include <sys/syscall.h>
#endif

#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"

#if NACL_ANDROID
/* Android doesn't have standard modify_ldt() function. */
int modify_ldt(int func, void *ptr, unsigned long bytecount) {
  return syscall(__NR_modify_ldt, func, ptr, bytecount);
}
#endif

/*
 * struct LdtEntry is a structure that is laid out exactly as the segment
 * descriptors described in the Intel reference manuals.  This is needed
 * because Mac and Windows use this representation in the methods used to
 * set and get entries in the local descriptor table (LDT), but use different
 * names for the members.  Linux uses a different representation to set, but
 * also reads entries back in this format.  It needs to be laid out packed,
 * bitwise, little endian.
 */
struct LdtEntry {
  uint16_t limit_00to15;
  uint16_t base_00to15;

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
 * The module initializer and finalizer set up a mutex used to guard LDT
 * manipulation.
 */
static struct NaClMutex nacl_ldt_mutex;

int NaClLdtInitPlatformSpecific(void) {
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
 * Find a free selector.  Always invoked while holding nacl_ldt_mutex.
 */
static int NaClFindUnusedEntryNumber(void) {
  int size = sizeof(struct LdtEntry) * LDT_ENTRIES;
  struct LdtEntry *entries = malloc(size);
  int i;

  int retval = modify_ldt(0, entries, size);

  if (-1 != retval) {
    retval = -1;  /* In case we don't find any free entry */
    for (i = 0; i < LDT_ENTRIES; ++i) {
      if (!entries[i].present) {
        retval = i;
        break;
      }
    }
  }

  free(entries);
  return retval;
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
  struct user_desc ud;
  int retval;

  NaClXMutexLock(&nacl_ldt_mutex);

  if (-1 == entry_number) {
    /* -1 means caller did not specify -- allocate */
    entry_number = NaClFindUnusedEntryNumber();

    if (-1 == entry_number) {
      /*
       * No free entries were available.
       */
      goto alloc_error;
    }
  }
  ud.entry_number = entry_number;

  switch (type) {
    case NACL_LDT_DESCRIPTOR_DATA:
      ud.contents = MODIFY_LDT_CONTENTS_DATA;
      break;
    case NACL_LDT_DESCRIPTOR_CODE:
      ud.contents = MODIFY_LDT_CONTENTS_CODE;
      break;
    default:
      goto alloc_error;
  }
  ud.read_exec_only = read_exec_only;
  ud.seg_32bit = 1;
  ud.seg_not_present = 0;
  ud.useable = 1;

  if (size_is_in_pages && ((unsigned long) base_addr & 0xfff)) {
    /*
     * The base address needs to be page aligned.
     */
    goto alloc_error;
  };
  ud.base_addr = (unsigned long) base_addr;

  if (size_minus_one > 0xfffff) {
    /*
     * If size is in pages, no more than 2**20 pages can be protected.
     * If size is in bytes, no more than 2**20 bytes can be protected.
     */
    goto alloc_error;
  }
  ud.limit = size_minus_one;
  ud.limit_in_pages = size_is_in_pages;

  /*
   * Install the LDT entry.
   */
  retval = modify_ldt(1, &ud, sizeof ud);
  if (-1 == retval) {
    goto alloc_error;
  }

  /*
   * Return an LDT selector with a requested privilege level of 3.
   */
  NaClXMutexUnlock(&nacl_ldt_mutex);
  return (ud.entry_number << 3) | 0x7;

  /*
   * All error returns go through this epilog.
   */
 alloc_error:
  NaClXMutexUnlock(&nacl_ldt_mutex);
  return 0;
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

  struct LdtEntry entries[LDT_ENTRIES];
  struct LdtEntry entry;
  int retval = modify_ldt(0, entries, sizeof(entries));
  if (-1 == retval) {
    return;
  }
  entry = entries[selector >> 3];
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
  struct user_desc ud;
  ud.entry_number = selector >> 3;
  ud.seg_not_present = 1;
  ud.base_addr = 0;
  ud.limit = 0;
  ud.limit_in_pages = 0;
  ud.read_exec_only = 0;
  ud.seg_32bit = 0;
  ud.useable = 0;
  ud.contents = MODIFY_LDT_CONTENTS_DATA;
  NaClXMutexLock(&nacl_ldt_mutex);
  modify_ldt(1, &ud, sizeof ud);
  NaClXMutexUnlock(&nacl_ldt_mutex);
}
