/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client support for thread local storage
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf64.h"
#include "native_client/src/untrusted/nacl/nacl_thread.h"
#include "native_client/src/untrusted/nacl/tls.h"
#include "native_client/src/untrusted/nacl/tls_params.h"

/*
 * We support two mechanisms for finding templates for TLS variables:
 *
 *  1) The PT_TLS header (in the ELF program headers), which is
 *     located via the __ehdr_start symbol, which current binutils
 *     linkers define when the ELF file headers and program headers
 *     are mapped into the address space.
 *
 *  2) The __tls_template_* symbols, which are defined by PNaCl's
 *     ExpandTls LLVM pass, which is used when linking ABI-stable
 *     pexes.
 *
 * We use weak references to refer to these symbols so that the code
 * can work with both mechanisms.
 *
 * The __tls_template_* symbols used to be defined by the binutils
 * linker (using linker scripts), but this has been superseded by
 * having the linker define __ehdr_start.
 */

extern char __tls_template_start __attribute__((weak));
extern char __tls_template_tdata_end __attribute__((weak));
extern char __tls_template_end __attribute__((weak));
extern uint32_t __tls_template_alignment __attribute__((weak));

extern union {
  Elf32_Ehdr ehdr32;
  Elf64_Ehdr ehdr64;
} __ehdr_start __attribute__((weak, visibility("hidden")));

static size_t aligned_size(size_t size, size_t alignment) {
  return (size + alignment - 1) & -alignment;
}

static char *aligned_addr(void *start, size_t alignment) {
  return (void *) aligned_size((size_t) start, alignment);
}

/*
 * Collect information about the TLS initializer data here.
 * The first call to get_tls_info() fills in all the data,
 * based either on PT_TLS or on __tls_template_*.
 */

struct tls_info {
  const char *tdata_start;  /* Address of .tdata (initializer data) */
  size_t tdata_size;        /* Size of .tdata (initializer data) */
  size_t tbss_size;         /* Size of .tbss (zero-fill space after .tdata) */
  size_t tls_alignment;     /* Alignment required for TLS segment */
};

static struct tls_info cached_tls_info;

#if defined(__arm__)

/*
 * On ARM, the linker might not optimize GD-model TLS accesses into the
 * simpler forms that don't use a helper function.  __tls_get_addr is
 * called with the address of a two-word GOT entry: the first word is the
 * module ID, which is immaterial for static linking; the second word is
 * the offset within the (only) module's TLS data.  The TLS data always
 * starts at 8 bytes past the thread pointer, plus alignment.
 */

static size_t tp_tls_offset;

static void finish_info_cache(const struct tls_info *info) {
  /*
   * Cache this calculation at startup so it doesn't need to be repeated.
   */
  tp_tls_offset = aligned_size(8, info->tls_alignment);
}

void *__tls_get_addr(uintptr_t *entry) {
  /*
   * This is never called before finish_info_cache has been called.
   */
  return (char *) __builtin_thread_pointer() + tp_tls_offset + entry[1];
}

#else  /* !defined(__arm__) */

static void finish_info_cache(
    const struct tls_info *info __attribute__((unused))) {
}

#endif  /* defined(__arm__) */

#define DEFINE_READ_PHDR(func_name, ehdr, Elf_Phdr, elf_class)          \
  __attribute__((unused))                                               \
  static int func_name(void) {                                          \
    if ((ehdr) == NULL ||                                               \
        (ehdr)->e_ident[EI_CLASS] != (elf_class) ||                     \
        (ehdr)->e_phentsize != sizeof(Elf_Phdr))                        \
      return 0;                                                         \
    const Elf_Phdr *phdr =                                              \
      (const Elf_Phdr *) ((const char *) (ehdr) + (ehdr)->e_phoff);     \
    for (int i = 0; i < (ehdr)->e_phnum; ++i) {                         \
      if (phdr[i].p_type == PT_TLS) {                                   \
        cached_tls_info.tls_alignment = phdr[i].p_align;                \
        cached_tls_info.tdata_start =                                   \
          (const char *) (uintptr_t) phdr[i].p_vaddr;                   \
        /* For a PIE, we should offset the the load address. */         \
        if ((ehdr)->e_type == ET_DYN)                                   \
          cached_tls_info.tdata_start += (uintptr_t) (ehdr);            \
        cached_tls_info.tdata_size = phdr[i].p_filesz;                  \
        cached_tls_info.tbss_size = phdr[i].p_memsz - phdr[i].p_filesz; \
        return 1;                                                       \
      }                                                                 \
    }                                                                   \
    return 0;                                                           \
  }

DEFINE_READ_PHDR(read_phdr32, &__ehdr_start.ehdr32, Elf32_Phdr, ELFCLASS32)
DEFINE_READ_PHDR(read_phdr64, &__ehdr_start.ehdr64, Elf64_Phdr, ELFCLASS64)

static const struct tls_info *get_tls_info(void) {
  if (cached_tls_info.tls_alignment == 0) {
    int did_read_phdr;
#if defined(__pnacl__) || defined(__x86_64__)
    /*
     * This is only needed in non-ABI-stable pexes for which the
     * ExpandTls pass has not been run.  On ABI-stable pexes,
     * link-time optimization will optimize away these calls because
     * &__ehdr_start == NULL.
     */
    did_read_phdr = read_phdr32() || read_phdr64();
#else
    did_read_phdr = read_phdr32();
#endif

    if (!did_read_phdr) {
      /*
       * We didn't find anything that way, so assume that we were
       * built with PNaCl's ExpandTls LLVM pass.
       */
      cached_tls_info.tls_alignment = __tls_template_alignment;
      cached_tls_info.tdata_start = &__tls_template_start;
      cached_tls_info.tdata_size = (&__tls_template_tdata_end -
                                    &__tls_template_start);
      cached_tls_info.tbss_size = (&__tls_template_end -
                                   &__tls_template_tdata_end);
    }

    finish_info_cache(&cached_tls_info);
  }
  return &cached_tls_info;
}

/*
 * We support x86 and ARM TLS layouts.
 *
 * x86 layout:
 *  * TLS data + BSS
 *  * padding to round TLS data+BSS size upto tls_alignment
 *  --- thread pointer ($tp) points here
 *  * TDB (thread library's data block)
 *
 * ARM layout:
 *  * TDB (thread library's data block)
 *     * note that no padding follows the TDB
 *  --- thread pointer ($tp) points here
 *  * 8-byte header for use by the thread library
 *  * padding to round 8-byte header upto tls_alignment
 *  * TLS data + BSS
 *
 * The offset from the thread pointer to the TLS data is fixed by the
 * linker.
 *
 * The addresses of the thread pointer and TLS data must both be
 * aligned to tls_alignment.  Since combined_area is not necessarily
 * aligned to tls_alignment, padding may be required at the start of
 * both x86 and ARM TLS layouts (not shown above).
 */

static char *tp_from_combined_area(const struct tls_info *info,
                                   void *combined_area, size_t tdb_size) {
  size_t tls_size = info->tdata_size + info->tbss_size;
  ptrdiff_t tdboff = __nacl_tp_tdb_offset(tdb_size);
  if (tdboff < 0) {
    /*
     * The combined area is big enough to hold the TDB and then be aligned
     * up to the $tp alignment requirement.  If the whole area is aligned
     * to the $tp requirement, then aligning the beginning of the area
     * would give us the beginning unchanged, which is not what we need.
     * Instead, align from the putative end of the TDB, to decide where
     * $tp--the true end of the TDB--should actually lie.
     */
    return aligned_addr((char *) combined_area + tdb_size, info->tls_alignment);
  } else {
    /*
     * The linker increases the size of the TLS block up to its alignment
     * requirement, and that total is subtracted from the $tp address to
     * access the TLS area.  To keep that final address properly aligned,
     * we need to align up from the allocated space and then add the
     * aligned size.
     */
    tls_size = aligned_size(tls_size, info->tls_alignment);
    return aligned_addr((char *) combined_area, info->tls_alignment) + tls_size;
  }
}

void *__nacl_tls_initialize_memory(void *combined_area, size_t tdb_size) {
  const struct tls_info *info = get_tls_info();
  size_t tls_size = info->tdata_size + info->tbss_size;
  char *combined_area_end =
      (char *) combined_area + __nacl_tls_combined_size(tdb_size);
  void *tp = tp_from_combined_area(info, combined_area, tdb_size);
  char *start = tp;

  if (__nacl_tp_tls_offset(0) > 0) {
    /*
     * From $tp, we skip the header size and then must round up from
     * there to the required alignment (which is what the linker will
     * will do when calculating TPOFF relocations at link time).  The
     * end result is that the offset from $tp matches the one chosen
     * by the linker exactly and that the final address is aligned to
     * info->tls_alignment (since $tp was already aligned to at least
     * that much).
     */
    start += aligned_size(__nacl_tp_tls_offset(tls_size), info->tls_alignment);
  } else {
    /*
     * We'll subtract the aligned size of the TLS block from $tp, which
     * must itself already be adequately aligned.
     */
    start += __nacl_tp_tls_offset(aligned_size(tls_size, info->tls_alignment));
  }

  /* Sanity check.  (But avoid pulling in assert() here.) */
  if (start + info->tdata_size + info->tbss_size > combined_area_end)
    __builtin_trap();
  memcpy(start, info->tdata_start, info->tdata_size);
  memset(start + info->tdata_size, 0, info->tbss_size);

  if (__nacl_tp_tdb_offset(tdb_size) == 0) {
    /*
     * On x86 (but not on ARM), the TDB sits directly at $tp and the
     * first word there must hold the $tp pointer itself.
     */
    void *tdb = (char *) tp + __nacl_tp_tdb_offset(tdb_size);
    *(void **) tdb = tdb;
  }

  return tp;
}

size_t __nacl_tls_combined_size(size_t tdb_size) {
  const struct tls_info *info = get_tls_info();
  size_t tls_size = info->tdata_size + info->tbss_size;
  ptrdiff_t tlsoff = __nacl_tp_tls_offset(tls_size);
  size_t combined_size = tls_size + tdb_size;
  /*
   * __nacl_tls_initialize_memory() accepts a non-aligned pointer; it
   * aligns the thread pointer itself.  We have to reserve some extra
   * space to allow this alignment padding to occur.
   */
  combined_size += info->tls_alignment - 1;
  if (tlsoff > 0) {
    /*
     * ARM case: We have to add ARM's 8 byte header, because that is
     * not incorporated into tls_size.  Furthermore, the header is
     * padded out to tls_alignment.
     */
    combined_size += aligned_size(tlsoff, info->tls_alignment);
  }
  return combined_size;
}
