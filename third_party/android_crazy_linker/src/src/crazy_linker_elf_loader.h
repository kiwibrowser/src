// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_ELF_LOADER_H
#define CRAZY_LINKER_ELF_LOADER_H

#include "crazy_linker_error.h"
#include "crazy_linker_system.h"  // For ScopedFileDescriptor
#include "elf_traits.h"

namespace crazy {

// Helper class used to load an ELF binary in memory.
//
// Note that this doesn't not perform any relocation, the purpose
// of this class is strictly to map all loadable segments from the
// file to their correct location.
//
class ElfLoader {
 public:
  ElfLoader();
  ~ElfLoader();

  // Try to load a library at a given address. On failure, this will
  // update the linker error message and returns false.
  //
  // |lib_path| is the full library path, and |wanted_address| should
  // be the desired load address, or 0 to enable randomization.
  //
  // |file_offset| is an offset in the file where the ELF header will
  // be looked for.
  //
  // |wanted_address| is the wanted load address (of the first loadable
  // segment), or 0 to enable randomization.
  //
  // On success, the library's loadable segments will be mapped in
  // memory with their original protection. However, no further processing
  // will be performed.
  //
  // On failure, returns false and assign an error message to |error|.
  bool LoadAt(const char* lib_path,
              off_t file_offset,
              uintptr_t wanted_address,
              Error* error);

  // Only call the following functions after a succesfull LoadAt() call.

  size_t phdr_count() { return phdr_num_; }
  ELF::Addr load_start() { return reinterpret_cast<ELF::Addr>(load_start_); }
  ELF::Addr load_size() { return load_size_; }
  ELF::Addr load_bias() { return load_bias_; }
  const ELF::Phdr* loaded_phdr() { return loaded_phdr_; }

 private:
  FileDescriptor fd_;
  const char* path_;

  ELF::Ehdr header_;
  size_t phdr_num_;

  void* phdr_mmap_;  // temporary copy of the program header.
  ELF::Phdr* phdr_table_;
  ELF::Addr phdr_size_;  // and its size.

  off_t file_offset_;
  void* wanted_load_address_;
  void* load_start_;     // First page of reserved address space.
  ELF::Addr load_size_;  // Size in bytes of reserved address space.
  ELF::Addr load_bias_;  // load_bias, add this value to all "vaddr"
                         // values in the library to get the corresponding
                         // memory address.

  const ELF::Phdr* loaded_phdr_;  // points to the loaded program header.

  void* reserved_start_;  // Real first page of reserved address space.
  size_t reserved_size_;  // Real size in bytes of reserved address space.

  // Individual steps used by ::LoadAt()
  bool ReadElfHeader(Error* error);
  bool ReadProgramHeader(Error* error);
  bool ReserveAddressSpace(Error* error);
  bool LoadSegments(Error* error);
  bool FindPhdr(Error* error);
  bool CheckPhdr(ELF::Addr, Error* error);
};

}  // namespace crazy

#endif  // CRAZY_LINKER_ELF_LOADER_H
