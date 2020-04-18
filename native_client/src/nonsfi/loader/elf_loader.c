/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/elf.h"
#include "native_client/src/public/nonsfi/elf_loader.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#define ElfW(type) Elf32_ ## type

/*
 * Note that Non-SFI NaCl uses a 4k page size (in order to allow loading
 * existing Linux DSOs), in contrast to SFI NaCl's 64k page size (required
 * for running on Windows).
 */
#define NONSFI_PAGE_SIZE 0x1000
#define NONSFI_PAGE_MASK (NONSFI_PAGE_SIZE - 1)
#define MAX_PHNUM 128

static uintptr_t PageSizeRoundDown(uintptr_t addr) {
  return addr & ~NONSFI_PAGE_MASK;
}

static uintptr_t PageSizeRoundUp(uintptr_t addr) {
  return PageSizeRoundDown(addr + NONSFI_PAGE_SIZE - 1);
}

static int ElfFlagsToMmapFlags(int pflags) {
  return ((pflags & PF_X) != 0 ? PROT_EXEC : 0) |
         ((pflags & PF_R) != 0 ? PROT_READ : 0) |
         ((pflags & PF_W) != 0 ? PROT_WRITE : 0);
}

static void CheckElfHeaders(ElfW(Ehdr) *ehdr) {
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
    NaClLog(LOG_FATAL, "Not an ELF file: no ELF header\n");
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
    NaClLog(LOG_FATAL, "Unexpected ELF class: not ELFCLASS32\n");
  }
  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    NaClLog(LOG_FATAL, "Not a little-endian ELF file\n");
  }
  if (ehdr->e_type != ET_DYN) {
    NaClLog(LOG_FATAL, "Not a relocatable ELF object (not ET_DYN)\n");
  }
  if (ehdr->e_machine != NACL_ELF_E_MACHINE) {
    NaClLog(LOG_FATAL, "Unexpected ELF e_machine field\n");
  }
  if (ehdr->e_version != EV_CURRENT) {
    NaClLog(LOG_FATAL, "Unexpected ELF e_version field\n");
  }
  if (ehdr->e_ehsize != sizeof(*ehdr)) {
    NaClLog(LOG_FATAL, "Unexpected ELF e_ehsize field\n");
  }
  if (ehdr->e_phentsize != sizeof(ElfW(Phdr))) {
    NaClLog(LOG_FATAL, "Unexpected ELF e_phentsize field\n");
  }
}

uintptr_t NaClLoadElfFile(int fd) {
  /* Read ELF file headers. */
  ElfW(Ehdr) ehdr;
  ssize_t bytes_read = pread(fd, &ehdr, sizeof(ehdr), 0);
  if (bytes_read != sizeof(ehdr)) {
    NaClLog(LOG_FATAL, "Failed to read ELF file headers\n");
  }
  CheckElfHeaders(&ehdr);

  /* Read ELF program headers. */
  if (ehdr.e_phnum > MAX_PHNUM) {
    NaClLog(LOG_FATAL, "ELF file has too many program headers\n");
  }
  ElfW(Phdr) phdr[MAX_PHNUM];
  ssize_t phdrs_size = sizeof(phdr[0]) * ehdr.e_phnum;
  bytes_read = pread(fd, phdr, phdrs_size, ehdr.e_phoff);
  if (bytes_read != phdrs_size) {
    NaClLog(LOG_FATAL, "Failed to read ELF program headers\n");
  }

  /* Find the first PT_LOAD segment. */
  size_t phdr_index = 0;
  while (phdr_index < ehdr.e_phnum && phdr[phdr_index].p_type != PT_LOAD)
    ++phdr_index;
  if (phdr_index == ehdr.e_phnum) {
    NaClLog(LOG_FATAL, "ELF file has no PT_LOAD header\n");
  }

  /*
   * ELF requires that PT_LOAD segments be in ascending order of p_vaddr.
   * Find the last one to calculate the whole address span of the image.
   */
  ElfW(Phdr) *first_load = &phdr[phdr_index];
  ElfW(Phdr) *last_load = &phdr[ehdr.e_phnum - 1];
  while (last_load > first_load && last_load->p_type != PT_LOAD)
    --last_load;

  if (first_load->p_vaddr != 0) {
    NaClLog(LOG_FATAL, "First PT_LOAD segment's load address is not 0\n");
  }
  size_t span = last_load->p_vaddr + last_load->p_memsz;

  /* Reserve address space. */
  void *mapping = mmap(NULL, span, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (mapping == MAP_FAILED) {
    NaClLog(LOG_FATAL, "Failed to reserve address space for executable\n");
  }
  uintptr_t load_bias = (uintptr_t) mapping;

  /* Map the PT_LOAD segments. */
  uintptr_t prev_segment_end = 0;
  int entry_point_is_valid = 0;
  ElfW(Phdr) *ph;
  for (ph = first_load; ph <= last_load; ++ph) {
    if (ph->p_type != PT_LOAD)
      continue;
    int prot = ElfFlagsToMmapFlags(ph->p_flags);
    uintptr_t segment_start = PageSizeRoundDown(ph->p_vaddr);
    uintptr_t segment_end = PageSizeRoundUp(ph->p_vaddr + ph->p_memsz);
    if (segment_start < prev_segment_end) {
      NaClLog(LOG_FATAL, "PT_LOAD segments overlap or are not sorted\n");
    }
    prev_segment_end = segment_end;
    void *segment_addr = (void *) (load_bias + segment_start);
    void *map_result = mmap((void *) segment_addr,
                            segment_end - segment_start,
                            prot, MAP_PRIVATE | MAP_FIXED, fd,
                            PageSizeRoundDown(ph->p_offset));
    if (map_result != segment_addr) {
      NaClLog(LOG_FATAL, "Failed to map ELF segment\n");
    }

    if ((ph->p_flags & PF_X) != 0 &&
        ph->p_vaddr <= ehdr.e_entry &&
        ehdr.e_entry < ph->p_vaddr + ph->p_filesz) {
      entry_point_is_valid = 1;
    }

    /* Handle the BSS. */
    if (ph->p_memsz < ph->p_filesz) {
      NaClLog(LOG_FATAL, "Bad ELF segment: p_memsz < p_filesz\n");
    }
    if (ph->p_memsz > ph->p_filesz) {
      if ((ph->p_flags & PF_W) == 0) {
        NaClLog(LOG_FATAL,
                "Bad ELF segment: non-writable segment with BSS\n");
      }

      uintptr_t bss_start = ph->p_vaddr + ph->p_filesz;
      uintptr_t bss_map_start = PageSizeRoundUp(bss_start);
      /*
       * Zero the BSS to the end of the page.
       *
       * Zeroing beyond p_memsz might be more than is necessary for Non-SFI
       * NaCl.  On Linux, programs such as ld.so use the rest of the page,
       * after p_memsz, as part of the brk() heap and assume that it has
       * been zeroed.  Non-SFI NaCl does not provide a brk() heap, though.
       * However, zeroing to the end of the page is simple enough, and it's
       * consistent with the case in additional pages must be mapped, which
       * will all be fully zeroed.
       */
      memset((void *) (load_bias + bss_start), 0, bss_map_start - bss_start);

      if (bss_map_start < segment_end) {
        void *map_addr = (void *) (load_bias + bss_map_start);
        map_result = mmap(map_addr, segment_end - bss_map_start,
                          prot, MAP_PRIVATE | MAP_ANON | MAP_FIXED,
                          -1, 0);
        if (map_result != map_addr) {
          NaClLog(LOG_FATAL, "Failed to map BSS for ELF segment\n");
        }
      }
    }
  }

  if (close(fd) != 0) {
    NaClLog(LOG_FATAL, "close() failed\n");
  }

  if (!entry_point_is_valid) {
    NaClLog(LOG_FATAL, "ELF entry point does not point into an executable "
            "PT_LOAD segment\n");
  }
  return load_bias + ehdr.e_entry;
}
