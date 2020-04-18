// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/frame_evictor.h"

#include "base/logging.h"

namespace viz {

FrameEvictor::FrameEvictor(FrameEvictorClient* client)
    : client_(client), has_frame_(false), visible_(false) {}

FrameEvictor::~FrameEvictor() {
  DiscardedFrame();
}

void FrameEvictor::SwappedFrame(bool visible) {
  visible_ = visible;
  has_frame_ = true;
  FrameEvictionManager::GetInstance()->AddFrame(this, visible);
}

void FrameEvictor::DiscardedFrame() {
  FrameEvictionManager::GetInstance()->RemoveFrame(this);
  has_frame_ = false;
}

void FrameEvictor::SetVisible(bool visible) {
  if (visible_ == visible)
    return;
  visible_ = visible;
  if (has_frame_) {
    if (visible) {
      LockFrame();
    } else {
      UnlockFrame();
    }
  }
}

void FrameEvictor::LockFrame() {
  DCHECK(has_frame_);
  FrameEvictionManager::GetInstance()->LockFrame(this);
}

void FrameEvictor::UnlockFrame() {
  DCHECK(has_frame_);
  FrameEvictionManager::GetInstance()->UnlockFrame(this);
}

void FrameEvictor::EvictCurrentFrame() {
  client_->EvictDelegatedFrame();
}

}  // namespace viz
