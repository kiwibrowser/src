// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/SkDiscardableMemory_chrome.h"

#include <stddef.h>

#include <utility>

#include "base/memory/discardable_memory.h"
#include "base/memory/discardable_memory_allocator.h"

SkDiscardableMemoryChrome::~SkDiscardableMemoryChrome() = default;

bool SkDiscardableMemoryChrome::lock() {
  return discardable_->Lock();
}

void* SkDiscardableMemoryChrome::data() {
  return discardable_->data();
}

void SkDiscardableMemoryChrome::unlock() {
  discardable_->Unlock();
}

SkDiscardableMemoryChrome::SkDiscardableMemoryChrome(
    std::unique_ptr<base::DiscardableMemory> memory)
    : discardable_(std::move(memory)) {}

base::trace_event::MemoryAllocatorDump*
SkDiscardableMemoryChrome::CreateMemoryAllocatorDump(
    const char* name,
    base::trace_event::ProcessMemoryDump* pmd) const {
  return discardable_->CreateMemoryAllocatorDump(name, pmd);
}

SkDiscardableMemory* SkDiscardableMemory::Create(size_t bytes) {
  return new SkDiscardableMemoryChrome(
      base::DiscardableMemoryAllocator::GetInstance()
          ->AllocateLockedDiscardableMemory(bytes));
}
