// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/media_router/media_sink_with_cast_modes.h"

namespace media_router {

MediaSinkWithCastModes::MediaSinkWithCastModes(const MediaSink& sink)
    : sink(sink) {}

MediaSinkWithCastModes::MediaSinkWithCastModes(
    const MediaSinkWithCastModes& other) = default;

MediaSinkWithCastModes::~MediaSinkWithCastModes() {}

bool MediaSinkWithCastModes::Equals(const MediaSinkWithCastModes& other) const {
  return sink.Equals(other.sink) && cast_modes == other.cast_modes;
}

}  // namespace media_router
