// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/blink_gc_memory_dump_provider.h"

#include "base/trace_event/process_memory_dump.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

TEST(BlinkGCDumpProviderTest, MemoryDump) {
  base::trace_event::MemoryDumpArgs args = {
      base::trace_event::MemoryDumpLevelOfDetail::DETAILED};
  std::unique_ptr<base::trace_event::ProcessMemoryDump> dump(
      new base::trace_event::ProcessMemoryDump(args));
  BlinkGCMemoryDumpProvider::Instance()->OnMemoryDump(args, dump.get());
  DCHECK(dump->GetAllocatorDump("blink_gc"));
  DCHECK(dump->GetAllocatorDump("blink_gc/allocated_objects"));
}

}  // namespace blink
