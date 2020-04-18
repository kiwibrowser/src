// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CLIENT_H_
#define UI_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CLIENT_H_

#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "ui/compositor/compositor_export.h"

namespace ui {

class COMPOSITOR_EXPORT ExternalBeginFrameClient {
 public:
  virtual ~ExternalBeginFrameClient() {}

  virtual void OnDisplayDidFinishFrame(const viz::BeginFrameAck& ack) = 0;
  virtual void OnNeedsExternalBeginFrames(bool needs_begin_frames) = 0;
};

}  // namespace ui

#endif  // UI_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CLIENT_H_
