/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Derived from Linux and Windows versions.
 * TODO(jrg): PrintSelector() #if 0'd.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
/* for LDT_ENTRIES */
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"

/* OSX specific */
#include <architecture/i386/desc.h>  /* ldt_desc, etc */
#include <architecture/i386/table.h> /* ldt_entry_t */
#include <architecture/i386/sel.h>   /* sel_t */
#include <i386/user_ldt.h>           /* i386_set_ldt() */


int NaClLdtInitPlatformSpecific(void) {
  return 1;
}

void NaClLdtFiniPlatformSpecific(void) {
}

static uint16_t BuildSelector(int sel_index) {
  union {
    sel_t sel;
    uint16_t uintsel;
  } u;

  /*
   * Return an LDT selector.
   */
  u.sel.rpl = USER_PRIV;
  u.sel.ti = SEL_LDT;
  u.sel.index = sel_index;

  return u.uintsel;
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
  ldt_entry_t ldt;
  int sel_index = -1;

  memset(&ldt, 0, sizeof(ldt));

  switch (type) {
    case NACL_LDT_DESCRIPTOR_DATA:
      ldt.data.type = (read_exec_only ? DESC_DATA_RONLY : DESC_DATA_WRITE);
      break;
    case NACL_LDT_DESCRIPTOR_CODE:
      ldt.code.type = (read_exec_only ? DESC_CODE_EXEC : DESC_CODE_READ);
      break;
    default:
      return 0;
  }

  /* Ideas from win/nacl_ldt.c */
  ldt.code.dpl = 3;
  ldt.code.present = 1;
  ldt.code.opsz = 1;

  if ((size_is_in_pages && ((unsigned long) base_addr & 0xfff)) ||
      (size_minus_one > 0xfffff)) {
    /*
     * The base address needs to be page aligned if page granularity.
     * If size is in pages no more than 2**20 pages can be protected.
     * If size is in bytes no more than 2**20 bytes can be protected.
     */
    return 0;
  }
  ldt.code.base00 = ((unsigned long) base_addr) & 0xffff;
  ldt.code.base16 = (((unsigned long) base_addr) >> 16) & 0xff;
  ldt.code.base24 = (((unsigned long) base_addr) >> 24) & 0xff;

  ldt.code.limit00 = size_minus_one & 0xffff;
  ldt.code.limit16 = (size_minus_one >> 16) & 0xf;
  ldt.code.granular = (size_is_in_pages ? DESC_GRAN_PAGE : DESC_GRAN_BYTE);

  /*
   * Install the LDT entry. -1 means caller did not specify, so let the kernel
   * allocate any available entry.
   */
  DCHECK(-1 == (int) LDT_AUTO_ALLOC);
  sel_index = i386_set_ldt(entry_number, &ldt, 1);

  if (-1 == sel_index) {
    return 0;
  }

  return BuildSelector(sel_index);
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
 * Changes a selector whose size is specified in pages.
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
 * Changes a selector whose size is specified in bytes.
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
  /* TODO(jrg) */
#if 0
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
#else
  UNREFERENCED_PARAMETER(selector);
#endif
}

/*
 * Mark a selector as available for future reuse.
 */
void NaClLdtDeleteSelector(uint16_t selector) {
  if (-1 == i386_set_ldt(selector >> 3, NULL, 1)) {
    perror("NaClLdtDeleteSelector: i386_set_ldt()");
  }
}
