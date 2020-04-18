// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_EMULATION_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_EMULATION_HANDLER_H_

#include "base/macros.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/browser/devtools/protocol/emulation.h"
#include "third_party/blink/public/web/web_device_emulation_params.h"

namespace content {

class RenderFrameHostImpl;
class WebContentsImpl;

namespace protocol {

class EmulationHandler : public DevToolsDomainHandler,
                         public Emulation::Backend {
 public:
  EmulationHandler();
  ~EmulationHandler() override;

  void Wire(UberDispatcher* dispatcher) override;
  void SetRenderer(int process_host_id,
                   RenderFrameHostImpl* frame_host) override;

  Response Disable() override;

  Response SetGeolocationOverride(Maybe<double> latitude,
                                  Maybe<double> longitude,
                                  Maybe<double> accuracy) override;
  Response ClearGeolocationOverride() override;

  Response SetEmitTouchEventsForMouse(
      bool enabled,
      Maybe<std::string> configuration) override;

  Response CanEmulate(bool* result) override;
  Response SetDeviceMetricsOverride(
      int width,
      int height,
      double device_scale_factor,
      bool mobile,
      Maybe<double> scale,
      Maybe<int> screen_widget,
      Maybe<int> screen_height,
      Maybe<int> position_x,
      Maybe<int> position_y,
      Maybe<bool> dont_set_visible_size,
      Maybe<Emulation::ScreenOrientation> screen_orientation,
      Maybe<protocol::Page::Viewport> viewport) override;
  Response ClearDeviceMetricsOverride() override;

  Response SetVisibleSize(int width, int height) override;

  blink::WebDeviceEmulationParams GetDeviceEmulationParams();
  void SetDeviceEmulationParams(const blink::WebDeviceEmulationParams& params);

  bool device_emulation_enabled() { return device_emulation_enabled_; }

 private:
  WebContentsImpl* GetWebContents();
  void UpdateTouchEventEmulationState();
  void UpdateDeviceEmulationState();

  bool touch_emulation_enabled_;
  std::string touch_emulation_configuration_;

  bool device_emulation_enabled_;
  blink::WebDeviceEmulationParams device_emulation_params_;

  RenderFrameHostImpl* host_;

  DISALLOW_COPY_AND_ASSIGN(EmulationHandler);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_EMULATION_HANDLER_H_
