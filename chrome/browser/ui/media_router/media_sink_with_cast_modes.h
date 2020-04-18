// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_SINK_WITH_CAST_MODES_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_SINK_WITH_CAST_MODES_H_

#include <set>

#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "chrome/common/media_router/media_sink.h"

namespace media_router {

// Contains information on a MediaSink and the set of cast modes it is
// compatible with. This should be interpreted under the context of a
// QueryResultManager which contains a mapping from MediaCastMode to
// MediaSource.
struct MediaSinkWithCastModes {
  explicit MediaSinkWithCastModes(const MediaSink& sink);
  MediaSinkWithCastModes(const MediaSinkWithCastModes& other);
  ~MediaSinkWithCastModes();

  MediaSink sink;
  CastModeSet cast_modes;

  bool Equals(const MediaSinkWithCastModes& other) const;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_SINK_WITH_CAST_MODES_H_
