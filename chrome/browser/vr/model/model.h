// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_MODEL_H_

#include <memory>

#include "chrome/browser/vr/model/capturing_state_model.h"
#include "chrome/browser/vr/model/color_scheme.h"
#include "chrome/browser/vr/model/controller_model.h"
#include "chrome/browser/vr/model/hosted_platform_ui.h"
#include "chrome/browser/vr/model/modal_prompt_type.h"
#include "chrome/browser/vr/model/omnibox_suggestions.h"
#include "chrome/browser/vr/model/platform_toast.h"
#include "chrome/browser/vr/model/reticle_model.h"
#include "chrome/browser/vr/model/speech_recognition_model.h"
#include "chrome/browser/vr/model/tab_model.h"
#include "chrome/browser/vr/model/text_input_info.h"
#include "chrome/browser/vr/model/toolbar_state.h"
#include "chrome/browser/vr/model/ui_mode.h"
#include "chrome/browser/vr/model/web_vr_model.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "ui/gfx/transform.h"

namespace vr {

struct Model {
  Model();
  ~Model();

  // VR browsing state.
  bool browsing_disabled = false;
  bool loading = false;
  float load_progress = 0.0f;
  bool incognito = false;
  bool in_cct = false;
  bool can_navigate_back = false;
  bool can_navigate_forward = false;
  ToolbarState toolbar_state;
  std::vector<OmniboxSuggestion> omnibox_suggestions;
  SpeechRecognitionModel speech;
  const ColorScheme& color_scheme() const;
  gfx::Transform projection_matrix;
  unsigned int content_texture_id = 0;
  unsigned int content_overlay_texture_id = 0;
  bool content_overlay_texture_non_empty = false;
  UiElementRenderer::TextureLocation content_location =
      UiElementRenderer::kTextureLocationLocal;
  UiElementRenderer::TextureLocation content_overlay_location =
      UiElementRenderer::kTextureLocationLocal;
  bool waiting_for_background = false;
  bool background_loaded = false;
  bool supports_selection = true;
  bool needs_keyboard_update = false;
  bool overflow_menu_enabled = false;
  bool standalone_vr_device = false;
  std::vector<TabModel> regular_tabs;
  std::vector<TabModel> incognito_tabs;
  bool incognito_tabs_view_selected = false;
  bool create_tabs_view = false;
  float floor_height = 0.0f;

  // WebVR state.
  WebVrModel web_vr;

  std::vector<UiMode> ui_modes;
  void push_mode(UiMode mode);
  void pop_mode();
  void pop_mode(UiMode mode);
  void toggle_mode(UiMode mode);
  UiMode get_mode() const;
  UiMode get_last_opaque_mode() const;
  bool has_mode_in_stack(UiMode mode) const;
  bool browsing_enabled() const;
  bool default_browsing_enabled() const;
  bool voice_search_enabled() const;
  bool omnibox_editing_enabled() const;
  bool editing_enabled() const;
  bool fullscreen_enabled() const;
  bool web_vr_enabled() const;
  bool web_vr_autopresentation_enabled() const;
  bool reposition_window_enabled() const;
  bool reposition_window_permitted() const;

  // Focused text state.
  bool editing_input = false;
  bool editing_web_input = false;
  // Editable text field state.
  EditedText omnibox_text_field_info;
  EditedText web_input_text_field_info;

  // Controller state.
  ControllerModel controller;
  ReticleModel reticle;

  // State affecting both VR browsing and WebVR.
  ModalPromptType active_modal_prompt_type = kModalPromptTypeNone;
  CapturingStateModel capturing_state;
  bool experimental_features_enabled = false;
  bool skips_redraw_when_not_dirty = false;
  bool exiting_vr = false;
  HostedPlatformUi hosted_platform_ui;

  std::unique_ptr<const PlatformToast> platform_toast;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_MODEL_H_
