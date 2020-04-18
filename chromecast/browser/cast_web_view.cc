// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_web_view.h"

namespace chromecast {

std::unique_ptr<content::BluetoothChooser>
CastWebView::Delegate::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  return nullptr;
}

CastWebView::CastWebView() {}

CastWebView::~CastWebView() {
  for (Observer& observer : observer_list_) {
    observer.OnPageDestroyed(this);
  }
}

void CastWebView::AddObserver(CastWebView::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void CastWebView::RemoveObserver(CastWebView::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

}  // namespace chromecast
