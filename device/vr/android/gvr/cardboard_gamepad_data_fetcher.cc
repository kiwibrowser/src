// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/android/gvr/cardboard_gamepad_data_fetcher.h"

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "device/gamepad/public/cpp/gamepads.h"
#include "device/vr/android/gvr/cardboard_gamepad_data_provider.h"

namespace device {

namespace {

void CopyToUString(UChar* dest, size_t dest_length, base::string16 src) {
  static_assert(sizeof(base::string16::value_type) == sizeof(UChar),
                "Mismatched string16/WebUChar size.");

  const size_t str_to_copy = std::min(src.size(), dest_length - 1);
  memcpy(dest, src.data(), str_to_copy * sizeof(base::string16::value_type));
  dest[str_to_copy] = 0;
}

}  // namespace

CardboardGamepadDataFetcher::Factory::Factory(
    CardboardGamepadDataProvider* data_provider,
    unsigned int display_id)
    : data_provider_(data_provider), display_id_(display_id) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

CardboardGamepadDataFetcher::Factory::~Factory() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

std::unique_ptr<GamepadDataFetcher>
CardboardGamepadDataFetcher::Factory::CreateDataFetcher() {
  return std::make_unique<CardboardGamepadDataFetcher>(data_provider_,
                                                       display_id_);
}

GamepadSource CardboardGamepadDataFetcher::Factory::source() {
  return GAMEPAD_SOURCE_CARDBOARD;
}

CardboardGamepadDataFetcher::CardboardGamepadDataFetcher(
    CardboardGamepadDataProvider* data_provider,
    unsigned int display_id)
    : display_id_(display_id) {
  // Called on UI thread.
  DVLOG(1) << __FUNCTION__ << "=" << this;
  data_provider->RegisterCardboardGamepadDataFetcher(this);
}

CardboardGamepadDataFetcher::~CardboardGamepadDataFetcher() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

GamepadSource CardboardGamepadDataFetcher::source() {
  return GAMEPAD_SOURCE_CARDBOARD;
}

void CardboardGamepadDataFetcher::OnAddedToProvider() {
  PauseHint(false);
}

void CardboardGamepadDataFetcher::SetGamepadData(CardboardGamepadData data) {
  // Called from UI thread.
  gamepad_data_ = data;
}

void CardboardGamepadDataFetcher::GetGamepadData(bool devices_changed_hint) {
  // Called from gamepad polling thread.
  PadState* state = GetPadState(0);
  if (!state)
    return;

  CardboardGamepadData provided_data = gamepad_data_;

  Gamepad& pad = state->data;
  if (!state->is_initialized) {
    state->is_initialized = true;
    // This is the first time we've seen this device, so do some one-time
    // initialization
    pad.connected = true;
    pad.is_xr = true;
    CopyToUString(pad.id, Gamepad::kIdLengthCap,
                  base::UTF8ToUTF16("Cardboard Button"));
    CopyToUString(pad.mapping, Gamepad::kMappingLengthCap,
                  base::UTF8ToUTF16(""));
    pad.buttons_length = 1;
    pad.axes_length = 0;

    pad.display_id = display_id_;

    pad.hand = GamepadHand::kNone;
  }

  pad.timestamp = provided_data.timestamp;

  bool pressed = provided_data.is_screen_touching;
  pad.buttons[0].touched = pressed;
  pad.buttons[0].pressed = pressed;
  pad.buttons[0].value = pressed ? 1.0f : 0.0f;
  pad.pose.not_null = false;
}

void CardboardGamepadDataFetcher::PauseHint(bool paused) {}

}  // namespace device
