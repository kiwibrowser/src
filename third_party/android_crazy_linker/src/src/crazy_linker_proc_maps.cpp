// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_proc_maps.h"

#include <inttypes.h>
#include <limits.h>

#include "elf_traits.h"
#include "crazy_linker_debug.h"
#include "crazy_linker_line_reader.h"
#include "crazy_linker_util.h"
#include "crazy_linker_system.h"

namespace crazy {

namespace {

// Decompose the components of a /proc/$PID/maps file into multiple
// components. |line| should be the address of a zero-terminated line
// of input. On success, returns true and sets |*entry|, false otherwise.
//
// IMPORTANT: On success, |entry->path| will point into the input line,
// the caller will have to copy the string into a different location if
// it needs to persist it.
bool ParseProcMapsLine(const char* line,
                       const char* line_end,
                       ProcMaps::Entry* entry) {
  // Example input lines on a 64-bit system, one cannot assume that
  // everything is properly sized.
  //
  // 00400000-0040b000 r-xp 00000000 08:01 6570708
  // /bin/cat
  // 0060a000-0060b000 r--p 0000a000 08:01 6570708
  // /bin/cat
  // 0060b000-0060c000 rw-p 0000b000 08:01 6570708
  // /bin/cat
  // 01dd0000-01df1000 rw-p 00000000 00:00 0
  // [heap]
  // 7f4b8d4d7000-7f4b8e22a000 r--p 00000000 08:01 38666648
  // /usr/lib/locale/locale-archive
  // 7f4b8e22a000-7f4b8e3df000 r-xp 00000000 08:01 28836281
  // /lib/x86_64-linux-gnu/libc-2.15.so
  // 7f4b8e3df000-7f4b8e5de000 ---p 001b5000 08:01 28836281
  // /lib/x86_64-linux-gnu/libc-2.15.so
  // 7f4b8e5de000-7f4b8e5e2000 r--p 001b4000 08:01 28836281
  // /lib/x86_64-linux-gnu/libc-2.15.so
  // 7f4b8e5e2000-7f4b8e5e4000 rw-p 001b8000 08:01 28836281
  // /lib/x86_64-linux-gnu/libc-2.15.so
  const char* p = line;
  for (int token = 0; token < 7; ++token) {
    char separator = (token == 0) ? '-' : ' ';
    // skip leading token separators first.
    while (p < line_end && *p == separator)
      p++;

    // find start and end of current token, and compute start of
    // next search. The result of memchr(_,_,0) is undefined, treated as
    // not-found.
    const char* tok_start = p;
    const size_t range = line_end - p;
    const char* tok_end;
    if (range > 0)
      tok_end = static_cast<const char*>(memchr(p, separator, range));
    else
      tok_end = NULL;
    if (!tok_end) {
      tok_end = line_end;
      p = line_end;
    } else {
      p = tok_end + 1;
    }

    if (tok_end == tok_start) {
      if (token == 6) {
        // empty token can happen for index 6, when there is no path
        // element on the line. This corresponds to anonymous memory
        // mapped segments.
        entry->path = NULL;
        entry->path_len = 0;
        break;
      }
      return false;
    }

    switch (token) {
      case 0:  // vma_start
        entry->vma_start = static_cast<size_t>(strtoumax(tok_start, NULL, 16));
        break;

      case 1:  // vma_end
        entry->vma_end = static_cast<size_t>(strtoumax(tok_start, NULL, 16));
        break;

      case 2:  // protection bits
      {
        int flags = 0;
        for (const char* t = tok_start; t < tok_end; ++t) {
          if (*t == 'r')
            flags |= PROT_READ;
          if (*t == 'w')
            flags |= PROT_WRITE;
          if (*t == 'x')
            flags |= PROT_EXEC;
        }
        entry->prot_flags = flags;
      } break;

      case 3:  // page offset
        entry->load_offset =
            static_cast<size_t>(strtoumax(tok_start, NULL, 16)) * PAGE_SIZE;
        break;

      case 6:  // path
        // Get rid of trailing newlines, if any.
        while (tok_end > tok_start && tok_end[-1] == '\n')
          tok_end--;
        entry->path = tok_start;
        entry->path_len = tok_end - tok_start;
        break;

      default:  // ignore all other tokens.
        ;
    }
  }
  return true;
}

}  // namespace

// Internal implementation of ProcMaps class.
class ProcMapsInternal {
 public:
  ProcMapsInternal() : index_(0), entries_() {}

  ~ProcMapsInternal() { Reset(); }

  bool Open(const char* path) {
    Reset();
    LineReader reader(path);
    index_ = 0;
    while (reader.GetNextLine()) {
      ProcMaps::Entry entry = {0, };
      if (!ParseProcMapsLine(
               reader.line(), reader.line() + reader.length(), &entry)) {
        // Ignore broken lines.
        continue;
      }

      // Reallocate path.
      const char* old_path = entry.path;
      if (old_path) {
        char* new_path = static_cast<char*>(::malloc(entry.path_len + 1));
        ::memcpy(new_path, old_path, entry.path_len);
        new_path[entry.path_len] = '\0';
        entry.path = const_cast<const char*>(new_path);
      }

      entries_.PushBack(entry);
    }
    return true;
  }

  void Rewind() { index_ = 0; }

  bool GetNextEntry(ProcMaps::Entry* entry) {
    if (index_ >= entries_.GetCount())
      return false;

    *entry = entries_[index_++];
    return true;
  }

 private:
  void Reset() {
    for (size_t n = 0; n < entries_.GetCount(); ++n) {
      ProcMaps::Entry& entry = entries_[n];
      ::free(const_cast<char*>(entry.path));
    }
    entries_.Resize(0);
  }

  size_t index_;
  Vector<ProcMaps::Entry> entries_;
};

ProcMaps::ProcMaps() {
  internal_ = new ProcMapsInternal();
  (void)internal_->Open("/proc/self/maps");
}

ProcMaps::ProcMaps(pid_t pid) {
  internal_ = new ProcMapsInternal();
  char maps_file[32];
  snprintf(maps_file, sizeof maps_file, "/proc/%u/maps", pid);
  (void)internal_->Open(maps_file);
}

ProcMaps::~ProcMaps() { delete internal_; }

void ProcMaps::Rewind() { internal_->Rewind(); }

bool ProcMaps::GetNextEntry(Entry* entry) {
  return internal_->GetNextEntry(entry);
}

int ProcMaps::GetProtectionFlagsForAddress(void* address) {
  size_t vma_addr = reinterpret_cast<size_t>(address);
  internal_->Rewind();
  ProcMaps::Entry entry;
  while (internal_->GetNextEntry(&entry)) {
    if (entry.vma_start <= vma_addr && vma_addr < entry.vma_end)
      return entry.prot_flags;
  }
  return 0;
}

bool FindElfBinaryForAddress(void* address,
                             uintptr_t* load_address,
                             char* path_buffer,
                             size_t path_buffer_len) {
  ProcMaps self_maps;
  ProcMaps::Entry entry;

  uintptr_t addr = reinterpret_cast<uintptr_t>(address);

  while (self_maps.GetNextEntry(&entry)) {
    if (entry.vma_start <= addr && addr < entry.vma_end) {
      *load_address = entry.vma_start;
      if (!entry.path) {
        LOG("Could not find ELF binary path!?");
        return false;
      }
      if (entry.path_len >= path_buffer_len) {
        LOG("ELF binary path too long: '%s'", entry.path);
        return false;
      }
      memcpy(path_buffer, entry.path, entry.path_len);
      path_buffer[entry.path_len] = '\0';
      return true;
    }
  }
  return false;
}

// Returns the current protection bit flags for the page holding a given
// address. Returns true on success, or false if the address is not mapped.
bool FindProtectionFlagsForAddress(void* address, int* prot_flags) {
  ProcMaps self_maps;
  ProcMaps::Entry entry;

  uintptr_t addr = reinterpret_cast<uintptr_t>(address);

  while (self_maps.GetNextEntry(&entry)) {
    if (entry.vma_start <= addr && addr < entry.vma_end) {
      *prot_flags = entry.prot_flags;
      return true;
    }
  }
  return false;
}

bool FindLoadAddressForFile(const char* file_name,
                            uintptr_t* load_address,
                            uintptr_t* load_offset) {
  size_t file_name_len = strlen(file_name);
  bool is_base_name = (strchr(file_name, '/') == NULL);
  ProcMaps self_maps;
  ProcMaps::Entry entry;

  while (self_maps.GetNextEntry(&entry)) {
    // Skip vDSO et al.
    if (entry.path_len == 0 || entry.path[0] == '[')
      continue;

    const char* entry_name = entry.path;
    size_t entry_len = entry.path_len;

    if (is_base_name) {
      const char* p = reinterpret_cast<const char*>(
          ::memrchr(entry.path, '/', entry.path_len));
      if (p) {
        entry_name = p + 1;
        entry_len = entry.path_len - (p - entry.path) - 1;
      }
    }

    if (file_name_len == entry_len &&
        !memcmp(file_name, entry_name, entry_len)) {
      *load_address = entry.vma_start;
      *load_offset = entry.load_offset;
      return true;
    }
  }

  return false;
}

}  // namespace crazy
