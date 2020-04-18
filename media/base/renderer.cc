// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/renderer.h"

namespace media {

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::OnSelectedVideoTracksChanged(
    const std::vector<DemuxerStream*>& enabled_tracks,
    base::OnceClosure change_completed_cb) {
  std::move(change_completed_cb).Run();
  DLOG(WARNING) << "Track changes are not supported.";
}

void Renderer::OnEnabledAudioTracksChanged(
    const std::vector<DemuxerStream*>& enabled_tracks,
    base::OnceClosure change_completed_cb) {
  std::move(change_completed_cb).Run();
  DLOG(WARNING) << "Track changes are not supported.";
}

}  // namespace media
