/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* @file
 *
 * Minimal ELF header declaration / constants for Elf32* values.
 * Constants are defined only for fields that are actualy
 * used.  (Unused constants for used fields are include only for
 * "completeness", though of course in many cases there are more
 * values in use, e.g., the EM_* values for e_machine.)
 *
 * (Re)Created from the ELF specification at
 * http://x86.ddj.com/ftp/manuals/tools/elf.pdf which is referenced
 * from wikipedia article
 * http://en.wikipedia.org/wki/Executable_and_Linkable_Format
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_ELF32_H_
#define NATIVE_CLIENT_SRC_INCLUDE_ELF32_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/elf_constants.h"

EXTERN_C_BEGIN

/* assumes 32-bit int, 16-bit short, 8-bit char */

/* Define 32-bit specific types */
typedef uint32_t    Elf32_Addr;   /* alignment 4 */
typedef uint16_t    Elf32_Half;   /* alignment 2 */
typedef uint32_t    Elf32_Off;    /* alignment 4 */
typedef int32_t     Elf32_Sword;  /* alignment 4 */
typedef uint32_t    Elf32_Word;   /* alignment 4 */
/* unsigned char, size 1, alignment 1 */

/* Define the structure of the file header for 32 bits. */
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf32_Half    e_type;
  Elf32_Half    e_machine;
  Elf32_Word    e_version;
  Elf32_Addr    e_entry;
  Elf32_Off     e_phoff;
  Elf32_Off     e_shoff;
  Elf32_Word    e_flags;
  Elf32_Half    e_ehsize;
  Elf32_Half    e_phentsize;
  Elf32_Half    e_phnum;
  Elf32_Half    e_shentsize;
  Elf32_Half    e_shnum;
  Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

/* Define the structure of a program header table for 32-bits. */
typedef struct {
  Elf32_Word    p_type;
  Elf32_Off     p_offset;
  Elf32_Addr    p_vaddr;
  Elf32_Addr    p_paddr;
  Elf32_Word    p_filesz;
  Elf32_Word    p_memsz;
  Elf32_Word    p_flags;
  Elf32_Word    p_align;
} Elf32_Phdr;

/*
 * Define 32-bit section headers.
 * ncfileutil wants section headers, even though service runtime does
 * not.
 */
typedef struct {
  Elf32_Word  sh_name;
  Elf32_Word  sh_type;
  Elf32_Word  sh_flags;
  Elf32_Addr  sh_addr;
  Elf32_Off   sh_offset;
  Elf32_Word  sh_size;
  Elf32_Word  sh_link;
  Elf32_Word  sh_info;
  Elf32_Word  sh_addralign;
  Elf32_Word  sh_entsize;
} Elf32_Shdr;

typedef struct {
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;

#define ELF32_R_TYPE(val) ((val) & 0xff)

typedef struct {
  Elf32_Sword d_tag;
  union {
    Elf32_Word d_val;
    Elf32_Addr d_ptr;
  } d_un;
} Elf32_Dyn;

typedef struct {
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

/*
 * The auxiliary vector is passed on the stack between ELF loaders,
 * dynamic linkers, and program startup code.  The gratuitous union
 * is the historical standard API, though it has no purpose today.
 */
typedef struct {
  Elf32_Word a_type;            /* Entry type */
  union {
      Elf32_Word a_val;         /* Integer value */
    } a_un;
} Elf32_auxv_t;


EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_ELF32_H_ */
