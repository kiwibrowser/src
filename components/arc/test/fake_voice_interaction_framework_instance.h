// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_FAKE_VOICE_INTERACTION_FRAMEWORK_INSTANCE_H_
#define COMPONENTS_ARC_TEST_FAKE_VOICE_INTERACTION_FRAMEWORK_INSTANCE_H_

#include <stddef.h>

#include "components/arc/common/voice_interaction_framework.mojom.h"
#include "ui/gfx/geometry/rect.h"

namespace arc {

class FakeVoiceInteractionFrameworkInstance
    : public mojom::VoiceInteractionFrameworkInstance {
 public:
  FakeVoiceInteractionFrameworkInstance();
  ~FakeVoiceInteractionFrameworkInstance() override;

  // mojom::VoiceInteractionFrameworkInstance overrides:
  void InitDeprecated(
      mojom::VoiceInteractionFrameworkHostPtr host_ptr) override;
  void Init(mojom::VoiceInteractionFrameworkHostPtr host_ptr,
            InitCallback callback) override;
  void StartVoiceInteractionSession(bool homescreen_is_active) override;
  void ToggleVoiceInteractionSession(bool homescreen_is_active) override;
  void StartVoiceInteractionSessionForRegion(const gfx::Rect& region) override;
  void SetMetalayerVisibility(bool visible) override;
  void SetVoiceInteractionEnabled(
      bool enable,
      SetVoiceInteractionEnabledCallback callback) override;
  void SetVoiceInteractionContextEnabled(bool enable) override;
  void StartVoiceInteractionSetupWizard() override;
  void ShowVoiceInteractionSettings() override;
  void GetVoiceInteractionSettings(
      GetVoiceInteractionSettingsCallback callback) override;

  void FlushMojoForTesting();

  size_t start_session_count() const { return start_session_count_; }
  size_t toggle_session_count() const { return toggle_session_count_; }
  size_t setup_wizard_count() const { return setup_wizard_count_; }
  size_t show_settings_count() const { return show_settings_count_; }
  size_t set_metalayer_visibility_count() const {
    return set_metalayer_visibility_count_;
  }
  bool metalayer_visible() const { return metalayer_visible_; }
  size_t start_session_for_region_count() const {
    return start_session_for_region_count_;
  }
  const gfx::Rect& selected_region() const { return selected_region_; }

  void ResetCounters() {
    start_session_count_ = 0u;
    toggle_session_count_ = 0u;
    setup_wizard_count_ = 0u;
    show_settings_count_ = 0u;
    set_metalayer_visibility_count_ = 0u;
    start_session_for_region_count_ = 0u;
  }

 private:
  size_t start_session_count_ = 0u;
  size_t toggle_session_count_ = 0u;
  size_t setup_wizard_count_ = 0u;
  size_t show_settings_count_ = 0u;
  size_t set_metalayer_visibility_count_ = 0u;
  bool metalayer_visible_ = true;
  size_t start_session_for_region_count_ = 0u;
  gfx::Rect selected_region_;
  mojom::VoiceInteractionFrameworkHostPtr host_;
  arc::mojom::VoiceInteractionState state_ =
      arc::mojom::VoiceInteractionState::STOPPED;

  DISALLOW_COPY_AND_ASSIGN(FakeVoiceInteractionFrameworkInstance);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_VOICE_INTERACTION_FRAMEWORK_INSTANCE_H_
