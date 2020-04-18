// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/model.h"

namespace vr {

namespace {

bool IsOpaqueUiMode(UiMode mode) {
  switch (mode) {
    case kModeBrowsing:
    case kModeFullscreen:
    case kModeWebVr:
    case kModeWebVrAutopresented:
    case kModeVoiceSearch:
    case kModeEditingOmnibox:
    case kModeTabsView:
      return true;
    case kModeRepositionWindow:
    case kModeModalPrompt:
    case kModeVoiceSearchListening:
      return false;
  }
  NOTREACHED();
  return true;
}

}  // namespace

Model::Model() = default;
Model::~Model() = default;

const ColorScheme& Model::color_scheme() const {
  ColorScheme::Mode mode = ColorScheme::kModeNormal;
  if (incognito)
    mode = ColorScheme::kModeIncognito;
  if (fullscreen_enabled())
    mode = ColorScheme::kModeFullscreen;
  return ColorScheme::GetColorScheme(mode);
}

void Model::push_mode(UiMode mode) {
  if (!ui_modes.empty() && ui_modes.back() == mode)
    return;
  ui_modes.push_back(mode);
}

void Model::pop_mode() {
  pop_mode(ui_modes.back());
}

void Model::pop_mode(UiMode mode) {
  if (ui_modes.empty() || ui_modes.back() != mode)
    return;
  // We should always have a mode to be in when we're clearing a mode.
  DCHECK_GE(ui_modes.size(), 2u);
  ui_modes.pop_back();
}

void Model::toggle_mode(UiMode mode) {
  if (!ui_modes.empty() && ui_modes.back() == mode) {
    pop_mode(mode);
    return;
  }
  push_mode(mode);
}

UiMode Model::get_mode() const {
  return ui_modes.back();
}

UiMode Model::get_last_opaque_mode() const {
  for (auto iter = ui_modes.rbegin(); iter != ui_modes.rend(); ++iter) {
    if (IsOpaqueUiMode(*iter))
      return *iter;
  }
  DCHECK(false) << "get_last_opaque_mode should only be called with at least "
                   "one opaque mode.";
  return kModeBrowsing;
}

bool Model::has_mode_in_stack(UiMode mode) const {
  for (auto stacked_mode : ui_modes) {
    if (mode == stacked_mode)
      return true;
  }
  return false;
}

bool Model::browsing_enabled() const {
  return !web_vr_enabled();
}

bool Model::default_browsing_enabled() const {
  return get_last_opaque_mode() == kModeBrowsing;
}

bool Model::voice_search_enabled() const {
  return get_last_opaque_mode() == kModeVoiceSearch;
}

bool Model::omnibox_editing_enabled() const {
  return get_last_opaque_mode() == kModeEditingOmnibox;
}

bool Model::editing_enabled() const {
  return editing_input || editing_web_input;
}

bool Model::fullscreen_enabled() const {
  return get_last_opaque_mode() == kModeFullscreen;
}

bool Model::web_vr_enabled() const {
  return get_last_opaque_mode() == kModeWebVr ||
         get_last_opaque_mode() == kModeWebVrAutopresented;
}

bool Model::web_vr_autopresentation_enabled() const {
  return get_last_opaque_mode() == kModeWebVrAutopresented;
}

bool Model::reposition_window_enabled() const {
  return ui_modes.back() == kModeRepositionWindow;
}

bool Model::reposition_window_permitted() const {
  return !editing_input && !editing_web_input &&
         active_modal_prompt_type == kModalPromptTypeNone &&
         !hosted_platform_ui.hosted_ui_enabled;
}

}  // namespace vr
