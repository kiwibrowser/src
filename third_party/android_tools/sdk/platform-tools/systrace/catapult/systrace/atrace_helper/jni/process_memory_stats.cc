// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "process_memory_stats.h"

#include <stdio.h>
#include <stdlib.h>

#include <memory>

#include "file_utils.h"
#include "logging.h"

namespace {
const int kKbPerPage = 4;

// Takes a C string buffer and chunks it into lines without creating any
// copies. It modifies the original buffer, by replacing \n with \0.
class LineReader {
 public:
  LineReader(char* buf, size_t size) : ptr_(buf), end_(buf + size) {}

  const char* NextLine() {
    if (ptr_ >= end_)
      return nullptr;
    const char* cur = ptr_;
    char* next = strchr(ptr_, '\n');
    if (next) {
      *next = '\0';
      ptr_ = next + 1;
    } else {
      ptr_ = end_;
    }
    return cur;
  }

 private:
  char* ptr_;
  char* end_;
};

bool ReadSmapsMetric(const char* line, const char* metric, uint64_t* res) {
  if (strncmp(line, metric, strlen(metric)))
    return false;
  line = strchr(line, ':');
  if (!line)
    return false;
  *res = strtoull(line + 1, nullptr, 10);
  return true;
}

}  // namespace

ProcessMemoryStats::ProcessMemoryStats(int pid) : pid_(pid) {}

bool ProcessMemoryStats::ReadLightStats() {
  char buf[64];
  if (file_utils::ReadProcFile(pid_, "statm", buf, sizeof(buf)) <= 0)
    return false;
  uint32_t vm_size_pages;
  uint32_t rss_pages;
  int res = sscanf(buf, "%u %u", &vm_size_pages, &rss_pages);
  CHECK(res == 2);
  rss_kb_ = rss_pages * kKbPerPage;
  virt_kb_ = vm_size_pages * kKbPerPage;
  return true;
}

bool ProcessMemoryStats::ReadFullStats() {
  const size_t kBufSize = 32u * 1024 * 1024;
  std::unique_ptr<char[]> buf(new char[kBufSize]);
  ssize_t rsize = file_utils::ReadProcFile(pid_, "smaps", &buf[0], kBufSize);
  if (rsize <= 0)
    return false;
  MmapInfo* last_mmap_entry = nullptr;
  std::unique_ptr<MmapInfo> new_mmap(new MmapInfo());
  CHECK(mmaps_.empty());
  CHECK(rss_kb_ == 0);

  // Iterate over all lines in /proc/PID/smaps.
  LineReader rd(&buf[0], rsize);
  for (const char* line = rd.NextLine(); line; line = rd.NextLine()) {
    // Check if the current line is the beginning of a new mmaps entry, e.g.:
    // be7f7000-be818000 rw-p 00000000 00:00 0          [stack]
    // Note that the mapped file name ([stack]) is optional and won't be
    // present
    // on anonymous memory maps (hence res >= 3 below).
    int res = sscanf(
        line, "%llx-%llx %4s %*llx %*[:0-9a-f] %*[0-9a-f]%*[ \t]%128[^\n]",
        &new_mmap->start_addr, &new_mmap->end_addr, new_mmap->prot_flags,
        new_mmap->mapped_file);
    if (res >= 3) {
      last_mmap_entry = new_mmap.get();
      CHECK(new_mmap->end_addr >= new_mmap->start_addr);
      new_mmap->virt_kb = (new_mmap->end_addr - new_mmap->start_addr) / 1024;
      if (res == 3)
        new_mmap->mapped_file[0] = '\0';
      virt_kb_ += new_mmap->virt_kb;
      mmaps_[new_mmap->start_addr] = std::move(new_mmap);
      new_mmap.reset(new MmapInfo());
    } else {
      // The current line is a metrics line within a mmap entry, e.g.:
      // Size:   4 kB
      uint64_t size = 0;
      CHECK(last_mmap_entry);
      if (ReadSmapsMetric(line, "Rss:", &size)) {
        last_mmap_entry->rss_kb = size;
        rss_kb_ += size;
      } else if (ReadSmapsMetric(line, "Pss:", &size)) {
        last_mmap_entry->pss_kb = size;
        pss_kb_ += size;
      } else if (ReadSmapsMetric(line, "Swap:", &size)) {
        last_mmap_entry->swapped_kb = size;
        swapped_kb_ += size;
      } else if (ReadSmapsMetric(line, "Shared_Clean:", &size)) {
        last_mmap_entry->shared_clean_kb = size;
        shared_clean_kb_ += size;
      } else if (ReadSmapsMetric(line, "Shared_Dirty:", &size)) {
        last_mmap_entry->shared_dirty_kb = size;
        shared_dirty_kb_ += size;
      } else if (ReadSmapsMetric(line, "Private_Clean:", &size)) {
        last_mmap_entry->private_clean_kb = size;
        private_clean_kb_ += size;
      } else if (ReadSmapsMetric(line, "Private_Dirty:", &size)) {
        last_mmap_entry->private_dirty_kb = size;
        private_dirty_kb_ += size;
      }
    }
  }
  return true;
}
