// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cursor/cursor_data.h"

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/cursor.h"

namespace ui {

CursorData::CursorData()
    : cursor_type_(CursorType::kNull), scale_factor_(0.0f) {}

CursorData::CursorData(CursorType type)
    : cursor_type_(type), scale_factor_(0.0f) {}

CursorData::CursorData(const gfx::Point& hotspot_point,
                       const std::vector<SkBitmap>& cursor_frames,
                       float scale_factor,
                       const base::TimeDelta& frame_delay)
    : cursor_type_(CursorType::kCustom),
      frame_delay_(frame_delay),
      scale_factor_(scale_factor),
      hotspot_(hotspot_point),
      cursor_frames_(cursor_frames) {
  for (SkBitmap& bitmap : cursor_frames_)
    generator_ids_.push_back(bitmap.getGenerationID());
}

CursorData::CursorData(const CursorData& cursor) = default;

CursorData::CursorData(CursorData&& cursor) = default;

CursorData::~CursorData() {}

CursorData& CursorData::operator=(const CursorData& cursor) = default;

CursorData& CursorData::operator=(CursorData&& cursor) = default;

bool CursorData::IsType(CursorType cursor_type) const {
  return cursor_type_ == cursor_type;
}

bool CursorData::IsSameAs(const CursorData& rhs) const {
  return cursor_type_ == rhs.cursor_type_ && frame_delay_ == rhs.frame_delay_ &&
         hotspot_ == rhs.hotspot_ && scale_factor_ == rhs.scale_factor_ &&
         generator_ids_ == rhs.generator_ids_;
}

}  // namespace ui
