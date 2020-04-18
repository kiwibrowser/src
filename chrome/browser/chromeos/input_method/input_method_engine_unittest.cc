// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/input_method_engine.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/chromeos/input_method/input_method_configuration.h"
#include "chrome/browser/chromeos/input_method/mock_input_method_manager_impl.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/input_method/input_method_engine_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ime/chromeos/extension_ime_util.h"
#include "ui/base/ime/chromeos/mock_component_extension_ime_manager_delegate.h"
#include "ui/base/ime/ime_bridge.h"
#include "ui/base/ime/ime_engine_handler_interface.h"
#include "ui/base/ime/mock_ime_input_context_handler.h"
#include "ui/base/ime/text_input_flags.h"
#include "ui/gfx/geometry/rect.h"

using input_method::InputMethodEngineBase;

namespace chromeos {

namespace input_method {
namespace {

const char kTestExtensionId[] = "mppnpdlheglhdfmldimlhpnegondlapf";
const char kTestExtensionId2[] = "dmpipdbjkoajgdeppkffbjhngfckdloi";
const char kTestImeComponentId[] = "test_engine_id";

enum CallsBitmap {
  NONE = 0U,
  ACTIVATE = 1U,
  DEACTIVATED = 2U,
  ONFOCUS = 4U,
  ONBLUR = 8U,
  ONCOMPOSITIONBOUNDSCHANGED = 16U,
  RESET = 32U
};

void InitInputMethod() {
  ComponentExtensionIMEManager* comp_ime_manager =
      new ComponentExtensionIMEManager;
  MockComponentExtIMEManagerDelegate* delegate =
      new MockComponentExtIMEManagerDelegate;

  ComponentExtensionIME ext1;
  ext1.id = kTestExtensionId;

  ComponentExtensionEngine ext1_engine1;
  ext1_engine1.engine_id = kTestImeComponentId;
  ext1_engine1.language_codes.push_back("en-US");
  ext1_engine1.layouts.push_back("us");
  ext1.engines.push_back(ext1_engine1);

  std::vector<ComponentExtensionIME> ime_list;
  ime_list.push_back(ext1);
  delegate->set_ime_list(ime_list);
  comp_ime_manager->Initialize(
      std::unique_ptr<ComponentExtensionIMEManagerDelegate>(delegate));

  MockInputMethodManagerImpl* manager = new MockInputMethodManagerImpl;
  manager->SetComponentExtensionIMEManager(
      std::unique_ptr<ComponentExtensionIMEManager>(comp_ime_manager));
  InitializeForTesting(manager);
}

class TestObserver : public InputMethodEngineBase::Observer {
 public:
  TestObserver() : calls_bitmap_(NONE) {}
  ~TestObserver() override {}

  void OnActivate(const std::string& engine_id) override {
    calls_bitmap_ |= ACTIVATE;
    engine_id_ = engine_id;
  }
  void OnDeactivated(const std::string& engine_id) override {
    calls_bitmap_ |= DEACTIVATED;
    engine_id_ = engine_id;
  }
  void OnFocus(
      const ui::IMEEngineHandlerInterface::InputContext& context) override {
    calls_bitmap_ |= ONFOCUS;
  }
  void OnBlur(int context_id) override { calls_bitmap_ |= ONBLUR; }
  bool IsInterestedInKeyEvent() const override { return true; }
  void OnKeyEvent(
      const std::string& engine_id,
      const InputMethodEngineBase::KeyboardEvent& event,
      ui::IMEEngineHandlerInterface::KeyEventDoneCallback key_data) override {}
  void OnInputContextUpdate(
      const ui::IMEEngineHandlerInterface::InputContext& context) override {}
  void OnCandidateClicked(
      const std::string& engine_id,
      int candidate_id,
      InputMethodEngineBase::MouseButtonEvent button) override {}
  void OnMenuItemActivated(const std::string& engine_id,
                           const std::string& menu_id) override {}
  void OnSurroundingTextChanged(const std::string& engine_id,
                                const std::string& text,
                                int cursor_pos,
                                int anchor_pos,
                                int offset) override {}
  void OnCompositionBoundsChanged(
      const std::vector<gfx::Rect>& bounds) override {
    calls_bitmap_ |= ONCOMPOSITIONBOUNDSCHANGED;
  }
  void OnReset(const std::string& engine_id) override {
    calls_bitmap_ |= RESET;
    engine_id_ = engine_id;
  }

  unsigned char GetCallsBitmapAndReset() {
    unsigned char ret = calls_bitmap_;
    calls_bitmap_ = NONE;
    return ret;
  }

  std::string GetEngineIdAndReset() {
    std::string engine_id{engine_id_};
    engine_id_.clear();
    return engine_id;
  }

 private:
  unsigned char calls_bitmap_;
  std::string engine_id_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class InputMethodEngineTest : public testing::Test {
 public:
  InputMethodEngineTest() : observer_(NULL), input_view_("inputview.html") {
    languages_.push_back("en-US");
    layouts_.push_back("us");
    InitInputMethod();
    ui::IMEBridge::Initialize();
    mock_ime_input_context_handler_.reset(new ui::MockIMEInputContextHandler());
    ui::IMEBridge::Get()->SetInputContextHandler(
        mock_ime_input_context_handler_.get());
  }
  ~InputMethodEngineTest() override {
    ui::IMEBridge::Get()->SetInputContextHandler(NULL);
    engine_.reset();
    Shutdown();
  }

 protected:
  void CreateEngine(bool whitelisted) {
    engine_.reset(new InputMethodEngine());
    observer_ = new TestObserver();
    std::unique_ptr<InputMethodEngineBase::Observer> observer_ptr(observer_);
    engine_->Initialize(std::move(observer_ptr),
                        whitelisted ? kTestExtensionId : kTestExtensionId2,
                        ProfileManager::GetActiveUserProfile());
  }

  void FocusIn(ui::TextInputType input_type) {
    ui::IMEEngineHandlerInterface::InputContext input_context(
        input_type, ui::TEXT_INPUT_MODE_DEFAULT, ui::TEXT_INPUT_FLAG_NONE,
        ui::TextInputClient::FOCUS_REASON_OTHER,
        false /* should_do_learning */);
    engine_->FocusIn(input_context);
    ui::IMEBridge::Get()->SetCurrentInputContext(input_context);
  }

  std::unique_ptr<InputMethodEngine> engine_;

  TestObserver* observer_;
  std::vector<std::string> languages_;
  std::vector<std::string> layouts_;
  GURL options_page_;
  GURL input_view_;

  std::unique_ptr<ui::MockIMEInputContextHandler>
      mock_ime_input_context_handler_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InputMethodEngineTest);
};

}  // namespace

TEST_F(InputMethodEngineTest, TestSwitching) {
  CreateEngine(false);
  // Enable/disable with focus.
  FocusIn(ui::TEXT_INPUT_TYPE_URL);
  EXPECT_EQ(NONE, observer_->GetCallsBitmapAndReset());
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  // Enable/disable without focus.
  engine_->FocusOut();
  EXPECT_EQ(NONE, observer_->GetCallsBitmapAndReset());
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  // Focus change when enabled.
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->FocusOut();
  EXPECT_EQ(ONBLUR, observer_->GetCallsBitmapAndReset());
  // Focus change when disabled.
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  FocusIn(ui::TEXT_INPUT_TYPE_TEXT);
  EXPECT_EQ(NONE, observer_->GetCallsBitmapAndReset());
  engine_->FocusOut();
  EXPECT_EQ(NONE, observer_->GetCallsBitmapAndReset());
}

TEST_F(InputMethodEngineTest, TestSwitching_Password_3rd_Party) {
  CreateEngine(false);
  // Enable/disable with focus.
  FocusIn(ui::TEXT_INPUT_TYPE_PASSWORD);
  EXPECT_EQ(NONE, observer_->GetCallsBitmapAndReset());
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  // Focus change when enabled.
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->FocusOut();
  EXPECT_EQ(ONBLUR, observer_->GetCallsBitmapAndReset());
  FocusIn(ui::TEXT_INPUT_TYPE_PASSWORD);
  EXPECT_EQ(ONFOCUS, observer_->GetCallsBitmapAndReset());
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
}

TEST_F(InputMethodEngineTest, TestSwitching_Password_Whitelisted) {
  CreateEngine(true);
  // Enable/disable with focus.
  FocusIn(ui::TEXT_INPUT_TYPE_PASSWORD);
  EXPECT_EQ(NONE, observer_->GetCallsBitmapAndReset());
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  // Focus change when enabled.
  engine_->Enable(kTestImeComponentId);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
  engine_->FocusOut();
  EXPECT_EQ(ONBLUR, observer_->GetCallsBitmapAndReset());
  FocusIn(ui::TEXT_INPUT_TYPE_PASSWORD);
  EXPECT_EQ(ONFOCUS, observer_->GetCallsBitmapAndReset());
  engine_->Disable();
  EXPECT_EQ(DEACTIVATED, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
}

// Tests input.ime.onReset API.
TEST_F(InputMethodEngineTest, TestReset) {
  CreateEngine(false);
  // Enables the extension with focus.
  engine_->Enable(kTestImeComponentId);
  FocusIn(ui::TEXT_INPUT_TYPE_URL);
  EXPECT_EQ(ACTIVATE | ONFOCUS, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());

  // Resets the engine.
  engine_->Reset();
  EXPECT_EQ(RESET, observer_->GetCallsBitmapAndReset());
  EXPECT_EQ(kTestImeComponentId, observer_->GetEngineIdAndReset());
}

TEST_F(InputMethodEngineTest, TestHistograms) {
  CreateEngine(true);
  FocusIn(ui::TEXT_INPUT_TYPE_TEXT);
  engine_->Enable(kTestImeComponentId);
  std::vector<InputMethodEngineBase::SegmentInfo> segments;
  int context = engine_->GetCotextIdForTesting();
  std::string error;
  base::HistogramTester histograms;
  engine_->SetComposition(context, "test", 0, 0, 0, segments, NULL);
  engine_->CommitText(context, "input", &error);
  engine_->SetComposition(context, "test", 0, 0, 0, segments, NULL);
  engine_->CommitText(context,
                      "\xE5\x85\xA5\xE5\x8A\x9B",  // 2 UTF-8 characters
                      &error);
  engine_->SetComposition(context, "test", 0, 0, 0, segments, NULL);
  engine_->CommitText(context, "input\xE5\x85\xA5\xE5\x8A\x9B", &error);
  histograms.ExpectTotalCount("InputMethod.CommitLength", 3);
  histograms.ExpectBucketCount("InputMethod.CommitLength", 5, 1);
  histograms.ExpectBucketCount("InputMethod.CommitLength", 2, 1);
  histograms.ExpectBucketCount("InputMethod.CommitLength", 7, 1);
}

TEST_F(InputMethodEngineTest, TestCompositionBoundsChanged) {
  CreateEngine(true);
  // Enable/disable with focus.
  std::vector<gfx::Rect> rects;
  rects.push_back(gfx::Rect());
  engine_->SetCompositionBounds(rects);
  EXPECT_EQ(ONCOMPOSITIONBOUNDSCHANGED, observer_->GetCallsBitmapAndReset());
}

}  // namespace input_method
}  // namespace chromeos
