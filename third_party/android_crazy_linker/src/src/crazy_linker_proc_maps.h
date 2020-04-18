// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_PROC_MAPS_H
#define CRAZY_LINKER_PROC_MAPS_H

// Helper classes and functions to extract useful information out of
// /proc/self/maps.

#include <stdint.h>
#include <sys/mman.h>  // for PROT_READ etc...

namespace crazy {

class ProcMapsInternal;

class ProcMaps {
 public:
  // Used to open /proc/self/maps.
  // There is no error reporting. If the file can't be opened, then
  // GetNextEntry() will return false on the first call.
  ProcMaps();

  // Used to open /proc/$PID/maps.
  // There is also no error reporting.
  explicit ProcMaps(pid_t pid);

  ~ProcMaps();

  // Small structure to model an entry.
  struct Entry {
    size_t vma_start;
    size_t vma_end;
    int prot_flags;
    size_t load_offset;
    const char* path;  // can be NULL, not always zero-terminated.
    size_t path_len;   // 0 if |path| is NULL.
  };

  void Rewind();

  // Get next entry in maps, return NULL on failure.
  // On success, return true and set |entry| to valid values.
  // Note that the |entry->path| field can be NULL or will point to
  // an address which content may change on the next call to this method.
  bool GetNextEntry(Entry* entry);

  int GetProtectionFlagsForAddress(void* address);

 private:
  ProcMapsInternal* internal_;
};

// Find which loaded ELF binary contains |address|.
// On success, returns true and sets |*load_address| to its load address,
// and fills |path_buffer| with the path to the corresponding file.
bool FindElfBinaryForAddress(void* address,
                             uintptr_t* load_address,
                             char* path_buffer,
                             size_t path_buffer_len);

// Returns the current protection bit flags for the page holding a given
// |address|. On success, returns true and sets |*prot_flags|.
bool FindProtectionFlagsForAddress(void* address, int* prot_flags);

// Return the load address of a given ELF binary.
// If |file_name| contains a slash, this will try to perform an
// exact match with the content of /proc/self/maps. Otherwise,
// it will be taken as a base name, and the load address of the first
// matching entry will be returned.
// On success, returns true and sets |*load_address| to the load address,
// and |*load_offset| to the load offset.
bool FindLoadAddressForFile(const char* file_name,
                            uintptr_t* load_address,
                            uintptr_t* load_offset);

}  // namespace crazy

#endif  // CRAZY_LINKER_PROC_MAPS_H
