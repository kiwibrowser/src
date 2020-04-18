// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_source.h"

#include <string>

#include "chrome/common/media_router/media_source_helper.h"

namespace media_router {

MediaSource::MediaSource(const MediaSource::Id& source_id) : id_(source_id) {
  GURL url(source_id);
  if (IsValidPresentationUrl(url))
    url_ = url;
}

MediaSource::MediaSource(const GURL& presentation_url)
    : id_(presentation_url.spec()), url_(presentation_url) {}

MediaSource::~MediaSource() {}

MediaSource::Id MediaSource::id() const {
  return id_;
}

GURL MediaSource::url() const {
  return url_;
}

bool MediaSource::operator==(const MediaSource& other) const {
  return id_ == other.id();
}

bool MediaSource::operator<(const MediaSource& other) const {
  return id_ < other.id();
}

std::string MediaSource::ToString() const {
  return "MediaSource[" + id_ + "]";
}

MediaSource::MediaSource() {}

}  // namespace media_router
