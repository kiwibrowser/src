// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_DEVICE_CHANGE_OBSERVER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_DEVICE_CHANGE_OBSERVER_H_

#include "content/public/browser/render_view_host.h"
#include "ui/events/devices/input_device_event_observer.h"

// This class monitors input changes on all platforms.
//
// It is responsible to instantiate the various platforms observers
// and it gets notified whenever the input capabilities change. Whenever
// a change is detected the WebKit preferences are getting updated so the
// interactions media-queries can be updated.
namespace content {
class CONTENT_EXPORT InputDeviceChangeObserver
    : public ui::InputDeviceEventObserver {
 public:
  InputDeviceChangeObserver(RenderViewHost* rvh);
  ~InputDeviceChangeObserver() override;

  // InputDeviceEventObserver public overrides.
  void OnTouchscreenDeviceConfigurationChanged() override;
  void OnKeyboardDeviceConfigurationChanged() override;
  void OnMouseDeviceConfigurationChanged() override;
  void OnTouchpadDeviceConfigurationChanged() override;

 private:
  RenderViewHost* render_view_host_;
  void NotifyRenderViewHost();
  DISALLOW_COPY_AND_ASSIGN(InputDeviceChangeObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_DEVICE_CHANGE_OBSERVER_H_
