// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_FRAME_EVICTOR_H_
#define COMPONENTS_VIZ_CLIENT_FRAME_EVICTOR_H_

#include "base/macros.h"
#include "components/viz/client/frame_eviction_manager.h"

namespace viz {

class FrameEvictorClient {
 public:
  virtual ~FrameEvictorClient() {}
  virtual void EvictDelegatedFrame() = 0;
};

class VIZ_CLIENT_EXPORT FrameEvictor : public FrameEvictionManagerClient {
 public:
  // |client| must outlive |this|.
  explicit FrameEvictor(FrameEvictorClient* client);
  ~FrameEvictor() override;

  void SwappedFrame(bool visible);
  void DiscardedFrame();
  void SetVisible(bool visible);
  void LockFrame();
  void UnlockFrame();
  bool HasFrame() { return has_frame_; }

 private:
  // FrameEvictionManagerClient implementation.
  void EvictCurrentFrame() override;

  FrameEvictorClient* client_;
  bool has_frame_;
  bool visible_;

  DISALLOW_COPY_AND_ASSIGN(FrameEvictor);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_FRAME_EVICTOR_H_
