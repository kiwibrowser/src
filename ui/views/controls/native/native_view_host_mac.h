// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_MAC_H_
#define UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_MAC_H_

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "ui/views/controls/native/native_view_host_wrapper.h"
#include "ui/views/views_export.h"

namespace views {

class NativeViewHost;

// Mac implementation of NativeViewHostWrapper.
class NativeViewHostMac : public NativeViewHostWrapper {
 public:
  explicit NativeViewHostMac(NativeViewHost* host);
  ~NativeViewHostMac() override;

  // Overridden from NativeViewHostWrapper:
  void AttachNativeView() override;
  void NativeViewDetaching(bool destroyed) override;
  void AddedToWidget() override;
  void RemovedFromWidget() override;
  bool SetCornerRadius(int corner_radius) override;
  void InstallClip(int x, int y, int w, int h) override;
  bool HasInstalledClip() override;
  void UninstallClip() override;
  void ShowWidget(int x, int y, int w, int h, int native_w, int native_h)
      override;
  void HideWidget() override;
  void SetFocus() override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  gfx::NativeCursor GetCursor(int x, int y) override;

 private:
  // Our associated NativeViewHost. Owns this.
  NativeViewHost* host_;

  // Retain the native view as it may be destroyed at an unpredictable time.
  base::scoped_nsobject<NSView> native_view_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewHostMac);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_MAC_H_
