// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/sql_memory_dump_provider.h"

#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "third_party/sqlite/sqlite3.h"

namespace sql {

// static
SqlMemoryDumpProvider* SqlMemoryDumpProvider::GetInstance() {
  return base::Singleton<
      SqlMemoryDumpProvider,
      base::LeakySingletonTraits<SqlMemoryDumpProvider>>::get();
}

SqlMemoryDumpProvider::SqlMemoryDumpProvider() = default;

SqlMemoryDumpProvider::~SqlMemoryDumpProvider() = default;

bool SqlMemoryDumpProvider::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  int memory_used = 0;
  int memory_high_water = 0;
  int status = sqlite3_status(SQLITE_STATUS_MEMORY_USED, &memory_used,
                              &memory_high_water, 1 /*resetFlag */);
  if (status != SQLITE_OK)
    return false;

  base::trace_event::MemoryAllocatorDump* dump =
      pmd->CreateAllocatorDump("sqlite");
  dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  memory_used);
  dump->AddScalar("malloc_high_wmark_size",
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  memory_high_water);

  int dummy_high_water = -1;
  int malloc_count = -1;
  status = sqlite3_status(SQLITE_STATUS_MALLOC_COUNT, &malloc_count,
                          &dummy_high_water, 0 /* resetFlag */);
  if (status == SQLITE_OK) {
    dump->AddScalar("malloc_count",
                    base::trace_event::MemoryAllocatorDump::kUnitsObjects,
                    malloc_count);
  }

  const char* system_allocator_name =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->system_allocator_pool_name();
  if (system_allocator_name) {
    pmd->AddSuballocation(dump->guid(), system_allocator_name);
  }
  return true;
}

}  // namespace sql
