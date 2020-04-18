/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* @file
 *
 * Minimal ELF header declaration / constants.  Only Elf values are
 * handled, and constants are defined only for fields that are actually
 * used.  (Unused constants for used fields are include only for
 * "completeness", though of course in many cases there are more
 * values in use, e.g., the EM_* values for e_machine.)
 *
 * Note: We define both 32 and 64 bit versions of the elf file in
 * separate files.  We define sizeless aliases here to keep most other
 * code width-agnostic.  But in fact, we really only deal with 32-bit
 * formats internally.  For compatibility with past practice, we
 * recognize the 64-bit format for x86-64, but we translate that to
 * 32-bit format in our own data structures.
 *
 * (Re)Created from the ELF specification at
 * http://x86.ddj.com/ftp/manuals/tools/elf.pdf which is referenced
 * from wikipedia article
 * http://en.wikipedia.org/wki/Executable_and_Linkable_Format
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_ELF_H_
#define NATIVE_CLIENT_SRC_INCLUDE_ELF_H_ 1


#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf64.h"


EXTERN_C_BEGIN

#define NACL_ELF_CLASS ELFCLASS32

#define NACL_PRIdElf_Addr   NACL_PRId32
#define NACL_PRIiElf_Addr   NACL_PRIi32
#define NACL_PRIoElf_Addr   NACL_PRIo32
#define NACL_PRIuElf_Addr   NACL_PRIu32
#define NACL_PRIxElf_Addr   NACL_PRIx32
#define NACL_PRIXElf_Addr   NACL_PRIX32

#define NACL_PRIxElf_AddrAll   "08" NACL_PRIx32
#define NACL_PRIXElf_AddrAll   "08" NACL_PRIX32

#define NACL_PRIdElf_Off    NACL_PRId32
#define NACL_PRIiElf_Off    NACL_PRIi32
#define NACL_PRIoElf_Off    NACL_PRIo32
#define NACL_PRIuElf_Off    NACL_PRIu32
#define NACL_PRIxElf_Off    NACL_PRIx32
#define NACL_PRIXElf_Off    NACL_PRIX32

#define NACL_PRIdElf_Half   NACL_PRId16
#define NACL_PRIiElf_Half   NACL_PRIi16
#define NACL_PRIoElf_Half   NACL_PRIo16
#define NACL_PRIuElf_Half   NACL_PRIu16
#define NACL_PRIxElf_Half   NACL_PRIx16
#define NACL_PRIXElf_Half   NACL_PRIX16

#define NACL_PRIdElf_Word   NACL_PRId32
#define NACL_PRIiElf_Word   NACL_PRIi32
#define NACL_PRIoElf_Word   NACL_PRIo32
#define NACL_PRIuElf_Word   NACL_PRIu32
#define NACL_PRIxElf_Word   NACL_PRIx32
#define NACL_PRIXElf_Word   NACL_PRIX32

#define NACL_PRIdElf_Sword  NACL_PRId32z
#define NACL_PRIiElf_Sword  NACL_PRIi32z
#define NACL_PRIoElf_Sword  NACL_PRIo32z
#define NACL_PRIuElf_Sword  NACL_PRIu32z
#define NACL_PRIxElf_Sword  NACL_PRIx32z
#define NACL_PRIXElf_Sword  NACL_PRIX32z

#define NACL_PRIdElf_Xword  NACL_PRId32
#define NACL_PRIiElf_Xword  NACL_PRIi32
#define NACL_PRIoElf_Xword  NACL_PRIo32
#define NACL_PRIuElf_Xword  NACL_PRIu32
#define NACL_PRIxElf_Xword  NACL_PRIx32
#define NACL_PRIXElf_Xword  NACL_PRIX32

#define NACL_PRIdElf_Sxword NACL_PRId32
#define NACL_PRIiElf_Sxword NACL_PRIi32
#define NACL_PRIoElf_Sxword NACL_PRIo32
#define NACL_PRIuElf_Sxword NACL_PRIu32
#define NACL_PRIxElf_Sxword NACL_PRIx32
#define NACL_PRIXElf_Sxword NACL_PRIX32

/* Define sub architecture neutral types */
typedef Elf32_Addr  Elf_Addr;
typedef Elf32_Off   Elf_Off;
typedef Elf32_Half  Elf_Half;
typedef Elf32_Word  Elf_Word;
typedef Elf32_Sword Elf_Sword;
typedef Elf32_Word  Elf_Xword;
typedef Elf32_Sword Elf_Xsword;

/* Define ranges for elf types. */
#define MIN_ELF_ADDR 0x0
#define MAX_ELF_ADDR 0xffffffff

/* Define a neutral form of the file header. */
typedef Elf32_Ehdr Elf_Ehdr;

/* Define a neutral form of a program header. */
typedef Elf32_Phdr Elf_Phdr;

/* Define neutral section headers. */
typedef Elf32_Shdr Elf_Shdr;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_ELF_H_ */
