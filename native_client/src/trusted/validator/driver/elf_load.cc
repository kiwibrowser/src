/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/elf.h"
#include "native_client/src/include/elf_constants.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/validator/driver/elf_load.h"


namespace elf_load {

void ReadImage(const char *filename, Image *image) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("Failed to open input file: %s\n", filename);
    exit(1);
  }

  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);

  image->resize(file_size);

  fseek(fp, 0, SEEK_SET);
  size_t got = fread(&(*image)[0], 1, file_size, fp);
  if (got != file_size) {
    printf("Unable to read image from input file: %s\n", filename);
    exit(1);
  }
  fclose(fp);
}


template<typename ElfEhdrType, typename ElfPhdrType>
Segment FindTextSegment(const Image &image) {
  // Initialization only to suppress 'uninitialized' warning.
  Segment segment = {NULL, 0, 0};
  bool found = false;

  const ElfEhdrType &header = *reinterpret_cast<const ElfEhdrType *>(&image[0]);
  CHECK(sizeof(header) <= image.size());
  CHECK(memcmp(header.e_ident, ELFMAG, SELFMAG) == 0);

  for (uint64_t index = 0; index < header.e_phnum; ++index) {
    uint64_t phdr_offset = header.e_phoff + header.e_phentsize * index;
    // static_cast to silence msvc on 32-bit platform
    const ElfPhdrType &phdr = *reinterpret_cast<const ElfPhdrType *>(
        &image[static_cast<size_t>(phdr_offset)]);

    // TODO(shcherbina): size of other loadable segments
    if (phdr.p_type == PT_LOAD && (phdr.p_flags & PF_X)) {
      if (found) {
        printf("More than one text segment.\n");
        exit(1);
      }

      if (phdr.p_flags != (PF_R | PF_X)) {
        // Cast to support 64-bit ELF.
        printf("Text segment is expected to have flags PF_R | PF_X "
               "(has 0x%" NACL_PRIx64 " instead).\n",
               static_cast<uint64_t>(phdr.p_flags));
        exit(1);
      }

      CHECK(phdr.p_filesz <= phdr.p_memsz);
      if (phdr.p_filesz < phdr.p_memsz) {
        printf("File image is smaller than memory image size.\n");
        exit(1);
      }

      // TODO(shcherbina): find or introduce proper constant.
      if (phdr.p_filesz > 256 << 20) {
        printf("Test segment is too large.\n");
        exit(1);
      }

      if (phdr.p_vaddr > UINT32_MAX - phdr.p_filesz) {
        printf("Text segment does not fit in 4GB.\n");
        exit(1);
      }

      segment.data = &image[static_cast<size_t>(phdr.p_offset)];
      segment.size = static_cast<uint32_t>(phdr.p_filesz);
      segment.vaddr = static_cast<uint32_t>(phdr.p_vaddr);
      found = true;
    }
  }
  if (!found) {
    printf("Text segment not found.\n");
    exit(1);
  }
  return segment;
}


Architecture GetElfArch(const Image &image) {
  // e_machine field is the same for Elf32_Ehdr and Elf64_Ehdr.
  const Elf32_Ehdr &header = *reinterpret_cast<const Elf32_Ehdr *>(&image[0]);
  switch (header.e_machine) {
    case EM_386:
      return X86_32;
    case EM_X86_64:
      return X86_64;
    case EM_ARM:
      return ARM;
    default:
      printf("Unsupported e_machine %" NACL_PRIu16 ".\n", header.e_machine);
      exit(1);
  }
}


Segment GetElfTextSegment(const Image &image) {
  // We don't know in advance whether it's elf32 or elf64, but we are only
  // looking at few first fields of the header, and they are the same for
  // Elf32_Ehdr and Elf64_Ehdr.
  const Elf32_Ehdr &header = *reinterpret_cast<const Elf32_Ehdr *>(&image[0]);
  if (image.size() < sizeof(header) ||
      memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
    printf("Not an ELF file.\n");
    exit(1);
  }

  Segment segment;
  switch (header.e_ident[EI_CLASS]) {
    case ELFCLASS32:
      segment = FindTextSegment<Elf32_Ehdr, Elf32_Phdr>(image);
      break;
    case ELFCLASS64:
      segment = FindTextSegment<Elf64_Ehdr, Elf64_Phdr>(image);
      break;
    default:
      printf("Invalid ELF class %d.\n", header.e_ident[EI_CLASS]);
      exit(1);
  }

  return segment;
}

}  // namespace elf_load
