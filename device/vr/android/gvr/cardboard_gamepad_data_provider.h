// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_ANDROID_CARDBOARD_GAMEPAD_DATA_PROVIDER_H
#define DEVICE_VR_ANDROID_CARDBOARD_GAMEPAD_DATA_PROVIDER_H

namespace device {

class CardboardGamepadDataFetcher;

// Filled in by vr_shell and consumed by CardboardGamepadDataFetcher.
struct CardboardGamepadData {
  CardboardGamepadData() : timestamp(0), is_screen_touching(false) {}
  int64_t timestamp;
  bool is_screen_touching;
};

// This class exposes Cardboard controller (screen touch) data to the gamepad
// API. Data is reported by VrShell, which implements the
// CardboardGamepadDataProvider interface.
//
// More specifically, here's the lifecycle, assuming VrShell
// implements GvrGamepadDataProvider:
//
// - VrShell creates CardboardGamepadDataFetcherFactory during initialization if
//  a cardboard is in use, or when WebVR presentation starts.
//
// - CardboardGamepadDataFetcherFactory creates CardboardGamepadDataFetcher.
//
// - CardboardGamepadDataFetcher registers itself with VrShell via
//   VrShell::RegisterCardboardGamepadDataFetcher.
//
// - While presenting, VrShell::OnTouch calls
//   CardboardGamepadDataFetcher->SetGamepadData to push button state,
//   CardboardGamepadDataFetcher::GetGamepadData returns this when polled.
//
// - VrShell starts executing its destructor or WebVR presentation ends.
//
// - VrShell destructor unregisters CardboardGamepadDataFetcherFactory.
//
// - CardboardGamepadDataFetcherFactory destructor destroys
// CardboardGamepadDataFetcher.
//
class CardboardGamepadDataProvider {
 public:
  // Called by the gamepad data fetcher constructor to register itself
  // for receiving data via SetGamepadData. The fetcher must remain
  // alive while the provider is calling SetGamepadData on it.
  virtual void RegisterCardboardGamepadDataFetcher(
      CardboardGamepadDataFetcher*) = 0;
};

}  // namespace device
#endif  // DEVICE_VR_ANDROID_CARDBOARD_GAMEPAD_DATA_PROVIDER_H
