// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "ash/highlighter/highlighter_controller.h"
#include "ash/highlighter/highlighter_controller_test_api.h"
#include "ash/public/interfaces/voice_interaction_controller.mojom.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/system/palette/mock_palette_tool_delegate.h"
#include "ash/system/palette/palette_ids.h"
#include "ash/system/palette/palette_tool.h"
#include "ash/system/palette/tools/metalayer_mode.h"
#include "ash/system/tray/hover_highlight_view.h"
#include "ash/test/ash_test_base.h"
#include "ash/voice_interaction/voice_interaction_controller.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace ash {

namespace {

// Base class for all metalayer ash tests.
class MetalayerToolTest : public AshTestBase {
 public:
  MetalayerToolTest() = default;
  ~MetalayerToolTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    palette_tool_delegate_ = std::make_unique<MockPaletteToolDelegate>();
    tool_ = std::make_unique<MetalayerMode>(palette_tool_delegate_.get());
    highlighter_test_api_ = std::make_unique<HighlighterControllerTestApi>(
        Shell::Get()->highlighter_controller());
  }

  void TearDown() override {
    // This needs to be called first to reset the controller state before the
    // shell instance gets torn down.
    highlighter_test_api_.reset();
    tool_.reset();
    AshTestBase::TearDown();
  }

 protected:
  std::unique_ptr<HighlighterControllerTestApi> highlighter_test_api_;
  std::unique_ptr<MockPaletteToolDelegate> palette_tool_delegate_;
  std::unique_ptr<PaletteTool> tool_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MetalayerToolTest);
};

}  // namespace

// The metalayer tool is always visible, but only enabled when the user
// has enabled the metalayer AND the voice interaction framework is ready.
TEST_F(MetalayerToolTest, PaletteMenuState) {
  const mojom::VoiceInteractionState kStates[] = {
      mojom::VoiceInteractionState::NOT_READY,
      mojom::VoiceInteractionState::STOPPED,
      mojom::VoiceInteractionState::RUNNING};
  const mojom::AssistantAllowedState kAllowedStates[] = {
      mojom::AssistantAllowedState::ALLOWED,
      mojom::AssistantAllowedState::DISALLOWED_BY_ARC_DISALLOWED,
      mojom::AssistantAllowedState::DISALLOWED_BY_ARC_POLICY,
      mojom::AssistantAllowedState::DISALLOWED_BY_LOCALE,
      mojom::AssistantAllowedState::DISALLOWED_BY_FLAG,
      mojom::AssistantAllowedState::DISALLOWED_BY_NONPRIMARY_USER,
      mojom::AssistantAllowedState::DISALLOWED_BY_SUPERVISED_USER,
      mojom::AssistantAllowedState::DISALLOWED_BY_INCOGNITO,
  };
  const base::string16 kLoading(base::ASCIIToUTF16("loading"));

  // Iterate over every possible combination of states.
  for (mojom::VoiceInteractionState state : kStates) {
    for (mojom::AssistantAllowedState allowed_state : kAllowedStates) {
      for (int enabled = 0; enabled <= 1; enabled++) {
        for (int context = 0; context <= 1; context++) {
          const bool allowed =
              allowed_state == mojom::AssistantAllowedState::ALLOWED;
          const bool ready = state != mojom::VoiceInteractionState::NOT_READY;
          const bool selectable = allowed && enabled && context && ready;

          Shell::Get()->voice_interaction_controller()->NotifyStatusChanged(
              state);
          Shell::Get()->voice_interaction_controller()->NotifySettingsEnabled(
              enabled);
          Shell::Get()->voice_interaction_controller()->NotifyContextEnabled(
              context);
          Shell::Get()->voice_interaction_controller()->NotifyFeatureAllowed(
              allowed_state);

          std::unique_ptr<views::View> view =
              base::WrapUnique(tool_->CreateView());
          EXPECT_TRUE(view);
          EXPECT_EQ(selectable, view->enabled());

          const base::string16 label_text =
              static_cast<HoverHighlightView*>(view.get())
                  ->text_label()
                  ->text();

          const bool label_contains_loading =
              label_text.find(kLoading) != base::string16::npos;

          EXPECT_EQ(allowed && enabled && context && !ready,
                    label_contains_loading);
          tool_->OnViewDestroyed();
        }
      }
    }
  }
}

// Verifies that the metalayer enabled/disabled state propagates to the
// highlighter controller.
TEST_F(MetalayerToolTest, EnablingDisablingMetalayerEnablesDisablesController) {
  // Enabling the metalayer tool enables the highligher controller.
  // It should also hide the palette.
  EXPECT_CALL(*palette_tool_delegate_.get(), HidePalette());
  highlighter_test_api_->ResetEnabledState();
  tool_->OnEnable();
  EXPECT_TRUE(highlighter_test_api_->HandleEnabledStateChangedCalled());
  EXPECT_TRUE(highlighter_test_api_->enabled());
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());

  // Disabling the metalayer tool disables the highlighter controller.
  highlighter_test_api_->ResetEnabledState();
  tool_->OnDisable();
  EXPECT_TRUE(highlighter_test_api_->HandleEnabledStateChangedCalled());
  EXPECT_FALSE(highlighter_test_api_->enabled());
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());
}

// Verifies that disabling the metalayer support disables the tool.
TEST_F(MetalayerToolTest, MetalayerUnsupportedDisablesPaletteTool) {
  Shell::Get()->voice_interaction_controller()->NotifyStatusChanged(
      mojom::VoiceInteractionState::RUNNING);
  Shell::Get()->voice_interaction_controller()->NotifySettingsEnabled(true);
  Shell::Get()->voice_interaction_controller()->NotifyContextEnabled(true);

  // Disabling the user prefs individually should disable the tool.
  tool_->OnEnable();
  EXPECT_CALL(*palette_tool_delegate_.get(),
              DisableTool(PaletteToolId::METALAYER));
  Shell::Get()->voice_interaction_controller()->NotifySettingsEnabled(false);
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());
  Shell::Get()->voice_interaction_controller()->NotifySettingsEnabled(true);

  tool_->OnEnable();
  EXPECT_CALL(*palette_tool_delegate_.get(),
              DisableTool(PaletteToolId::METALAYER));
  Shell::Get()->voice_interaction_controller()->NotifyContextEnabled(false);
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());
  Shell::Get()->voice_interaction_controller()->NotifyContextEnabled(true);

  // Test VoiceInteractionState changes.
  tool_->OnEnable();

  // Changing the state from RUNNING to STOPPED and back should not disable the
  // tool.
  EXPECT_CALL(*palette_tool_delegate_.get(),
              DisableTool(PaletteToolId::METALAYER))
      .Times(0);
  Shell::Get()->voice_interaction_controller()->NotifyStatusChanged(
      mojom::VoiceInteractionState::STOPPED);
  Shell::Get()->voice_interaction_controller()->NotifyStatusChanged(
      mojom::VoiceInteractionState::RUNNING);
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());

  // Changing the state to NOT_READY should disable the tool.
  EXPECT_CALL(*palette_tool_delegate_.get(),
              DisableTool(PaletteToolId::METALAYER));
  Shell::Get()->voice_interaction_controller()->NotifyStatusChanged(
      mojom::VoiceInteractionState::NOT_READY);
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());
}

// Verifies that detaching the highlighter client disables the palette tool.
TEST_F(MetalayerToolTest, DetachingClientDisablesPaletteTool) {
  tool_->OnEnable();
  // If the client detaches, the tool should become disabled.
  EXPECT_CALL(*palette_tool_delegate_.get(),
              DisableTool(PaletteToolId::METALAYER));
  highlighter_test_api_->DetachClient();
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());

  // If the client attaches again, the tool should not become enabled.
  highlighter_test_api_->AttachClient();
  EXPECT_CALL(*palette_tool_delegate_.get(),
              EnableTool(PaletteToolId::METALAYER))
      .Times(0);
  testing::Mock::VerifyAndClearExpectations(palette_tool_delegate_.get());
}

}  // namespace ash
