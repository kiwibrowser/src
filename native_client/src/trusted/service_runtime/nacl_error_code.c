/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

#include "native_client/src/shared/platform/nacl_log.h"

char const  *NaClErrorString(NaClErrorCode errcode) {
  switch (errcode) {
    case LOAD_OK:
      return "Ok";
    case LOAD_STATUS_UNKNOWN:
      return "Load status unknown (load incomplete)";
    case LOAD_UNSUPPORTED_OS_PLATFORM:
      return "Operating system platform is not supported";
    case LOAD_DEP_UNSUPPORTED:
      return "Data Execution Prevention is required but is not supported";
    case LOAD_INTERNAL:
      return "Internal error";
    case LOAD_DUP_LOAD_MODULE:  /* -R: nexe supplied by RPC, but only once! */
      return "Multiple LoadModule RPCs";
    case LOAD_DUP_START_MODULE:  /* -X: implies StartModule RPC, but once! */
      return "Multiple StartModule RPCs";
    case LOAD_OPEN_ERROR:
      return "Cannot open NaCl module file";
    case LOAD_READ_ERROR:
      return "Cannot read file";
    case LOAD_TOO_MANY_PROG_HDRS:
      return "Too many program header entries in ELF file";
    case LOAD_BAD_PHENTSIZE:
      return "ELF program header size wrong";
    case LOAD_BAD_ELF_MAGIC:
      return "Bad ELF header magic number";
    case LOAD_NOT_32_BIT:
      return "Not a 32-bit ELF file";
    case LOAD_NOT_64_BIT:
      return "Not a 64-bit ELF file";
    case LOAD_BAD_ABI:
      return "ELF file has unexpected OS ABI";
    case LOAD_NOT_EXEC:
      return "ELF file type not executable";
    case LOAD_BAD_MACHINE:
      return "ELF file for wrong architecture";
    case LOAD_BAD_ELF_VERS:
      return "ELF version mismatch";
    case LOAD_TOO_MANY_SECT:
      return "Too many section headers";
    case LOAD_BAD_SECT:
      return "ELF bad sections";
    case LOAD_NO_MEMORY:
      return "Insufficient memory to load file";
    case LOAD_SECT_HDR:
      return "ELF section header string table load error";
    case LOAD_ADDR_SPACE_TOO_SMALL:
      return "Address space too small";
    case LOAD_ADDR_SPACE_TOO_BIG:
      return "Address space too big";
    case LOAD_DATA_OVERLAPS_STACK_SECTION:
      return ("Memory \"hole\" between end of BSS and start of stack"
              " is negative in size");
    case LOAD_RODATA_OVERLAPS_DATA:
      return "Read-only data segment overlaps data segment";
    case LOAD_DATA_NOT_LAST_SEGMENT:
      return "Data segment exists, but is not last segment";
    case LOAD_NO_DATA_BUT_RODATA_NOT_LAST_SEGMENT:
      return ("No data segment, read-only data segment exists,"
              " but is not last segment");
    case LOAD_TEXT_OVERLAPS_RODATA:
      return "Text segment overlaps rodata segment";
    case LOAD_TEXT_OVERLAPS_DATA:
      return "No rodata segment, and text segment overlaps data segment";
    case LOAD_BAD_RODATA_ALIGNMENT:
      return "The rodata segment is not properly aligned";
    case LOAD_BAD_DATA_ALIGNMENT:
      return "The data segment is not properly aligned";
    case LOAD_UNLOADABLE:
      return "Error during loading";
    case LOAD_BAD_ELF_TEXT:
      return "ELF file contains no text segment";
    case LOAD_TEXT_SEG_TOO_BIG:
      return "ELF file text segment too large";
    case LOAD_DATA_SEG_TOO_BIG:
      return "ELF file data segment(s) too large";
    case LOAD_MPROTECT_FAIL:
      return "Cannot protect pages";
    case LOAD_MADVISE_FAIL:
      return "Cannot release unused data segment";
    case LOAD_TOO_MANY_SYMBOL_STR:
      return "Malformed ELF file: too many string tables";
    case LOAD_SYMTAB_ENTRY_TOO_SMALL:
      return "Symbol table entry too small";
    case LOAD_NO_SYMTAB:
      return "No symbol table";
    case LOAD_NO_SYMTAB_STRINGS:
      return "No string table for symbols";
    case LOAD_SYMTAB_ENTRY:
      return "Error entering new symbol into symbol table";
    case LOAD_UNKNOWN_SYMBOL_TYPE:
      return "Unknown symbol type";
    case LOAD_SYMTAB_DUP:
      return "Duplicate entry in symbol table";
    case LOAD_REL_ERROR:
      return "Bad relocation read error";
    case LOAD_REL_UNIMPL:
      return "Relocation type unimplemented";
    case LOAD_UNDEF_SYMBOL:
      return "Undefined external symbol";
    case LOAD_BAD_SYMBOL_DATA:
      return "Bad symbol table data";
    case LOAD_BAD_FILE:
      return "ELF file not accessible";
    case LOAD_BAD_ENTRY:
      return "Bad program entry point address";
    case LOAD_SEGMENT_OUTSIDE_ADDRSPACE:
      return ("ELF executable contains a segment which lies outside"
              " the assigned address space");
    case LOAD_DUP_SEGMENT:
      return ("ELF executable contains a duplicate segment"
              " (please run objdump to see which)");
    case LOAD_SEGMENT_BAD_LOC:
      return "ELF executable text/rodata segment has wrong starting address";
    case LOAD_BAD_SEGMENT:
      return "ELF executable contains an unexpected/unallowed segment/flags";
    case LOAD_REQUIRED_SEG_MISSING:
      return "ELF executable missing a required segment (text)";
    case LOAD_SEGMENT_BAD_PARAM:
      return "ELF executable segment header parameter error";
    case LOAD_VALIDATION_FAILED:
      return "Validation failure. File violates Native Client safety rules.";
    case LOAD_UNIMPLEMENTED:
      return "Not implemented for this architecture.";
    case SRT_NO_SEG_SEL:
      return "Service Runtime: cannot allocate segment selector";
    case LOAD_BAD_EHSIZE:
      return "ELFCLASS64 file header has wrong e_ehsize value";
    case LOAD_EHDR_OVERFLOW:
      return "ELFCLASS64 file header has fields that overflow 32 bits";
    case LOAD_PHDR_OVERFLOW:
      return "ELFCLASS64 program header has fields that overflow 32 bits";
    case LOAD_UNSUPPORTED_CPU:
      return "CPU model is not supported";
    case LOAD_NO_MEMORY_FOR_DYNAMIC_TEXT:
      return "Insufficient memory to allocate dynamic text region";
    case LOAD_NO_MEMORY_FOR_ADDRESS_SPACE:
      return "Insufficient memory to allocate untrusted address space";
    case LOAD_CODE_SEGMENT_TOO_LARGE:
      return "ELF executable's code segment is larger than the "
             "arbitrary size limit imposed to mitigate spraying attacks";
    case NACL_ERROR_CODE_MAX:
      /* A bad error code, but part of the enum so we need to list it here. */
      break;
  }

  /*
   * do not use default case label, to make sure that the compiler
   * will generate a warning with -Wswitch-enum for new codes
   * introduced in nacl_error_codes.h for which there is no
   * corresponding entry here.  instead, we pretend that fall-through
   * from the switch is possible.  (otherwise -W complains control
   * reaches end of non-void function.)
   */
  return "BAD ERROR CODE";
}
