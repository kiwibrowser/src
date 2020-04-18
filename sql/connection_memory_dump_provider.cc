// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/connection_memory_dump_provider.h"

#include <inttypes.h>

#include "base/strings/stringprintf.h"
#include "base/trace_event/process_memory_dump.h"
#include "third_party/sqlite/sqlite3.h"

namespace sql {

ConnectionMemoryDumpProvider::ConnectionMemoryDumpProvider(
    sqlite3* db,
    const std::string& name)
    : db_(db), connection_name_(name) {}

ConnectionMemoryDumpProvider::~ConnectionMemoryDumpProvider() = default;

void ConnectionMemoryDumpProvider::ResetDatabase() {
  base::AutoLock lock(lock_);
  db_ = nullptr;
}

bool ConnectionMemoryDumpProvider::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  if (args.level_of_detail == base::trace_event::MemoryDumpLevelOfDetail::LIGHT)
    return true;

  int cache_size = 0;
  int schema_size = 0;
  int statement_size = 0;
  if (!GetDbMemoryUsage(&cache_size, &schema_size, &statement_size)) {
    return false;
  }

  base::trace_event::MemoryAllocatorDump* dump =
      pmd->CreateAllocatorDump(FormatDumpName());
  dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  cache_size + schema_size + statement_size);
  dump->AddScalar("cache_size",
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  cache_size);
  dump->AddScalar("schema_size",
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  schema_size);
  dump->AddScalar("statement_size",
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  statement_size);
  return true;
}

bool ConnectionMemoryDumpProvider::ReportMemoryUsage(
    base::trace_event::ProcessMemoryDump* pmd,
    const std::string& dump_name) {
  int cache_size = 0;
  int schema_size = 0;
  int statement_size = 0;
  if (!GetDbMemoryUsage(&cache_size, &schema_size, &statement_size))
    return false;

  auto* mad = pmd->CreateAllocatorDump(dump_name);
  mad->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                 base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                 cache_size + schema_size + statement_size);
  pmd->AddSuballocation(mad->guid(), FormatDumpName());

  return true;
}

bool ConnectionMemoryDumpProvider::GetDbMemoryUsage(int* cache_size,
                                                    int* schema_size,
                                                    int* statement_size) {
  // Lock is acquired here so that db_ is not reset in ResetDatabase when
  // collecting stats.
  base::AutoLock lock(lock_);
  if (!db_) {
    return false;
  }

  // The high water mark is not tracked for the following usages.
  int dummy_int;
  int status = sqlite3_db_status(db_, SQLITE_DBSTATUS_CACHE_USED, cache_size,
                                 &dummy_int, 0 /* resetFlag */);
  DCHECK_EQ(SQLITE_OK, status);
  status = sqlite3_db_status(db_, SQLITE_DBSTATUS_SCHEMA_USED, schema_size,
                             &dummy_int, 0 /* resetFlag */);
  DCHECK_EQ(SQLITE_OK, status);
  status = sqlite3_db_status(db_, SQLITE_DBSTATUS_STMT_USED, statement_size,
                             &dummy_int, 0 /* resetFlag */);
  DCHECK_EQ(SQLITE_OK, status);

  return true;
}

std::string ConnectionMemoryDumpProvider::FormatDumpName() const {
  return base::StringPrintf(
      "sqlite/%s_connection/0x%" PRIXPTR,
      connection_name_.empty() ? "Unknown" : connection_name_.c_str(),
      reinterpret_cast<uintptr_t>(this));
}

}  // namespace sql
