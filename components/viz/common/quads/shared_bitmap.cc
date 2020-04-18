// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/quads/shared_bitmap.h"

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "base/memory/shared_memory_handle.h"
#include "base/numerics/safe_math.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "components/viz/common/resources/resource_format_utils.h"

namespace viz {

SharedBitmap::SharedBitmap(uint8_t* pixels,
                           const SharedBitmapId& id,
                           uint32_t sequence_number)
    : pixels_(pixels), id_(id), sequence_number_(sequence_number) {}

SharedBitmap::~SharedBitmap() {}

// static
SharedBitmapId SharedBitmap::GenerateId() {
  SharedBitmapId id;
  // Needs cryptographically-secure random numbers.
  base::RandBytes(id.name, sizeof(id.name));
  return id;
}

base::trace_event::MemoryAllocatorDumpGuid GetSharedBitmapGUIDForTracing(
    const SharedBitmapId& bitmap_id) {
  auto bitmap_id_hex = base::HexEncode(bitmap_id.name, sizeof(bitmap_id.name));
  return base::trace_event::MemoryAllocatorDumpGuid(
      base::StringPrintf("sharedbitmap-x-process/%s", bitmap_id_hex.c_str()));
}

}  // namespace viz
