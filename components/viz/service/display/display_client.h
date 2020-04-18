// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_DISPLAY_CLIENT_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_DISPLAY_CLIENT_H_

#include "components/viz/common/quads/render_pass.h"

namespace gfx {
struct CALayerParams;
}  // namespace gfx

namespace ui {
class LatencyInfo;
}  // namespace ui

namespace viz {

class DisplayClient {
 public:
  virtual ~DisplayClient() {}
  virtual void DisplayOutputSurfaceLost() = 0;
  virtual void DisplayWillDrawAndSwap(bool will_draw_and_swap,
                                      const RenderPassList& render_passes) = 0;
  virtual void DisplayDidDrawAndSwap() = 0;
  virtual void DisplayDidReceiveCALayerParams(
      const gfx::CALayerParams& ca_layer_params) = 0;

  // Notifies that a swap has occured after some latency info with snapshot
  // component reached the display.
  virtual void DidSwapAfterSnapshotRequestReceived(
      const std::vector<ui::LatencyInfo>& latency_info) = 0;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_DISPLAY_CLIENT_H_
