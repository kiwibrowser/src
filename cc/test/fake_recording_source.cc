// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_recording_source.h"

#include "cc/test/fake_raster_source.h"

namespace cc {

FakeRecordingSource::FakeRecordingSource() : playback_allowed_event_(nullptr) {}

scoped_refptr<RasterSource> FakeRecordingSource::CreateRasterSource() const {
  return FakeRasterSource::CreateFromRecordingSourceWithWaitable(
      this, playback_allowed_event_);
}

bool FakeRecordingSource::EqualsTo(const FakeRecordingSource& other) {
  return size_ == other.size_ &&
         slow_down_raster_scale_factor_for_debug_ ==
             other.slow_down_raster_scale_factor_for_debug_ &&
         requires_clear_ == other.requires_clear_ &&
         is_solid_color_ == other.is_solid_color_ &&
         clear_canvas_with_debug_color_ ==
             other.clear_canvas_with_debug_color_ &&
         solid_color_ == other.solid_color_ &&
         background_color_ == other.background_color_;
}

}  // namespace cc
