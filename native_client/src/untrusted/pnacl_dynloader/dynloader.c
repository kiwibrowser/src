/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/pnacl_dynloader/dynloader.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/elf.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"


/* Only need to handle PHDRs produced by the PNaCl translator's linker. */
#define MAX_PHNUM 20

#if !defined(__native_client_nonsfi__)
static struct nacl_irt_code_data_alloc irt_alloc_interface;

static void init_irt_alloc_interface(void) {
  /*
   * We don't check the result here because __libnacl_irt_init_fn()
   * rechecks the IRT interface struct.
   */
  nacl_interface_query(NACL_IRT_CODE_DATA_ALLOC_v0_1, &irt_alloc_interface,
                       sizeof(irt_alloc_interface));
}

static int allocate_code_data(uintptr_t hint, size_t code_size,
                              uintptr_t data_offset, size_t data_size,
                              uintptr_t *begin) {
  if (!__libnacl_irt_init_fn(&irt_alloc_interface.allocate_code_data,
                             init_irt_alloc_interface)) {
    return ENOSYS;
  }
  return irt_alloc_interface.allocate_code_data(hint, code_size, data_offset,
                                                data_size, begin);
}
#endif

static uintptr_t page_size_round_down(uintptr_t addr) {
  return addr & ~(getpagesize() - 1);
}

static uintptr_t page_size_round_up(uintptr_t addr) {
  return page_size_round_down(addr + getpagesize() - 1);
}

static int elf_flags_to_mmap_flags(int pflags) {
  return ((pflags & PF_X) != 0 ? PROT_EXEC : 0) |
         ((pflags & PF_R) != 0 ? PROT_READ : 0) |
         ((pflags & PF_W) != 0 ? PROT_WRITE : 0);
}

static void check_elf_headers(Elf32_Ehdr *ehdr) {
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
    NaClLog(LOG_FATAL, "Not an ELF file: no ELF header\n");
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
    NaClLog(LOG_FATAL, "Unexpected ELF class; not ELFCLASS32: ei_class=0x%x\n",
            ehdr->e_ident[EI_CLASS]);
  }
  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    NaClLog(LOG_FATAL, "Not a little-endian ELF file: ei_data=0x%x\n",
            ehdr->e_ident[EI_DATA]);
  }
  if (ehdr->e_type != ET_DYN) {
    NaClLog(LOG_FATAL,
            "Not a relocatable ELF object (not ET_DYN): e_type=0x%x\n",
            ehdr->e_type);
  }
  if (ehdr->e_machine != NACL_ELF_E_MACHINE) {
    NaClLog(LOG_FATAL, "Unexpected ELF header value: e_machine=0x%x\n",
            ehdr->e_machine);
  }
  if (ehdr->e_version != EV_CURRENT) {
    NaClLog(LOG_FATAL, "Unexpected ELF header value: e_version=0x%x\n",
            ehdr->e_version);
  }
  if (ehdr->e_ehsize != sizeof(*ehdr)) {
    NaClLog(LOG_FATAL, "Unexpected ELF header value: e_ehsize=0x%x\n",
            ehdr->e_ehsize);
  }
  if (ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
    NaClLog(LOG_FATAL, "Unexpected ELF header value: e_phentsize=0x%x\n",
            ehdr->e_phentsize);
  }
}

static bool find_dt_entry(Elf32_Dyn *dyn, int dt_tag, uintptr_t *result) {
  for (; dyn->d_tag != DT_NULL; dyn++) {
    if (dyn->d_tag == dt_tag) {
      *result = dyn->d_un.d_val;
      return true;
    }
  }
  return false;
}

static void apply_relocations(Elf32_Dyn *dyn, uintptr_t load_bias) {
  uintptr_t rel_offset;
  uintptr_t rel_size;
  bool found_dt_rel = find_dt_entry(dyn, DT_REL, &rel_offset);
  bool found_dt_relsz = find_dt_entry(dyn, DT_RELSZ, &rel_size);
  if (!found_dt_rel && !found_dt_relsz) {
    /* Nothing to do.  A very simple DSO might have no relocations. */
    return;
  }
  if (!found_dt_rel || !found_dt_relsz) {
    NaClLog(LOG_FATAL, "Malformed ELF object: DT_REL or DT_RELSZ missing\n");
  }
  const Elf32_Rel *rels = (const Elf32_Rel *) (load_bias + rel_offset);
  const Elf32_Rel *rels_end = (const Elf32_Rel *) (load_bias + rel_offset
                                                   + rel_size);
  for (; rels < rels_end; rels++) {
    uint32_t reloc_type = ELF32_R_TYPE(rels->r_info);
    uintptr_t addr = load_bias + rels->r_offset;
    switch (reloc_type) {
#if defined(__i386__)
      case R_386_RELATIVE:
#elif defined(__arm__)
      case R_ARM_RELATIVE:
#else
# error Unhandled architecture
#endif
        /*
         * Note that |addr| might be not 4-byte-aligned, but that is OK
         * because PNaCl allows misaligned accesses.
         */
        *(uint32_t *) addr += load_bias;
        break;
      default:
        NaClLog(LOG_FATAL, "Unhandled relocation type: %d\n", reloc_type);
    }
  }
}

static int load_elf_file_from_fd(int fd, void **pso_root) {
  /* Read ELF file headers. */
  Elf32_Ehdr ehdr;
  ssize_t bytes_read = pread(fd, &ehdr, sizeof(ehdr), 0);
  if (bytes_read != sizeof(ehdr)) {
    NaClLog(LOG_ERROR, "Failed to read ELF file headers: errno=%d\n", errno);
    return errno;
  }
  check_elf_headers(&ehdr);

  /* Read ELF program headers. */
  if (ehdr.e_phnum > MAX_PHNUM) {
    NaClLog(LOG_ERROR, "ELF file has too many program headers: e_phnum=%d\n",
            ehdr.e_phnum);
    return EIO;
  }
  Elf32_Phdr phdr[MAX_PHNUM];
  ssize_t phdrs_size = sizeof(phdr[0]) * ehdr.e_phnum;
  bytes_read = pread(fd, phdr, phdrs_size, ehdr.e_phoff);
  if (bytes_read != phdrs_size) {
    NaClLog(LOG_ERROR, "Failed to read ELF program headers: errno=%d\n", errno);
    return errno;
  }

  /* Find code and data segments. */
  Elf32_Phdr *code_segment = NULL;
  Elf32_Phdr *first_data = NULL;
  Elf32_Phdr *last_data = NULL;
  Elf32_Phdr *ph;
  for (ph = phdr; ph < &phdr[ehdr.e_phnum]; ++ph) {
    if (ph->p_type != PT_LOAD)
      continue;
    if ((ph->p_flags & PF_X) != 0) {
      /* Code segment. */
      CHECK(code_segment == NULL);
      code_segment = ph;
    } else {
      /* Data segment. */
      last_data = ph;
      if (first_data == NULL)
        first_data = ph;
    }
  }

  /*
   * Currently, if the PNaCl translator is given a data-only PSO, it
   * produces a PSO with an empty code segment rather than a DSO without a
   * code segment.
   */
  CHECK(code_segment != NULL);
  CHECK(code_segment->p_vaddr == 0);
  CHECK(code_segment->p_filesz == code_segment->p_memsz);
  CHECK(first_data != NULL);

  /*
   * load_bias is the offset to add to PT_LOAD segments' load addresses to
   * give their address in memory.  This is non-zero for ET_DYN ELF objects
   * but would be zero for ET_EXEC objects.
   */
  uintptr_t load_bias;
#if defined(__native_client_nonsfi__)
  /*
   * For Non-SFI mode, data and code may be allocated together in one
   * continuous chunk of memory (without a call to allocate_code_data to
   * allocate each separately). However, there may still be a gap between the
   * code and data segments, so the full span is calculated prior to mmap-ing.
   */
  size_t span = last_data->p_vaddr + last_data->p_memsz;
  void *map_result = mmap(NULL, span, PROT_NONE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (map_result == MAP_FAILED)
    return errno;
  load_bias = (uintptr_t) map_result;
#else  /* SFI */
  size_t code_size = code_segment->p_filesz;
  size_t data_offset = first_data->p_vaddr;
  size_t data_size = last_data->p_vaddr + last_data->p_memsz - data_offset;
  if (code_size == 0) {
    CHECK(data_offset == 0);
    /*
     * The IRT's allocate_code_data() does not accept code_size == 0,
     * so use a data-only mmap() for this case.
     */
    void *map_result = mmap(NULL, data_size, PROT_NONE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map_result == MAP_FAILED)
      return errno;
    load_bias = (uintptr_t) map_result;
  } else {
    int err = allocate_code_data(0, code_size, data_offset,
                                 page_size_round_up(data_size), &load_bias);
    if (err != 0)
      return err;
  }
#endif

  /* Map the PT_LOAD segments. */
  Elf32_Dyn *pt_dynamic = 0;
  uintptr_t prev_segment_end = 0;
  bool pso_root_is_valid = 0;
  for (ph = phdr; ph < &phdr[ehdr.e_phnum]; ++ph) {
    int segment_index = ph - phdr;
    switch (ph->p_type) {
      case PT_DYNAMIC:
        pt_dynamic = (Elf32_Dyn *) (load_bias + ph->p_vaddr);
        break;

      case PT_LOAD: {
        int prot = elf_flags_to_mmap_flags(ph->p_flags);
        uintptr_t segment_start = page_size_round_down(ph->p_vaddr);
        uintptr_t segment_end = page_size_round_up(ph->p_vaddr + ph->p_memsz);
        if (segment_start < prev_segment_end) {
          NaClLog(LOG_FATAL, "PT_LOAD segments overlap or are not sorted: %d\n",
                  segment_index);
        }
        prev_segment_end = segment_end;

        /* Handle the BSS (zero-initialized data). */
        if (ph->p_memsz < ph->p_filesz) {
          NaClLog(LOG_FATAL, "Bad ELF segment %d: p_memsz < p_filesz\n",
                  segment_index);
        }
        if (ph->p_memsz > ph->p_filesz) {
          if ((ph->p_flags & PF_W) == 0) {
            NaClLog(LOG_FATAL,
                    "Bad ELF segment %d: non-writable segment with BSS\n",
                    segment_index);
          }
          /*
           * NaCl's mmap() interface has a quirk when mapping the last 64k
           * page of a file.  Within the page, NaCl maps the 4k pages
           * beyond the file's extent as PROT_NONE.  This is due to
           * Windows' limitations.  For background, see:
           * https://code.google.com/p/nativeclient/issues/detail?id=1068
           * This affects how we handle the BSS in the RW data segment.
           *
           * The RW data segment consists of two parts:
           *   * data initialized from the file
           *     -- offsets 0 to p_filesz from the segment's start
           *   * zero-initialized data, known as the "BSS"
           *     -- offsets p_filesz to p_memsz from the segment's start
           *
           * Usually there will be one corner-case 64k page in memory that
           * overlaps both of those parts.  A normal Unix dynamic linker
           * would map this corner-case page from the file and then zero
           * the BSS within this mapping using memset().  But that memset()
           * can crash, since the BSS can go beyond the file's extent.
           *
           * Instead, we mmap() this corner-case page as anonymous memory
           * and copy in initialized data from the file using pread().
           * This is potentially slightly faster than the memset()
           * approach, anyway, because it avoids trapping to the kernel to
           * do a copy-on-write of the corner-case page.
           */
          uintptr_t bss_map_start =
              page_size_round_down(ph->p_vaddr + ph->p_filesz);
          uintptr_t offset_into_segment = bss_map_start - ph->p_vaddr;

          void *map_addr = (void *) (load_bias + bss_map_start);
          void *map_result = mmap(
              map_addr, segment_end - bss_map_start,
              prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
          if (map_result != map_addr) {
            NaClLog(LOG_FATAL,
                    "Failed to map BSS for ELF segment %d: errno=%d\n",
                    segment_index, errno);
          }
          ssize_t bytes_to_copy = ph->p_filesz - offset_into_segment;
          CHECK(bytes_to_copy >= 0);
          bytes_read = pread(fd, map_addr, bytes_to_copy,
                             ph->p_offset + offset_into_segment);
          if (bytes_read != bytes_to_copy) {
            NaClLog(LOG_FATAL, "Failed to pread() bytes before BSS: errno=%d\n",
                    errno);
          }
          /* Reduce the range that we will mmap() in from the file. */
          segment_end = bss_map_start;
        }

        if (segment_end != segment_start) {
          void *segment_addr = (void *) (load_bias + segment_start);
          void *map_result = mmap(segment_addr, segment_end - segment_start,
                                  prot, MAP_PRIVATE | MAP_FIXED, fd,
                                  page_size_round_down(ph->p_offset));
          if (map_result != segment_addr) {
            NaClLog(LOG_FATAL, "Failed to map ELF segment %d: errno=%d\n",
                    segment_index, errno);
          }
        }

        /*
         * We use e_entry to point to __pnacl_pso_root.  Check that it
         * points to a GlobalVariable (in a data segment) rather than a
         * function (in a code segment).  This is just a non-essential
         * sanity check for well-formedness.  The PNaCl ABI verifier
         * already checks that __pnacl_pso_root is a GlobalVariable.
         */
        if ((ph->p_flags & PF_X) == 0 &&
            ph->p_vaddr <= ehdr.e_entry &&
            ehdr.e_entry < ph->p_vaddr + ph->p_filesz) {
          pso_root_is_valid = true;
        }
      }
    }
  }

  if (!pso_root_is_valid) {
    NaClLog(LOG_FATAL, "PSO root does not point into a non-executable "
            "PT_LOAD segment: e_entry=0x%x\n", ehdr.e_entry);
  }

  apply_relocations(pt_dynamic, load_bias);

  *pso_root = (void *) (load_bias + ehdr.e_entry);
  return 0;
}

int pnacl_load_elf_file(const char *filename, void **pso_root) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0)
    return errno;
  int err = load_elf_file_from_fd(fd, pso_root);
  if (close(fd) != 0) {
    NaClLog(LOG_FATAL, "pnacl_load_elf_file: close() failed: errno=%d\n",
            errno);
  }
  return err;
}
