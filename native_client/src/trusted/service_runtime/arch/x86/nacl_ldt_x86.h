/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl local descriptor table (LDT) managment
 */
#ifndef SERVICE_RUNTIME_NACL_LDT_H__
#define SERVICE_RUNTIME_NACL_LDT_H__ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/* TODO(petr): this should go to linux/x86 */
#if NACL_LINUX
/*
 * The modify_ldt system call is used to get and set the local descriptor
 * table.
 */
extern int modify_ldt(int func, void* ptr, unsigned long bytecount);
#endif

/*
 * Module initialization and finalization.
 */
extern int NaClLdtInit(void);
extern void NaClLdtFini(void);

/*
 * NaClLdtAllocateSelector creates an entry installed in the local descriptor
 * table. If successfully installed, it returns a non-zero selector.  If it
 * fails to install the entry, it returns zero.
 */
typedef enum {
  NACL_LDT_DESCRIPTOR_DATA,
  NACL_LDT_DESCRIPTOR_CODE
} NaClLdtDescriptorType;

uint16_t NaClLdtAllocatePageSelector(NaClLdtDescriptorType type,
                                     int read_exec_only,
                                     void* base_addr,
                                     uint32_t size_in_pages);

uint16_t NaClLdtAllocateByteSelector(NaClLdtDescriptorType type,
                                     int read_exec_only,
                                     void* base_addr,
                                     uint32_t size_in_bytes);

uint16_t NaClLdtChangePageSelector(int32_t entry_number,
                                   NaClLdtDescriptorType type,
                                   int read_exec_only,
                                   void* base_addr,
                                   uint32_t size_in_pages);

uint16_t NaClLdtChangeByteSelector(int32_t entry_number,
                                   NaClLdtDescriptorType type,
                                   int read_exec_only,
                                   void* base_addr,
                                   uint32_t size_in_bytes);

uint16_t NaClLdtAllocateSelector(int entry_number,
                                 int size_is_in_pages,
                                 NaClLdtDescriptorType type,
                                 int read_exec_only,
                                 void* base_addr,
                                 uint32_t size_minus_one);

/*
 * NaClLdtDeleteSelector frees the LDT entry associated with a given selector.
 */
void NaClLdtDeleteSelector(uint16_t selector);

/*
 * NaClLdtPrintSelector prints the local descriptor table for the specified
 * selector.
 */
void NaClLdtPrintSelector(uint16_t selector);

EXTERN_C_END

#endif

