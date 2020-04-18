// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/cast_dialog_view.h"

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/ui/media_router/cast_dialog_controller.h"
#include "chrome/browser/ui/media_router/cast_dialog_model.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/media_router/cast_dialog_sink_button.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/widget/widget.h"

using testing::_;
using testing::Invoke;
using testing::WithArg;

namespace media_router {

namespace {

UIMediaSink CreateAvailableSink() {
  UIMediaSink sink;
  sink.id = "sink_available";
  sink.state = UIMediaSinkState::AVAILABLE;
  sink.allowed_actions = static_cast<int>(UICastAction::CAST_TAB);
  return sink;
}

UIMediaSink CreateConnectedSink() {
  UIMediaSink sink;
  sink.id = "sink_connected";
  sink.state = UIMediaSinkState::CONNECTED;
  sink.allowed_actions = static_cast<int>(UICastAction::STOP);
  return sink;
}

CastDialogModel CreateModelWithSinks(std::vector<UIMediaSink> sinks) {
  CastDialogModel model;
  model.dialog_header = base::UTF8ToUTF16("Dialog header");
  model.media_sinks = std::move(sinks);
  return model;
}

}  // namespace

class MockCastDialogController : public CastDialogController {
 public:
  MOCK_METHOD1(AddObserver, void(CastDialogController::Observer* observer));
  MOCK_METHOD1(RemoveObserver, void(CastDialogController::Observer* observer));
  MOCK_METHOD2(StartCasting,
               void(const std::string& sink_id, MediaCastMode cast_mode));
  MOCK_METHOD1(StopCasting, void(const std::string& route_id));
};

class CastDialogViewTest : public ChromeViewsTestBase {
 protected:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    // Create an anchor for the dialog.
    views::Widget::InitParams params =
        CreateParams(views::Widget::InitParams::TYPE_WINDOW);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    anchor_widget_ = std::make_unique<views::Widget>();
    anchor_widget_->Init(params);
    anchor_widget_->Show();
  }

  void TearDown() override {
    anchor_widget_.reset();
    ChromeViewsTestBase::TearDown();
  }

  void InitializeDialogWithModel(const CastDialogModel& model) {
    EXPECT_CALL(controller_, AddObserver(_))
        .WillOnce(
            WithArg<0>(Invoke([this](CastDialogController::Observer* observer) {
              dialog_ = static_cast<CastDialogView*>(observer);
            })));
    CastDialogView::ShowDialog(anchor_widget_->GetContentsView(), &controller_);

    dialog_->OnModelUpdated(model);
  }

  void SelectSinkAtIndex(int index) {
    ui::MouseEvent mouse_event(ui::ET_MOUSE_PRESSED, gfx::Point(0, 0),
                               gfx::Point(0, 0), ui::EventTimeForNow(), 0, 0);
    dialog_->ButtonPressed(dialog_->sink_buttons_for_test()[1], mouse_event);
  }

  std::unique_ptr<views::Widget> anchor_widget_;
  MockCastDialogController controller_;
  CastDialogView* dialog_ = nullptr;
};

// Flaky on Mac. https://crbug.com/843599
#if defined(OS_MACOSX)
#define MAYBE_ShowAndHideDialog DISABLED_ShowAndHideDialog
#else
#define MAYBE_ShowAndHideDialog ShowAndHideDialog
#endif
TEST_F(CastDialogViewTest, MAYBE_ShowAndHideDialog) {
  EXPECT_FALSE(CastDialogView::IsShowing());
  EXPECT_EQ(nullptr, CastDialogView::GetCurrentDialogWidget());

  EXPECT_CALL(controller_, AddObserver(_));
  CastDialogView::ShowDialog(anchor_widget_->GetContentsView(), &controller_);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(CastDialogView::IsShowing());
  EXPECT_NE(nullptr, CastDialogView::GetCurrentDialogWidget());

  EXPECT_CALL(controller_, RemoveObserver(_));
  CastDialogView::HideDialog();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(CastDialogView::IsShowing());
  EXPECT_EQ(nullptr, CastDialogView::GetCurrentDialogWidget());
}

TEST_F(CastDialogViewTest, PopulateDialog) {
  CastDialogModel model = CreateModelWithSinks({CreateAvailableSink()});
  InitializeDialogWithModel(model);

  EXPECT_TRUE(dialog_->ShouldShowCloseButton());
  EXPECT_EQ(model.dialog_header, dialog_->GetWindowTitle());
  EXPECT_EQ(ui::DIALOG_BUTTON_OK, dialog_->GetDialogButtons());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_MEDIA_ROUTER_START_CASTING_BUTTON),
            dialog_->GetDialogButtonLabel(ui::DIALOG_BUTTON_OK));
}

TEST_F(CastDialogViewTest, ChooseSinks) {
  CastDialogModel model =
      CreateModelWithSinks({CreateAvailableSink(), CreateConnectedSink()});
  InitializeDialogWithModel(model);

  // Activate the main action button. The sink at index 0 should be selected by
  // default.
  EXPECT_CALL(controller_, StartCasting(model.media_sinks[0].id, TAB_MIRROR));
  dialog_->Accept();

  // The label on the main action button should be updated when a different sink
  // is chosen.
  SelectSinkAtIndex(1);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_MEDIA_ROUTER_STOP_CASTING_BUTTON),
            dialog_->GetDialogButtonLabel(ui::DIALOG_BUTTON_OK));
  EXPECT_CALL(controller_, StopCasting(model.media_sinks[0].route_id));
  dialog_->Accept();
}

TEST_F(CastDialogViewTest, UpdateModel) {
  CastDialogModel model =
      CreateModelWithSinks({CreateAvailableSink(), CreateConnectedSink()});
  InitializeDialogWithModel(model);
  SelectSinkAtIndex(1);
  model.media_sinks[1].state = UIMediaSinkState::AVAILABLE;
  model.media_sinks[1].allowed_actions =
      static_cast<int>(UICastAction::CAST_TAB);
  dialog_->OnModelUpdated(model);

  // Sink selection should be retained across a model update.
  EXPECT_CALL(controller_, StartCasting(model.media_sinks[1].id, TAB_MIRROR));
  dialog_->Accept();
}

}  // namespace media_router
