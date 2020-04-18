/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl error codes.
 */

#ifndef SERVICE_RUNTIME_NACL_ERROR_CODE_H__
#define SERVICE_RUNTIME_NACL_ERROR_CODE_H__   1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * These error codes are reported via UMA so, if you edit them:
 * 1) make sure you understand UMA, first.
 * 2) update src/tools/metrics/histograms/histograms.xml in Chromium
 * 3) never reuse old numbers for a different meaning; add new ones on the end
 * Values are explicitly specified to make sure they don't shift around when
 * edited, and also to make reading about:histograms easier.
 */
typedef enum NaClErrorCode {
  LOAD_OK = 0,
  LOAD_STATUS_UNKNOWN = 1,  /* load status not available yet */
  LOAD_UNSUPPORTED_OS_PLATFORM = 2,
  LOAD_DEP_UNSUPPORTED = 3,
  LOAD_INTERNAL = 4,
  LOAD_DUP_LOAD_MODULE = 5,
  LOAD_DUP_START_MODULE = 6,
  LOAD_OPEN_ERROR = 7,
  LOAD_READ_ERROR = 8,
  LOAD_TOO_MANY_PROG_HDRS = 9,
  LOAD_BAD_PHENTSIZE = 10,
  LOAD_BAD_ELF_MAGIC = 11,
  LOAD_NOT_32_BIT = 12,
  LOAD_NOT_64_BIT = 13,
  LOAD_BAD_ABI = 14,
  LOAD_NOT_EXEC = 15,
  LOAD_BAD_MACHINE = 16,
  LOAD_BAD_ELF_VERS = 17,
  LOAD_TOO_MANY_SECT = 18,
  LOAD_BAD_SECT = 19,
  LOAD_NO_MEMORY = 20,
  LOAD_SECT_HDR = 21,
  LOAD_ADDR_SPACE_TOO_SMALL = 22,
  LOAD_ADDR_SPACE_TOO_BIG = 23,
  LOAD_DATA_OVERLAPS_STACK_SECTION = 24,
  LOAD_RODATA_OVERLAPS_DATA = 25,
  LOAD_DATA_NOT_LAST_SEGMENT = 26,
  LOAD_NO_DATA_BUT_RODATA_NOT_LAST_SEGMENT = 27,
  LOAD_TEXT_OVERLAPS_RODATA = 28,
  LOAD_TEXT_OVERLAPS_DATA = 29,
  LOAD_BAD_RODATA_ALIGNMENT = 30,
  LOAD_BAD_DATA_ALIGNMENT = 31,
  LOAD_UNLOADABLE = 32,
  LOAD_BAD_ELF_TEXT = 33,
  LOAD_TEXT_SEG_TOO_BIG = 34,
  LOAD_DATA_SEG_TOO_BIG = 35,
  LOAD_MPROTECT_FAIL = 36,
  LOAD_MADVISE_FAIL = 37,
  LOAD_TOO_MANY_SYMBOL_STR = 38,
  LOAD_SYMTAB_ENTRY_TOO_SMALL = 39,
  LOAD_NO_SYMTAB = 40,
  LOAD_NO_SYMTAB_STRINGS = 41,
  LOAD_SYMTAB_ENTRY = 42,
  LOAD_UNKNOWN_SYMBOL_TYPE = 43,
  LOAD_SYMTAB_DUP = 44,
  LOAD_REL_ERROR = 45,
  LOAD_REL_UNIMPL = 46,
  LOAD_UNDEF_SYMBOL = 47,
  LOAD_BAD_SYMBOL_DATA = 48,
  LOAD_BAD_FILE = 49,
  LOAD_BAD_ENTRY = 50,
  LOAD_SEGMENT_OUTSIDE_ADDRSPACE = 51,
  LOAD_DUP_SEGMENT = 52,
  LOAD_SEGMENT_BAD_LOC = 53,
  LOAD_BAD_SEGMENT = 54,
  LOAD_REQUIRED_SEG_MISSING = 55,
  LOAD_SEGMENT_BAD_PARAM = 56,
  LOAD_VALIDATION_FAILED = 57,
  LOAD_UNIMPLEMENTED = 58,
  /*
   * service runtime errors (post load, during startup phase)
   */
  SRT_NO_SEG_SEL = 59,

  LOAD_BAD_EHSIZE = 60,
  LOAD_EHDR_OVERFLOW = 61,
  LOAD_PHDR_OVERFLOW = 62,
  /* CPU blacklist or whitelist error */
  LOAD_UNSUPPORTED_CPU = 63,
  LOAD_NO_MEMORY_FOR_DYNAMIC_TEXT = 64,
  LOAD_NO_MEMORY_FOR_ADDRESS_SPACE = 65,
  LOAD_CODE_SEGMENT_TOO_LARGE = 66,
  NACL_ERROR_CODE_MAX
} NaClErrorCode;

char const  *NaClErrorString(NaClErrorCode  errcode);

#ifdef __cplusplus
}  /* end of extern "C" */
#endif

#endif
