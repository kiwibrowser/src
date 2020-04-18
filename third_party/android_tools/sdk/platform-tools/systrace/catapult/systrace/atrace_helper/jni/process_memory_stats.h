// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PROCESS_MEMORY_STATS_H_
#define PROCESS_MEMORY_STATS_H_

#include <stdint.h>

#include <map>
#include <memory>

// Reads process memory stats from /proc/pid/{statm,smaps}.
class ProcessMemoryStats {
 public:
  struct MmapInfo {
    char mapped_file[128] = {};
    char prot_flags[5] = {};
    uint64_t start_addr = 0;
    uint64_t end_addr = 0;
    uint64_t virt_kb = 0;
    uint64_t pss_kb = 0;  // Proportional Set Size.
    uint64_t rss_kb = 0;  // Resident Set Size.
    uint64_t private_clean_kb = 0;
    uint64_t private_dirty_kb = 0;
    uint64_t shared_clean_kb = 0;
    uint64_t shared_dirty_kb = 0;
    uint64_t swapped_kb = 0;
  };

  explicit ProcessMemoryStats(int pid);

  bool ReadLightStats();
  bool ReadFullStats();

  // Available after ReadLightStats().
  uint64_t virt_kb() const { return virt_kb_; }
  uint64_t rss_kb() const { return rss_kb_; }

  // Available after ReadFullStats().
  uint64_t private_clean_kb() const { return private_clean_kb_; }
  uint64_t private_dirty_kb() const { return private_dirty_kb_; }
  uint64_t shared_clean_kb() const { return shared_clean_kb_; }
  uint64_t shared_dirty_kb() const { return shared_dirty_kb_; }
  uint64_t swapped_kb() const { return swapped_kb_; }

 private:
  ProcessMemoryStats(const ProcessMemoryStats&) = delete;
  void operator=(const ProcessMemoryStats&) = delete;

  const int pid_;

  // Light stats.
  uint64_t virt_kb_ = 0;
  uint64_t rss_kb_ = 0;

  // Full stats.
  uint64_t pss_kb_ = 0;
  uint64_t private_clean_kb_ = 0;
  uint64_t private_dirty_kb_ = 0;
  uint64_t shared_clean_kb_ = 0;
  uint64_t shared_dirty_kb_ = 0;
  uint64_t swapped_kb_ = 0;

  std::map<uintptr_t, std::unique_ptr<MmapInfo>> mmaps_;
};

#endif  // PROCESS_MEMORY_STATS_H_
