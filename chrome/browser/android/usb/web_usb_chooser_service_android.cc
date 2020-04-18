// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/usb/web_usb_chooser_service_android.h"

#include <utility>

#include "chrome/browser/ui/android/usb_chooser_dialog_android.h"
#include "chrome/browser/usb/usb_chooser_controller.h"
#include "content/public/browser/browser_thread.h"

WebUsbChooserServiceAndroid::WebUsbChooserServiceAndroid(
    content::RenderFrameHost* render_frame_host)
    : WebUsbChooserService(render_frame_host) {}

WebUsbChooserServiceAndroid::~WebUsbChooserServiceAndroid() {}

void WebUsbChooserServiceAndroid::ShowChooser(
    std::unique_ptr<UsbChooserController> controller) {
  dialog_ = UsbChooserDialogAndroid::Create(
      render_frame_host(), std::move(controller),
      base::BindOnce(&WebUsbChooserServiceAndroid::OnDialogClosed,
                     base::Unretained(this)));
}

void WebUsbChooserServiceAndroid::OnDialogClosed() {
  dialog_.reset();
}
