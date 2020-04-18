// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_data_consumer_handle.h"

#include <string.h>
#include <algorithm>
#include <memory>
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

WebDataConsumerHandle::WebDataConsumerHandle() {
  DCHECK(ThreadState::Current());
}

WebDataConsumerHandle::~WebDataConsumerHandle() {
  DCHECK(ThreadState::Current());
}

WebDataConsumerHandle::Result WebDataConsumerHandle::Reader::Read(
    void* data,
    size_t size,
    Flags flags,
    size_t* read_size) {
  *read_size = 0;
  const void* src = nullptr;
  size_t available;
  Result r = BeginRead(&src, flags, &available);
  if (r != WebDataConsumerHandle::kOk)
    return r;
  *read_size = std::min(available, size);
  memcpy(data, src, *read_size);
  return EndRead(*read_size);
}

}  // namespace blink
