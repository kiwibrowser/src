/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/minidump_generator/build_id.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf64.h"

#if defined(__GLIBC__)
# define DYNAMIC_LOADING_SUPPORT 1
#else
# define DYNAMIC_LOADING_SUPPORT 0
#endif


// The next-generation (upstream) linkers define this symbol when the
// ELF file and program headers are visible in the address space.  We
// use a weak reference to be compatible with the old nacl-binutils
// linkers and layout, where this symbol is not defined and the
// headers are not visible in the address space.
extern const char __ehdr_start __attribute__((weak));

// The newest versions of the "old" nacl-binutils linker define these
// symbols to give the bounds of the section that contains the build ID note.
extern const Elf32_Nhdr __note_gnu_build_id_start[] __attribute__((weak));
extern const Elf32_Nhdr __note_gnu_build_id_end[] __attribute__((weak));

static uintptr_t note_align(uintptr_t value) {
  return (value + 3) & ~3;
}

static int GetBuildIdFromNotes(const Elf32_Nhdr *start,
                               const Elf32_Nhdr *end,
                               const char **data, size_t *size) {
  const Elf32_Nhdr *note = start;

  while (note < end) {
    uintptr_t name_ptr = (uintptr_t) &note[1];
    assert(name_ptr <= (uintptr_t) end);
    uintptr_t desc_ptr = note_align(name_ptr + note->n_namesz);
    const Elf32_Nhdr *next_ptr = ((const Elf32_Nhdr *)
                                  note_align(desc_ptr + note->n_descsz));
    assert(next_ptr <= end);
    if (note->n_type == NT_GNU_BUILD_ID &&
        note->n_namesz == sizeof(ELF_NOTE_GNU) &&
        memcmp((const char *) name_ptr, ELF_NOTE_GNU,
               sizeof(ELF_NOTE_GNU)) == 0) {
      *data = (const char *) desc_ptr;
      *size = note->n_descsz;
      return 1;
    }
    note = next_ptr;
  }

  return 0;
}

// The PNaCl toolchain currently generates ELFCLASS64 executables for
// x86-64, so we need to handle that case as well as ELFCLASS32.  In
// the long term, the NaCl toolchains should generate ELFCLASS32 for
// all architectures.
template <class Elf_Ehdr, class Elf_Phdr>
static int GetBuildId(const char **data, size_t *size) {
  Elf_Ehdr *ehdr = (Elf_Ehdr *) &__ehdr_start;
  if (ehdr->e_phentsize != sizeof(Elf_Phdr)) {
    fprintf(stderr, "Build ID not found: bad e_phentsize\n");
    return 0;
  }
  Elf_Phdr *phdrs = (Elf_Phdr *) ((uintptr_t) ehdr + ehdr->e_phoff);
  bool found_pt_note = false;
  for (int i = 0; i < ehdr->e_phnum; ++i) {
    Elf_Phdr *phdr = &phdrs[i];
    if (phdr->p_type == PT_NOTE) {
      found_pt_note = true;
      // Elf64_Nhdr is the same as Elf32_Nhdr so we can use either here.
      const Elf32_Nhdr *note = (const Elf32_Nhdr *) ((uintptr_t) ehdr +
                                                     phdr->p_offset);
      const Elf32_Nhdr *note_end = ((const Elf32_Nhdr *)
                                    (uintptr_t) note + phdr->p_memsz);
      if (GetBuildIdFromNotes(note, note_end, data, size))
        return 1;
    }
  }
  if (found_pt_note) {
    fprintf(stderr, "Build ID not found: no NT_GNU_BUILD_ID in PT_NOTE\n");
  } else {
    fprintf(stderr, "Build ID not found: no PT_NOTE segment\n");
  }
  return 0;
}

/*
 * This is only useful (or meaningful) for the static-linking case.
 * When this file is part of a shared object, then this would be
 * finding the build ID of that shared object itself rather than that
 * of the main executable.
 */
#if !DYNAMIC_LOADING_SUPPORT
int nacl_get_build_id(const char **data, size_t *size) {
  if (__note_gnu_build_id_start != NULL) {
    assert(__note_gnu_build_id_end >= __note_gnu_build_id_start);
    return GetBuildIdFromNotes(__note_gnu_build_id_start,
                               __note_gnu_build_id_end,
                               data, size);
  }
  if (&__ehdr_start == NULL) {
    fprintf(stderr, "Build ID not found: __ehdr_start not defined\n");
    return 0;
  }
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *) &__ehdr_start;
  switch (ehdr->e_ident[EI_CLASS]) {
    case ELFCLASS32:
      return GetBuildId<Elf32_Ehdr, Elf32_Phdr>(data, size);
    case ELFCLASS64:
      return GetBuildId<Elf64_Ehdr, Elf64_Phdr>(data, size);
  }
  fprintf(stderr, "Build ID not found: unexpected ELFCLASS\n");
  return 0;
}
#endif

int nacl_get_build_id_from_notes(
    const void *notes, size_t notes_size,
    const char **data, size_t *size) {
  const Elf32_Nhdr *start = reinterpret_cast<const Elf32_Nhdr *>(notes);
  const Elf32_Nhdr *end = reinterpret_cast<const Elf32_Nhdr *>(
      reinterpret_cast<const char *>(notes) + notes_size);
  return GetBuildIdFromNotes(start, end, data, size);
}
