/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl helper functions to deal with elf images
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/elf_constants.h"
#include "native_client/src/include/elf.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"

#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/fault_injection/fault_injection.h"
#include "native_client/src/trusted/perf_counter/nacl_perf_counter.h"
#include "native_client/src/trusted/service_runtime/elf_util.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"
#include "native_client/src/trusted/validator/validation_metadata.h"

/* private */
struct NaClElfImage {
  Elf_Ehdr  ehdr;
  Elf_Phdr  phdrs[NACL_MAX_PROGRAM_HEADERS];
  int       loadable[NACL_MAX_PROGRAM_HEADERS];
};


enum NaClPhdrCheckAction {
  PCA_NONE,
  PCA_TEXT_CHECK,
  PCA_RODATA,
  PCA_DATA,
  PCA_IGNORE  /* ignore this segment. */
};


struct NaClPhdrChecks {
  Elf_Word                  p_type;
  Elf_Word                  p_flags;  /* rwx */
  enum NaClPhdrCheckAction  action;
  int                       required;  /* only for text for now */
  Elf_Addr                  p_vaddr;  /* if non-zero, vaddr must be this */
};

/*
 * Other than empty segments, these are the only ones that are allowed.
 */
static const struct NaClPhdrChecks nacl_phdr_check_data[] = {
  /* phdr */
  { PT_PHDR, PF_R, PCA_IGNORE, 0, 0, },
  /* text */
  { PT_LOAD, PF_R|PF_X, PCA_TEXT_CHECK, 1, NACL_TRAMPOLINE_END, },
  /* rodata */
  { PT_LOAD, PF_R, PCA_RODATA, 0, 0, },
  /* data/bss */
  { PT_LOAD, PF_R|PF_W, PCA_DATA, 0, 0, },
  /* tls */
  { PT_TLS, PF_R, PCA_IGNORE, 0, 0},
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  /* arm exception handling unwind info (for c++)*/
  /* TODO(robertm): for some reason this does NOT end up in ro maybe because
   *             it is relocatable. Try hacking the linker script to move it.
   */
  { PT_ARM_EXIDX, PF_R, PCA_IGNORE, 0, 0, },
#endif
  /*
   * allow optional GNU stack permission marker, but require that the
   * stack is non-executable.
   */
  { PT_GNU_STACK, PF_R|PF_W, PCA_NONE, 0, 0, },
  /* ignored segments */
  { PT_DYNAMIC, PF_R|PF_W, PCA_IGNORE, 0, 0},
  /*
   * PT_DYNAMIC with PF_R doesn't occur in practice, but leaving it here just
   * in case it has been used.
   */
  { PT_DYNAMIC, PF_R, PCA_IGNORE, 0, 0},
  { PT_INTERP, PF_R, PCA_IGNORE, 0, 0},
  { PT_NOTE, PF_R, PCA_IGNORE, 0, 0},
  { PT_GNU_EH_FRAME, PF_R, PCA_IGNORE, 0, 0},
  { PT_GNU_RELRO, PF_R, PCA_IGNORE, 0, 0},
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  { PT_MIPS_REGINFO, PF_R, PCA_IGNORE, 0, 0},
#endif
  { PT_NULL, PF_R, PCA_IGNORE, 0, 0},
};


static void NaClDumpElfHeader(int loglevel, Elf_Ehdr *elf_hdr) {

#define DUMP(m,f)    do { NaClLog(loglevel,                     \
                                  #m " = %" f "\n",             \
                                  elf_hdr->m); } while (0)

  NaClLog(loglevel, "=================================================\n");
  NaClLog(loglevel, "Elf header\n");
  NaClLog(loglevel, "==================================================\n");

  DUMP(e_ident+1, ".3s");
  DUMP(e_type, "#x");
  DUMP(e_machine, "#x");
  DUMP(e_version, "#x");
  DUMP(e_entry, "#"NACL_PRIxElf_Addr);
  DUMP(e_phoff, "#"NACL_PRIxElf_Off);
  DUMP(e_shoff, "#"NACL_PRIxElf_Off);
  DUMP(e_flags, "#"NACL_PRIxElf_Word);
  DUMP(e_ehsize, "#"NACL_PRIxElf_Half);
  DUMP(e_phentsize, "#"NACL_PRIxElf_Half);
  DUMP(e_phnum, "#"NACL_PRIxElf_Half);
  DUMP(e_shentsize, "#"NACL_PRIxElf_Half);
  DUMP(e_shnum, "#"NACL_PRIxElf_Half);
  DUMP(e_shstrndx, "#"NACL_PRIxElf_Half);
#undef DUMP
 NaClLog(loglevel, "sizeof(Elf32_Ehdr) = 0x%x\n", (int) sizeof *elf_hdr);
}


static void NaClDumpElfProgramHeader(int     loglevel,
                                     Elf_Phdr *phdr) {
#define DUMP(mem, f) do {                                \
    NaClLog(loglevel, "%s: %" f "\n", #mem, phdr->mem);  \
  } while (0)

  DUMP(p_type, NACL_PRIxElf_Word);
  DUMP(p_offset, NACL_PRIxElf_Off);
  DUMP(p_vaddr, NACL_PRIxElf_Addr);
  DUMP(p_paddr, NACL_PRIxElf_Addr);
  DUMP(p_filesz, NACL_PRIxElf_Xword);
  DUMP(p_memsz, NACL_PRIxElf_Xword);
  DUMP(p_flags, NACL_PRIxElf_Word);
  NaClLog(2, " (%s %s %s)\n",
          (phdr->p_flags & PF_R) ? "PF_R" : "",
          (phdr->p_flags & PF_W) ? "PF_W" : "",
          (phdr->p_flags & PF_X) ? "PF_X" : "");
  DUMP(p_align, NACL_PRIxElf_Xword);
#undef  DUMP
  NaClLog(loglevel, "\n");
}


NaClErrorCode NaClElfImageValidateElfHeader(struct NaClElfImage *image) {
  const Elf_Ehdr *hdr = &image->ehdr;

  if (memcmp(hdr->e_ident, ELFMAG, SELFMAG)) {
    NaClLog(LOG_ERROR, "bad elf magic\n");
    return LOAD_BAD_ELF_MAGIC;
  }

  if (ELFCLASS32 != hdr->e_ident[EI_CLASS]) {
    NaClLog(LOG_ERROR, "bad elf class\n");
    return LOAD_NOT_32_BIT;
  }

  if (ET_EXEC != hdr->e_type) {
    NaClLog(LOG_ERROR, "non executable\n");
    return LOAD_NOT_EXEC;
  }

  if (NACL_ELF_E_MACHINE != hdr->e_machine) {
    NaClLog(LOG_ERROR, "bad machine: %"NACL_PRIxElf_Half"\n", hdr->e_machine);
    return LOAD_BAD_MACHINE;
  }

  if (EV_CURRENT != hdr->e_version) {
    NaClLog(LOG_ERROR, "bad elf version: %"NACL_PRIxElf_Word"\n",
            hdr->e_version);
    return LOAD_BAD_ELF_VERS;
  }

  return LOAD_OK;
}

/* TODO(robertm): decouple validation from computation of
                   static_text_end and max_vaddr */
NaClErrorCode NaClElfImageValidateProgramHeaders(
  struct NaClElfImage *image,
  uint8_t             addr_bits,
  struct NaClElfImageInfo *info) {
    /*
     * Scan phdrs and do sanity checks in-line.  Verify that the load
     * address is NACL_TRAMPOLINE_END, that we have a single text
     * segment.  Data and TLS segments are not required, though it is
     * hard to avoid with standard tools, but in any case there should
     * be at most one each.  Ensure that no segment's vaddr is outside
     * of the address space.  Ensure that PT_GNU_STACK is present, and
     * that x is off.
     */
  const Elf_Ehdr      *hdr = &image->ehdr;
  int                 seen_seg[NACL_ARRAY_SIZE(nacl_phdr_check_data)];

  int                 segnum;
  const Elf_Phdr      *php;
  size_t              j;

  memset(info, 0, sizeof(*info));

  info->max_vaddr = NACL_TRAMPOLINE_END;

  /*
   * nacl_phdr_check_data is small, so O(|check_data| * nap->elf_hdr.e_phum)
   * is okay.
   */
  memset(seen_seg, 0, sizeof seen_seg);
  for (segnum = 0; segnum < hdr->e_phnum; ++segnum) {
    php = &image->phdrs[segnum];
    NaClLog(3, "Looking at segment %d, type 0x%x, p_flags 0x%x\n",
            segnum, php->p_type, php->p_flags);
    if (0 == php->p_memsz) {
      /*
       * We will not load this segment.
       */
      NaClLog(3, "Ignoring empty segment\n");
      continue;
    }

    for (j = 0; j < NACL_ARRAY_SIZE(nacl_phdr_check_data); ++j) {
      if (php->p_type == nacl_phdr_check_data[j].p_type
          && php->p_flags == nacl_phdr_check_data[j].p_flags)
        break;
    }
    if (j == NACL_ARRAY_SIZE(nacl_phdr_check_data)) {
      /* segment not in nacl_phdr_check_data */
      NaClLog(2,
              "Segment %d is of unexpected type 0x%x, flag 0x%x\n",
              segnum,
              php->p_type,
              php->p_flags);
      return LOAD_BAD_SEGMENT;
    }

    NaClLog(2, "Matched nacl_phdr_check_data[%"NACL_PRIuS"]\n", j);
    if (seen_seg[j]) {
      NaClLog(2, "Segment %d is a type that has been seen\n", segnum);
      return LOAD_DUP_SEGMENT;
    }
    seen_seg[j] = 1;

    if (PCA_IGNORE == nacl_phdr_check_data[j].action) {
      NaClLog(3, "Ignoring\n");
      continue;
    }

    /*
     * We will load this segment later.  Do the sanity checks.
     */
    if (0 != nacl_phdr_check_data[j].p_vaddr
        && (nacl_phdr_check_data[j].p_vaddr != php->p_vaddr)) {
      NaClLog(2,
              ("Segment %d: bad virtual address: 0x%08"
               NACL_PRIxElf_Addr","
               " expected 0x%08"NACL_PRIxElf_Addr"\n"),
              segnum,
              php->p_vaddr,
              nacl_phdr_check_data[j].p_vaddr);
      return LOAD_SEGMENT_BAD_LOC;
    }
    if (php->p_vaddr < NACL_TRAMPOLINE_END) {
      NaClLog(2,
              ("Segment %d: virtual address (0x%08"NACL_PRIxElf_Addr
               ") too low\n"),
              segnum,
              php->p_vaddr);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }
    if (php->p_vaddr >= ((uint64_t) 1U << addr_bits) ||
        ((uint64_t) 1U << addr_bits) - php->p_vaddr < php->p_memsz) {
      if (php->p_vaddr + php->p_memsz < php->p_vaddr) {
        NaClLog(2,
                "Segment %d: p_memsz caused integer overflow\n",
                segnum);
      } else {
        NaClLog(2,
                "Segment %d: too large, ends at 0x%08"NACL_PRIxElf_Addr"\n",
                segnum,
                php->p_vaddr + php->p_memsz);
      }
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }
    if (php->p_filesz > php->p_memsz) {
      NaClLog(2,
              ("Segment %d: file size 0x%08"NACL_PRIxElf_Xword" larger"
               " than memory size 0x%08"NACL_PRIxElf_Xword"\n"),
              segnum,
              php->p_filesz,
              php->p_memsz);
      return LOAD_SEGMENT_BAD_PARAM;
    }

    image->loadable[segnum] = 1;
    /* record our decision that we will load this segment */

    /*
     * NACL_TRAMPOLINE_END <= p_vaddr
     *                     <= p_vaddr + p_memsz
     *                     < ((uintptr_t) 1U << nap->addr_bits)
     */
    if (info->max_vaddr < php->p_vaddr + php->p_memsz) {
      info->max_vaddr = php->p_vaddr + php->p_memsz;
    }

    switch (nacl_phdr_check_data[j].action) {
      case PCA_NONE:
        break;
      case PCA_TEXT_CHECK:
        if (0 == php->p_memsz) {
          return LOAD_BAD_ELF_TEXT;
        }
        info->static_text_end = NACL_TRAMPOLINE_END + php->p_filesz;
        break;
      case PCA_RODATA:
        info->rodata_start = php->p_vaddr;
        info->rodata_end = php->p_vaddr + php->p_memsz;
        break;
      case PCA_DATA:
        info->data_start = php->p_vaddr;
        info->data_end = php->p_vaddr + php->p_memsz;
        break;
      case PCA_IGNORE:
        break;
    }
  }
  for (j = 0; j < NACL_ARRAY_SIZE(nacl_phdr_check_data); ++j) {
    if (nacl_phdr_check_data[j].required && !seen_seg[j]) {
      return LOAD_REQUIRED_SEG_MISSING;
    }
  }

  return LOAD_OK;
}



struct NaClElfImage *NaClElfImageNew(struct NaClDesc *ndp,
                                     NaClErrorCode *err_code) {
  ssize_t read_ret;
  struct NaClElfImage *result;
  struct NaClElfImage image;
  union {
    Elf32_Ehdr ehdr32;
#if NACL_BUILD_SUBARCH == 64
    Elf64_Ehdr ehdr64;
#endif
  } ehdr;
  int cur_ph;

  memset(image.loadable, 0, sizeof image.loadable);

  /*
   * We read the larger size of an ELFCLASS64 header even if it turns out
   * we're reading an ELFCLASS32 file.  No usable ELFCLASS32 binary could
   * be so small that it's not larger than Elf64_Ehdr anyway.
   */
  read_ret = (*NACL_VTBL(NaClDesc, ndp)->PRead)(ndp, &ehdr, sizeof ehdr, 0);
  if (NaClSSizeIsNegErrno(&read_ret) || (size_t) read_ret != sizeof ehdr) {
    *err_code = LOAD_READ_ERROR;
    NaClLog(2, "could not load elf headers\n");
    return 0;
  }

#if NACL_BUILD_SUBARCH == 64
  if (ELFCLASS64 == ehdr.ehdr64.e_ident[EI_CLASS]) {
    /*
     * Convert ELFCLASS64 format to ELFCLASS32 format.
     * The initial four fields are the same in both classes.
     */
    memcpy(image.ehdr.e_ident, ehdr.ehdr64.e_ident, EI_NIDENT);
    image.ehdr.e_ident[EI_CLASS] = ELFCLASS32;
    image.ehdr.e_type = ehdr.ehdr64.e_type;
    image.ehdr.e_machine = ehdr.ehdr64.e_machine;
    image.ehdr.e_version = ehdr.ehdr64.e_version;
    if (ehdr.ehdr64.e_entry > 0xffffffffU ||
        ehdr.ehdr64.e_phoff > 0xffffffffU ||
        ehdr.ehdr64.e_shoff > 0xffffffffU) {
      *err_code = LOAD_EHDR_OVERFLOW;
      NaClLog(2, "ELFCLASS64 file header fields overflow 32 bits\n");
      return 0;
    }
    image.ehdr.e_entry = (Elf32_Addr) ehdr.ehdr64.e_entry;
    image.ehdr.e_phoff = (Elf32_Off) ehdr.ehdr64.e_phoff;
    image.ehdr.e_shoff = (Elf32_Off) ehdr.ehdr64.e_shoff;
    image.ehdr.e_flags = ehdr.ehdr64.e_flags;
    if (ehdr.ehdr64.e_ehsize != sizeof(ehdr.ehdr64)) {
      *err_code = LOAD_BAD_EHSIZE;
      NaClLog(2, "ELFCLASS64 file e_ehsize != %d\n", (int) sizeof(ehdr.ehdr64));
      return 0;
    }
    image.ehdr.e_ehsize = sizeof(image.ehdr);
    image.ehdr.e_phentsize = sizeof(image.phdrs[0]);
    image.ehdr.e_phnum = ehdr.ehdr64.e_phnum;
    image.ehdr.e_shentsize = ehdr.ehdr64.e_shentsize;
    image.ehdr.e_shnum = ehdr.ehdr64.e_shnum;
    image.ehdr.e_shstrndx = ehdr.ehdr64.e_shstrndx;
  } else
#endif
  {
    image.ehdr = ehdr.ehdr32;
  }

  NaClDumpElfHeader(2, &image.ehdr);

  *err_code = NaClElfImageValidateElfHeader(&image);
  if (LOAD_OK != *err_code) {
    return 0;
  }

  /* read program headers */
  if (image.ehdr.e_phnum > NACL_MAX_PROGRAM_HEADERS) {
    *err_code = LOAD_TOO_MANY_PROG_HDRS;
    NaClLog(2, "too many prog headers\n");
    return 0;
  }

#if NACL_BUILD_SUBARCH == 64
  if (ELFCLASS64 == ehdr.ehdr64.e_ident[EI_CLASS]) {
    /*
     * We'll load the 64-bit phdrs and convert them to 32-bit format.
     */
    Elf64_Phdr phdr64[NACL_MAX_PROGRAM_HEADERS];

    if (ehdr.ehdr64.e_phentsize != sizeof(Elf64_Phdr)) {
      *err_code = LOAD_BAD_PHENTSIZE;
      NaClLog(2, "bad prog headers size\n");
      NaClLog(2, " ehdr64.e_phentsize = 0x%"NACL_PRIxElf_Half"\n",
              ehdr.ehdr64.e_phentsize);
      NaClLog(2, "  sizeof(Elf64_Phdr) = 0x%"NACL_PRIxS"\n",
              sizeof(Elf64_Phdr));
      return 0;
    }

    /*
     * We know the multiplication won't overflow since we rejected
     * e_phnum values larger than the small constant NACL_MAX_PROGRAM_HEADERS.
     */
    read_ret = (*NACL_VTBL(NaClDesc, ndp)->
                PRead)(ndp,
                       &phdr64[0],
                       image.ehdr.e_phnum * sizeof phdr64[0],
                       (nacl_off64_t) image.ehdr.e_phoff);
    if (NaClSSizeIsNegErrno(&read_ret) ||
        (size_t) read_ret != image.ehdr.e_phnum * sizeof phdr64[0]) {
      *err_code = LOAD_READ_ERROR;
      NaClLog(2, "cannot load tp prog headers\n");
      return 0;
    }

    for (cur_ph = 0; cur_ph < image.ehdr.e_phnum; ++cur_ph) {
      if (phdr64[cur_ph].p_offset > 0xffffffffU ||
          phdr64[cur_ph].p_vaddr > 0xffffffffU ||
          phdr64[cur_ph].p_paddr > 0xffffffffU ||
          phdr64[cur_ph].p_filesz > 0xffffffffU ||
          phdr64[cur_ph].p_memsz > 0xffffffffU ||
          phdr64[cur_ph].p_align > 0xffffffffU) {
        *err_code = LOAD_PHDR_OVERFLOW;
        NaClLog(2, "ELFCLASS64 program header fields overflow 32 bits\n");
        return 0;
      }
      image.phdrs[cur_ph].p_type = phdr64[cur_ph].p_type;
      image.phdrs[cur_ph].p_offset = (Elf32_Off) phdr64[cur_ph].p_offset;
      image.phdrs[cur_ph].p_vaddr = (Elf32_Addr) phdr64[cur_ph].p_vaddr;
      image.phdrs[cur_ph].p_paddr = (Elf32_Addr) phdr64[cur_ph].p_paddr;
      image.phdrs[cur_ph].p_filesz = (Elf32_Word) phdr64[cur_ph].p_filesz;
      image.phdrs[cur_ph].p_memsz = (Elf32_Word) phdr64[cur_ph].p_memsz;
      image.phdrs[cur_ph].p_flags = phdr64[cur_ph].p_flags;
      image.phdrs[cur_ph].p_align = (Elf32_Word) phdr64[cur_ph].p_align;
    }
  } else
#endif
  {
    if (image.ehdr.e_phentsize != sizeof image.phdrs[0]) {
      *err_code = LOAD_BAD_PHENTSIZE;
      NaClLog(2, "bad prog headers size\n");
      NaClLog(2, " image.ehdr.e_phentsize = 0x%"NACL_PRIxElf_Half"\n",
              image.ehdr.e_phentsize);
      NaClLog(2, "  sizeof image.phdrs[0] = 0x%"NACL_PRIxS"\n",
              sizeof image.phdrs[0]);
      return 0;
    }

    read_ret = (*NACL_VTBL(NaClDesc, ndp)->
                PRead)(ndp,
                       &image.phdrs[0],
                       image.ehdr.e_phnum * sizeof image.phdrs[0],
                       (nacl_off64_t) image.ehdr.e_phoff);
    if (NaClSSizeIsNegErrno(&read_ret) ||
        (size_t) read_ret != image.ehdr.e_phnum * sizeof image.phdrs[0]) {
      *err_code = LOAD_READ_ERROR;
      NaClLog(2, "cannot load tp prog headers\n");
      return 0;
    }
  }

  NaClLog(2, "=================================================\n");
  NaClLog(2, "Elf Program headers\n");
  NaClLog(2, "==================================================\n");
  for (cur_ph = 0; cur_ph <  image.ehdr.e_phnum; ++cur_ph) {
    NaClDumpElfProgramHeader(2, &image.phdrs[cur_ph]);
  }

  /* we delay allocating till the end to avoid cleanup code */
  result = malloc(sizeof image);
  if (result == 0) {
    *err_code = LOAD_NO_MEMORY;
    NaClLog(LOG_FATAL, "no enough memory for image meta data\n");
    return 0;
  }
  memcpy(result, &image, sizeof image);
  *err_code = LOAD_OK;
  return result;
}

/*
 * Attempt to map into the NaClApp object nap from the NaCl descriptor
 * ndp an ELF segment of type p_flags that start at file_offset for
 * segment_size bytes, to memory starting at paddr (system address).
 * If it is a code segment, make a scratch mapping and check
 * validation in readonly_text mode -- if it succeeds, we map into the
 * target address; if it fails, we return failure so that pread-based
 * loading can proceed.  For rodata and data segments, less checking
 * is needed.  In the text and data case, the end of the segment may
 * not land on a NACL_MAP_PAGESIZE boundary; when this occurs, we will
 * map in all whole NACL_MAP_PAGESIZE chunks, and pread in the tail
 * partial chunk.
 *
 * Returns: LOAD_OK, LOAD_STATUS_UNKNOWN, other error codes.
 *
 * LOAD_OK             -- if the segment has been fully handled
 * LOAD_STATUS_UNKNOWN -- if pread-based fallback is required
 * other error codes   -- if a fatal error occurs, and the caller
 *                        should propagate up
 *
 * See NaClSysMmapIntern in nacl_syscall_common.c for corresponding
 * mmap syscall where PROT_EXEC allows shared libraries to be mapped
 * into dynamic code space.
 */
static NaClErrorCode NaClElfFileMapSegment(struct NaClApp *nap,
                                           struct NaClDesc *ndp,
                                           Elf_Word p_flags,
                                           Elf_Off file_offset,
                                           Elf_Off segment_size,
                                           uintptr_t vaddr,
                                           uintptr_t paddr) {
  size_t rounded_filesz;       /* 64k rounded */
  int mmap_prot = 0;
  uintptr_t image_sys_addr;
  NaClValidationStatus validator_status = NaClValidationFailed;
  struct NaClValidationMetadata metadata;
  int read_last_page_if_partial_allocation_page = 1;
  ssize_t read_ret;
  struct NaClPerfCounter time_mmap_segment;
  NaClPerfCounterCtor(&time_mmap_segment, "NaClElfFileMapSegment");

  rounded_filesz = NaClRoundAllocPage(segment_size);

  NaClLog(4,
          "NaClElfFileMapSegment: checking segment flags 0x%x"
          " to determine map checks\n",
          p_flags);
  /*
   * Is this the text segment?  If so, map into scratch memory and
   * run validation (possibly cached result) with !stubout_mode,
   * readonly_text.  If validator says it's okay, map directly into
   * target location with NACL_ABI_PROT_READ|_EXEC.  If anything
   * failed, fall back to PRead.  NB: the assumption is that there
   * is only one PT_LOAD with PF_R|PF_X segment; this assumption is
   * enforced by phdr seen_seg checks above in
   * NaClElfImageValidateProgramHeaders.
   *
   * After this function returns, we will be setting memory protection
   * in NaClMemoryProtection, so the actual memory protection used is
   * immaterial.
   *
   * For rodata and data/bss, we mmap with NACL_ABI_PROT_READ or
   * NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE as appropriate,
   * without doing validation.  There is no fallback to PRead, since
   * we don't validate the contents.
   */
  switch (p_flags) {
    case PF_R | PF_X:
      NaClLog(4,
              "NaClElfFileMapSegment: text segment and"
              " file is safe for mmap\n");
      if (NACL_VTBL(NaClDesc, ndp)->typeTag != NACL_DESC_HOST_IO) {
        NaClLog(4, "NaClElfFileMapSegment: not supported type, got %d\n",
                NACL_VTBL(NaClDesc, ndp)->typeTag);
        return LOAD_STATUS_UNKNOWN;
      }
      /*
       * Unlike the mmap case, we do not re-run validation to
       * allow patching here; instead, we handle validation
       * failure by going to the pread_fallback case.  In the
       * future, we should consider doing an in-place mapping and
       * allowing HLT patch validation, which should be cheaper
       * since those pages that do not require patching (hopefully
       * majority) will remain file-backed and not require swap
       * space, even if we had to fault in every page.
       */
      NaClLog(1, "NaClElfFileMapSegment: mapping for validation\n");
      NaClPerfCounterMark(&time_mmap_segment, "PreMap");
      NaClPerfCounterIntervalLast(&time_mmap_segment);
      image_sys_addr = (*NACL_VTBL(NaClDesc, ndp)->
                        Map)(ndp,
                             NaClDescEffectorTrustedMem(),
                             (void *) NULL,
                             rounded_filesz,
                             NACL_ABI_PROT_READ,
                             NACL_ABI_MAP_PRIVATE,
                             file_offset);
      NaClPerfCounterMark(&time_mmap_segment, "MapForValidate");
      NaClPerfCounterIntervalLast(&time_mmap_segment);
      if (NaClPtrIsNegErrno(&image_sys_addr)) {
        NaClLog(LOG_INFO,
                "NaClElfFileMapSegment: Could not make scratch mapping,"
                " falling back to reading\n");
        return LOAD_STATUS_UNKNOWN;
      }
      /* ask validator / validation cache */
      NaClMetadataFromNaClDescCtor(&metadata, ndp);
      CHECK(segment_size == nap->static_text_end - NACL_TRAMPOLINE_END);
      validator_status = NACL_FI_VAL(
          "ELF_LOAD_FORCE_VALIDATION_STATUS",
          enum NaClValidationStatus,
          (*nap->validator->
           Validate)(vaddr,
                     (uint8_t *) image_sys_addr,
                     segment_size,  /* actual size */
                     0,  /* stubout_mode: no */
                     nap->pnacl_mode ? NACL_DISABLE_NONTEMPORALS_X86 : 0,
                     1,  /* readonly_text: yes */
                     nap->cpu_features,
                     &metadata,
                     nap->validation_cache));
      NaClPerfCounterMark(&time_mmap_segment, "ValidateMapped");
      NaClPerfCounterIntervalLast(&time_mmap_segment);
      NaClLog(3, "NaClElfFileMapSegment: validator_status %d\n",
              validator_status);
      NaClMetadataDtor(&metadata);
      /*
       * Remove scratch mapping, then map directly into untrusted
       * address space or pread.
       */
      NaClHostDescUnmapUnsafe((void *) image_sys_addr, rounded_filesz);
      NACL_MAKE_MEM_UNDEFINED((void *) paddr, rounded_filesz);

      if (NaClValidationSucceeded != validator_status) {
        NaClLog(3,
                ("NaClElfFileMapSegment: readonly_text validation for mmap"
                 " failed.  Will retry validation allowing HALT stubbing out"
                 " of unsupported instruction extensions.\n"));
        return LOAD_STATUS_UNKNOWN;
      }

      NaClLog(1, "NaClElfFileMapSegment: mapping into code space\n");
      /*
       * Windows appears to not allow RWX mappings.  This interferes
       * with HALT_SLED and having to HALT pad the last page.  We
       * allow partial code pages, so
       * read_last_page_if_partial_allocation_page will ensure that
       * the last page is writable, so we will be able to write HALT
       * instructions as needed.
       */
      mmap_prot = NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC;
      /*
       * NB: the log string is used by tests/mmap_main_nexe/nacl.scons
       * and must be logged at a level that is less than or equal to
       * the requested verbosity level there.
       */
      NaClLog(1, "NaClElfFileMapSegment: EXERCISING MMAP LOAD PATH\n");
      nap->main_exe_prevalidated = 1;
      break;

    case PF_R | PF_W:
      /* read-write (initialized data) */
      mmap_prot = NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE;
      /*
       * NB: the partial page processing will result in zeros
       * following the initialized data, so that the BSS will be zero.
       * On a typical system, this page is mapped in and the BSS
       * region is memset to zero, which means that this partial page
       * is faulted in.  Rather than saving a syscall (pread) and
       * faulting it in, we just use the same code path as for code,
       * which is (slightly) simpler.
       */
      break;

    case PF_R:
      /* read-only */
      mmap_prot = NACL_ABI_PROT_READ;
      /*
       * For rodata, we allow mapping in "garbage" past a partial
       * page; this potentially eliminates a disk I/O operation
       * (if data section has no partial page), possibly delaying
       * disk spin-up if the code was in the validation cache.
       * And it saves another 64kB of swap.
       */
      read_last_page_if_partial_allocation_page = 0;
      break;

    default:
      NaClLog(LOG_FATAL, "NaClElfFileMapSegment: unexpected p_flags %d\n",
              p_flags);
  }
  if (rounded_filesz != segment_size &&
      read_last_page_if_partial_allocation_page) {
    uintptr_t tail_offset = rounded_filesz - NACL_MAP_PAGESIZE;
    size_t tail_size = segment_size - tail_offset;
    NaClLog(4, "NaClElfFileMapSegment: pread tail\n");
    read_ret = (*NACL_VTBL(NaClDesc, ndp)->
                PRead)(ndp,
                       (void *) (paddr + tail_offset),
                       tail_size,
                       (nacl_off64_t) (file_offset + tail_offset));
    NaClPerfCounterMark(&time_mmap_segment, "PRead tail");
    NaClPerfCounterIntervalLast(&time_mmap_segment);
    if (NaClSSizeIsNegErrno(&read_ret) || (size_t) read_ret != tail_size) {
      NaClLog(LOG_ERROR,
              "NaClElfFileMapSegment: pread load of page tail failed\n");
      return LOAD_SEGMENT_BAD_PARAM;
    }
    rounded_filesz -= NACL_MAP_PAGESIZE;
  }
  /* mmap in */
  if (rounded_filesz == 0) {
    NaClLog(4,
            "NaClElfFileMapSegment: no pages to map, probably because"
            " the segment was a partial page, so it was processed by"
            " reading.\n");
  } else {
    NaClLog(4,
            "NaClElfFileMapSegment: mapping %"NACL_PRIuS" (0x%"
            NACL_PRIxS") bytes to"
            " address 0x%"NACL_PRIxPTR", position %"
            NACL_PRIdElf_Off" (0x%"NACL_PRIxElf_Off")\n",
            rounded_filesz, rounded_filesz,
            paddr,
            file_offset, file_offset);
    image_sys_addr = (*NACL_VTBL(NaClDesc, ndp)->
                      Map)(ndp,
                           nap->effp,
                           (void *) paddr,
                           rounded_filesz,
                           mmap_prot,
                           NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_FIXED,
                           file_offset);
    NaClPerfCounterMark(&time_mmap_segment, "MapFinal");
    NaClPerfCounterIntervalLast(&time_mmap_segment);
    if (image_sys_addr != paddr) {
      NaClLog(LOG_FATAL,
              ("NaClElfFileMapSegment: map to 0x%"NACL_PRIxPTR" (prot %x) "
               "failed: got 0x%"NACL_PRIxPTR"\n"),
              paddr, mmap_prot, image_sys_addr);
    }
    /* Tell Valgrind that we've mapped a segment of nacl_file. */
    NaClFileMappingForValgrind(paddr, rounded_filesz, file_offset);
  }
  return LOAD_OK;
}

NaClErrorCode NaClElfImageLoad(struct NaClElfImage *image,
                               struct NaClDesc *ndp,
                               struct NaClApp *nap) {
  int segnum;
  uintptr_t vaddr;
  uintptr_t paddr;
  uintptr_t end_vaddr;
  ssize_t read_ret;
  int safe_for_mmap;

  for (segnum = 0; segnum < image->ehdr.e_phnum; ++segnum) {
    const Elf_Phdr *php = &image->phdrs[segnum];
    Elf_Off offset = (Elf_Off) NaClTruncAllocPage(php->p_offset);
    Elf_Off filesz = php->p_offset + php->p_filesz - offset;

    /* did we decide that we will load this segment earlier? */
    if (!image->loadable[segnum]) {
      continue;
    }

    NaClLog(2, "loading segment %d\n", segnum);

    if (0 == php->p_filesz) {
      NaClLog(4, "zero-sized segment.  ignoring...\n");
      continue;
    }

    end_vaddr = php->p_vaddr + php->p_filesz;
    /* integer overflow? */
    if (end_vaddr < php->p_vaddr) {
      NaClLog(LOG_FATAL, "parameter error should have been detected already\n");
    }
    /*
     * is the end virtual address within the NaCl application's
     * address space?  if it is, it implies that the start virtual
     * address is also.
     */
    if (end_vaddr >= ((uintptr_t) 1U << nap->addr_bits)) {
      NaClLog(LOG_FATAL, "parameter error should have been detected already\n");
    }

    vaddr = NaClTruncAllocPage(php->p_vaddr);
    paddr = NaClUserToSysAddr(nap, vaddr);
    CHECK(kNaClBadAddress != paddr);

    /*
     * Check NaClDescIsSafeForMmap(ndp) to see if it might be okay to
     * mmap.
     */
    NaClLog(4, "NaClElfImageLoad: checking descriptor mmap safety\n");
    safe_for_mmap = NaClDescIsSafeForMmap(ndp);
    if (safe_for_mmap) {
      NaClLog(4, "NaClElfImageLoad: safe-for-mmap\n");
    }

    if (!safe_for_mmap &&
        NACL_FI("ELF_LOAD_BYPASS_DESCRIPTOR_SAFETY_CHECK", 0, 1)) {
      NaClLog(LOG_WARNING, "WARNING: BYPASSING DESCRIPTOR SAFETY CHECK\n");
      safe_for_mmap = 1;
    }
    if (safe_for_mmap) {
      NaClErrorCode map_status;
      NaClLog(4, "NaClElfImageLoad: safe-for-mmap\n");
      map_status = NaClElfFileMapSegment(nap, ndp, php->p_flags,
                                         offset, filesz, vaddr, paddr);
      /*
       * NB: -Werror=switch-enum forces us to not use a switch.
       */
      if (LOAD_OK == map_status) {
        /* Segment has been handled -- proceed to next segment */
        continue;
      } else if (LOAD_STATUS_UNKNOWN != map_status) {
        /*
         * A real error!  Return it so that this can be reported to
         * the embedding code (via start_module status).
         */
        return map_status;
      }
      /* Fall through: pread-based fallback requested */
    }
    NaClLog(4,
            "PReading %"NACL_PRIdElf_Xword" (0x%"NACL_PRIxElf_Xword") bytes to"
            " address 0x%"NACL_PRIxPTR", position %"
            NACL_PRIdElf_Off" (0x%"NACL_PRIxElf_Off")\n",
            filesz, filesz,
            paddr,
            offset, offset);

    /*
     * Tell valgrind that this memory is accessible and undefined. For more
     * details see
     * http://code.google.com/p/nativeclient/wiki/ValgrindMemcheck#Implementation_details
     */
    NACL_MAKE_MEM_UNDEFINED((void *) paddr, filesz);

    read_ret = (*NACL_VTBL(NaClDesc, ndp)->
                PRead)(ndp, (void *) paddr, filesz, (nacl_off64_t) offset);
    if (NaClSSizeIsNegErrno(&read_ret) || (size_t) read_ret != filesz) {
      NaClLog(LOG_ERROR, "load failure segment %d", segnum);
      return LOAD_SEGMENT_BAD_PARAM;
    }
    /* region from p_filesz to p_memsz should already be zero filled */

    /* Tell Valgrind that we've mapped a segment of nacl_file. */
    NaClFileMappingForValgrind(paddr, filesz, offset);
  }

  return LOAD_OK;
}


NaClErrorCode NaClElfImageLoadDynamically(
    struct NaClElfImage *image,
    struct NaClApp *nap,
    struct NaClDesc *ndp,
    struct NaClValidationMetadata *metadata) {
  ssize_t read_ret;
  int segnum;
  for (segnum = 0; segnum < image->ehdr.e_phnum; ++segnum) {
    const Elf_Phdr *php = &image->phdrs[segnum];
    Elf_Addr vaddr = php->p_vaddr & ~(NACL_MAP_PAGESIZE - 1);
    Elf_Off offset = php->p_offset & ~(NACL_MAP_PAGESIZE - 1);
    Elf_Off filesz = php->p_offset + php->p_filesz - offset;
    Elf_Off memsz = php->p_offset + php->p_memsz - offset;
    int32_t result;

    /*
     * By checking if filesz is larger than memsz, we no longer run the risk of
     * a malicious ELF object overrunning into the trusted address space when
     * reading data of size "filez" into a buffer of size "memsz".
     */
    if (filesz > memsz) {
      return LOAD_UNLOADABLE;
    }

    /*
     * We check for PT_LOAD directly rather than using the "loadable"
     * array because we are not using NaClElfImageValidateProgramHeaders()
     * to fill out the "loadable" array for this ELF object.  This ELF
     * object does not have to fit such strict constraints (such as
     * having code at 0x20000), and safety checks are applied by
     * NaClTextDyncodeCreate() and NaClSysMmapIntern().
     */
    if (PT_LOAD != php->p_type) {
      continue;
    }

    if (0 != (php->p_flags & PF_X)) {
      /* Load code segment. */
      /*
       * We make a copy of the code.  This is not ideal given that this
       * code path is used only for loading the IRT, and we could assume
       * that the contents of the irt.nexe file will not change underneath
       * us.  We should be able to mmap() the IRT's code segment instead of
       * copying it.
       * TODO(mseaborn): Reduce the amount of copying here.
       */
      char *code_copy = malloc(filesz);
      if (NULL == code_copy) {
        NaClLog(LOG_ERROR, "NaClElfImageLoadDynamically: malloc failed\n");
        return LOAD_NO_MEMORY;
      }
      read_ret = (*NACL_VTBL(NaClDesc, ndp)->
                  PRead)(ndp, code_copy, filesz, (nacl_off64_t) offset);
      if (NaClSSizeIsNegErrno(&read_ret) ||
          (size_t) read_ret != filesz) {
        free(code_copy);
        NaClLog(LOG_ERROR, "NaClElfImageLoadDynamically: "
                "failed to read code segment\n");
        return LOAD_READ_ERROR;
      }
      if (NULL != metadata) {
        metadata->code_offset = offset;
      }
      result = NaClTextDyncodeCreate(nap, (uint32_t) vaddr,
                                     code_copy, (uint32_t) filesz, metadata);
      free(code_copy);
      if (0 != result) {
        NaClLog(LOG_ERROR, "NaClElfImageLoadDynamically: "
                "failed to load code segment\n");
        return LOAD_UNLOADABLE;
      }
    } else {
      /* Load data segment. */
      void *paddr = (void *) NaClUserToSys(nap, vaddr);
      size_t mapping_size = NaClRoundAllocPage(memsz);
      /*
       * Note that we do not used NACL_ABI_MAP_FIXED because we do not
       * want to silently overwrite any existing mappings, such as the
       * user app's data segment or the stack.  We detect overmapping
       * when mmap chooses not to use the preferred address we supply.
       * (Ideally mmap would provide a MAP_EXCL option for this
       * instead.)
       */
      result = NaClSysMmapIntern(
          nap, (void *) (uintptr_t) vaddr, mapping_size,
          NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
          NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
          -1, 0);
      if ((int32_t) vaddr != result) {
        NaClLog(LOG_ERROR, "NaClElfImageLoadDynamically: "
                "failed to map data segment\n");
        return LOAD_UNLOADABLE;
      }
      read_ret = (*NACL_VTBL(NaClDesc, ndp)->
                  PRead)(ndp, paddr, filesz, (nacl_off64_t) offset);
      if (NaClSSizeIsNegErrno(&read_ret) ||
          (size_t) read_ret != filesz) {
        NaClLog(LOG_ERROR, "NaClElfImageLoadDynamically: "
                "failed to read data segment\n");
        return LOAD_READ_ERROR;
      }
      /*
       * Note that we do not need to zero the BSS (the region from
       * p_filesz to p_memsz) because it should already be zero
       * filled.  This would not be the case if we were mapping the
       * data segment from the file.
       */

      if (0 == (php->p_flags & PF_W)) {
        /* Handle read-only data segment. */
        int rc = NaClMprotect(paddr, mapping_size, NACL_ABI_PROT_READ);
        if (0 != rc) {
          NaClLog(LOG_ERROR, "NaClElfImageLoadDynamically: "
                  "failed to mprotect read-only data segment\n");
          return LOAD_MPROTECT_FAIL;
        }

        NaClVmmapAddWithOverwrite(&nap->mem_map,
                                  vaddr >> NACL_PAGESHIFT,
                                  mapping_size >> NACL_PAGESHIFT,
                                  NACL_ABI_PROT_READ,
                                  NACL_ABI_MAP_PRIVATE,
                                  NULL,
                                  0,
                                  0);
      }
    }
  }
  return LOAD_OK;
}

void NaClElfImageDelete(struct NaClElfImage *image) {
  free(image);
}


uintptr_t NaClElfImageGetEntryPoint(struct NaClElfImage *image) {
  return image->ehdr.e_entry;
}
