// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/desktop_capture/desktop_media_picker_views.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/webrtc/fake_desktop_media_list.h"
#include "chrome/browser/ui/views/desktop_capture/desktop_media_list_view.h"
#include "chrome/browser/ui/views/desktop_capture/desktop_media_source_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/web_modal/test_web_contents_modal_dialog_host.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/events/event_utils.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/test/scoped_views_test_helper.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"
#include "ui/views/window/dialog_delegate.h"

using content::DesktopMediaID;

namespace views {

const std::vector<DesktopMediaID::Type> kSourceTypes = {
    DesktopMediaID::TYPE_SCREEN, DesktopMediaID::TYPE_WINDOW,
    DesktopMediaID::TYPE_WEB_CONTENTS};

class DesktopMediaPickerViewsTest : public testing::Test {
 public:
  DesktopMediaPickerViewsTest() {}
  ~DesktopMediaPickerViewsTest() override {}

  void SetUp() override {
    test_helper_.test_views_delegate()->set_layout_provider(
        ChromeLayoutProvider::CreateLayoutProvider());

    std::vector<std::unique_ptr<DesktopMediaList>> source_lists;
    for (auto type : kSourceTypes) {
      media_lists_[type] = new FakeDesktopMediaList(type);
      source_lists.push_back(
          std::unique_ptr<FakeDesktopMediaList>(media_lists_[type]));
    }

    base::string16 app_name = base::ASCIIToUTF16("foo");

    picker_views_.reset(new DesktopMediaPickerViews());
    DesktopMediaPicker::Params picker_params;
    picker_params.context = test_helper_.GetContext();
    picker_params.app_name = app_name;
    picker_params.target_name = app_name;
    picker_params.request_audio = true;
    picker_views_->Show(picker_params, std::move(source_lists),
                        base::Bind(&DesktopMediaPickerViewsTest::OnPickerDone,
                                   base::Unretained(this)));
  }

  void TearDown() override {
    if (GetPickerDialogView()) {
      EXPECT_CALL(*this, OnPickerDone(content::DesktopMediaID()));
      GetPickerDialogView()->GetWidget()->CloseNow();
    }
  }

  DesktopMediaPickerDialogView* GetPickerDialogView() const {
    return picker_views_->GetDialogViewForTesting();
  }

  bool ClickSourceTypeButton(DesktopMediaID::Type source_type) {
    int index =
        GetPickerDialogView()->GetIndexOfSourceTypeForTesting(source_type);

    if (index == -1)
      return false;

    GetPickerDialogView()->GetPaneForTesting()->SelectTabAt(index);
    return true;
  }

  MOCK_METHOD1(OnPickerDone, void(content::DesktopMediaID));

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  views::ScopedViewsTestHelper test_helper_;
  std::map<DesktopMediaID::Type, FakeDesktopMediaList*> media_lists_;
  std::unique_ptr<DesktopMediaPickerViews> picker_views_;
};

TEST_F(DesktopMediaPickerViewsTest, DoneCallbackCalledWhenWindowClosed) {
  EXPECT_CALL(*this, OnPickerDone(content::DesktopMediaID()));

  GetPickerDialogView()->GetWidget()->Close();
  base::RunLoop().RunUntilIdle();
}

// Flaky on Windows. https://crbug.com/644614
#if defined(OS_WIN)
#define MAYBE_DoneCallbackCalledOnOkButtonPressed DISABLED_DoneCallbackCalledOnOkButtonPressed
#else
#define MAYBE_DoneCallbackCalledOnOkButtonPressed DoneCallbackCalledOnOkButtonPressed
#endif

TEST_F(DesktopMediaPickerViewsTest, MAYBE_DoneCallbackCalledOnOkButtonPressed) {
  const DesktopMediaID kFakeId(DesktopMediaID::TYPE_WINDOW, 222);
  EXPECT_CALL(*this, OnPickerDone(kFakeId));

  media_lists_[DesktopMediaID::TYPE_WINDOW]->AddSourceByFullMediaID(kFakeId);
  GetPickerDialogView()->GetCheckboxForTesting()->SetChecked(true);

  EXPECT_FALSE(
      GetPickerDialogView()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_WINDOW));

  GetPickerDialogView()->GetMediaSourceViewForTesting(0)->OnFocus();

  EXPECT_TRUE(
      GetPickerDialogView()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  GetPickerDialogView()->GetDialogClientView()->AcceptWindow();
  base::RunLoop().RunUntilIdle();
}

// Verifies that a MediaSourceView is selected with mouse left click and
// original selected MediaSourceView gets unselected.
TEST_F(DesktopMediaPickerViewsTest, SelectMediaSourceViewOnSingleClick) {
  for (auto source_type : kSourceTypes) {
    EXPECT_TRUE(ClickSourceTypeButton(source_type));
    media_lists_[source_type]->AddSourceByFullMediaID(
        DesktopMediaID(source_type, 0));
    media_lists_[source_type]->AddSourceByFullMediaID(
        DesktopMediaID(source_type, 1));

    DesktopMediaSourceView* source_view_0 =
        GetPickerDialogView()->GetMediaSourceViewForTesting(0);

    DesktopMediaSourceView* source_view_1 =
        GetPickerDialogView()->GetMediaSourceViewForTesting(1);

    // By default, the first screen is selected, but not for other sharing
    // types.
    EXPECT_EQ(source_type == DesktopMediaID::TYPE_SCREEN,
              source_view_0->is_selected());
    EXPECT_FALSE(source_view_1->is_selected());

    // Source view 0 is selected with mouse click.
    ui::MouseEvent press(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);

    GetPickerDialogView()->GetMediaSourceViewForTesting(0)->OnMousePressed(
        press);

    EXPECT_TRUE(source_view_0->is_selected());
    EXPECT_FALSE(source_view_1->is_selected());

    // Source view 1 is selected and source view 0 is unselected with mouse
    // click.
    GetPickerDialogView()->GetMediaSourceViewForTesting(1)->OnMousePressed(
        press);

    EXPECT_FALSE(source_view_0->is_selected());
    EXPECT_TRUE(source_view_1->is_selected());
  }
}

TEST_F(DesktopMediaPickerViewsTest, DoneCallbackCalledOnDoubleClick) {
  const DesktopMediaID kFakeId(DesktopMediaID::TYPE_WEB_CONTENTS, 222);
  EXPECT_CALL(*this, OnPickerDone(kFakeId));

  media_lists_[DesktopMediaID::TYPE_WEB_CONTENTS]->AddSourceByFullMediaID(
      kFakeId);
  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_WEB_CONTENTS));
  GetPickerDialogView()->GetCheckboxForTesting()->SetChecked(false);

  ui::MouseEvent double_click(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                              ui::EventTimeForNow(),
                              ui::EF_LEFT_MOUSE_BUTTON | ui::EF_IS_DOUBLE_CLICK,
                              ui::EF_LEFT_MOUSE_BUTTON);

  GetPickerDialogView()->GetMediaSourceViewForTesting(0)->OnMousePressed(
      double_click);
  base::RunLoop().RunUntilIdle();
}

TEST_F(DesktopMediaPickerViewsTest, DoneCallbackCalledOnDoubleTap) {
  const DesktopMediaID kFakeId(DesktopMediaID::TYPE_SCREEN, 222);
  EXPECT_CALL(*this, OnPickerDone(kFakeId));

  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_SCREEN));
  GetPickerDialogView()->GetCheckboxForTesting()->SetChecked(false);

  media_lists_[DesktopMediaID::TYPE_SCREEN]->AddSourceByFullMediaID(kFakeId);
  ui::GestureEventDetails details(ui::ET_GESTURE_TAP);
  details.set_tap_count(2);
  ui::GestureEvent double_tap(10, 10, 0, base::TimeTicks(), details);

  GetPickerDialogView()->GetMediaSourceViewForTesting(0)->OnGestureEvent(
      &double_tap);
  base::RunLoop().RunUntilIdle();
}

TEST_F(DesktopMediaPickerViewsTest, CancelButtonAlwaysEnabled) {
  EXPECT_TRUE(
      GetPickerDialogView()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_CANCEL));
}

// Verifies that the MediaSourceView is added or removed when |media_list_| is
// updated.
TEST_F(DesktopMediaPickerViewsTest, AddAndRemoveMediaSource) {
  for (auto source_type : kSourceTypes) {
    EXPECT_TRUE(ClickSourceTypeButton(source_type));
    // No media source at first.
    EXPECT_FALSE(GetPickerDialogView()->GetMediaSourceViewForTesting(0));

    for (int i = 0; i < 3; ++i) {
      media_lists_[source_type]->AddSourceByFullMediaID(
          DesktopMediaID(source_type, i));
      EXPECT_TRUE(GetPickerDialogView()->GetMediaSourceViewForTesting(i));
    }

    for (int i = 2; i >= 0; --i) {
      media_lists_[source_type]->RemoveSource(i);
      EXPECT_FALSE(GetPickerDialogView()->GetMediaSourceViewForTesting(i));
    }
  }
}

// Verifies that focusing the MediaSourceView marks it selected and the
// original selected MediaSourceView gets unselected.
TEST_F(DesktopMediaPickerViewsTest, FocusMediaSourceViewToSelect) {
  for (auto source_type : kSourceTypes) {
    ClickSourceTypeButton(source_type);
    media_lists_[source_type]->AddSourceByFullMediaID(
        DesktopMediaID(source_type, 0));
    media_lists_[source_type]->AddSourceByFullMediaID(
        DesktopMediaID(source_type, 1));

    DesktopMediaSourceView* source_view_0 =
        GetPickerDialogView()->GetMediaSourceViewForTesting(0);

    DesktopMediaSourceView* source_view_1 =
        GetPickerDialogView()->GetMediaSourceViewForTesting(1);

    source_view_0->OnFocus();
    EXPECT_TRUE(source_view_0->is_selected());

    // Removing the focus does not undo the selection.
    source_view_0->OnBlur();
    EXPECT_TRUE(source_view_0->is_selected());

    source_view_1->OnFocus();
    EXPECT_FALSE(source_view_0->is_selected());
    EXPECT_TRUE(source_view_1->is_selected());
  }
}

TEST_F(DesktopMediaPickerViewsTest, OkButtonDisabledWhenNoSelection) {
  for (auto source_type : kSourceTypes) {
    EXPECT_TRUE(ClickSourceTypeButton(source_type));
    media_lists_[source_type]->AddSourceByFullMediaID(
        DesktopMediaID(source_type, 111));

    GetPickerDialogView()->GetMediaSourceViewForTesting(0)->OnFocus();
    EXPECT_TRUE(
        GetPickerDialogView()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

    media_lists_[source_type]->RemoveSource(0);
    EXPECT_FALSE(
        GetPickerDialogView()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));
  }
}

// Verifies that the MediaListView gets the initial focus.
TEST_F(DesktopMediaPickerViewsTest, ListViewHasInitialFocus) {
  EXPECT_TRUE(GetPickerDialogView()->GetMediaListViewForTesting()->HasFocus());
}

// Verifies the visible status of audio checkbox.
TEST_F(DesktopMediaPickerViewsTest, AudioCheckboxState) {
  bool expect_value = false;
  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_SCREEN));
#if defined(OS_WIN) || defined(USE_CRAS)
  expect_value = true;
#endif
  EXPECT_EQ(expect_value,
            GetPickerDialogView()->GetCheckboxForTesting()->visible());

  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_WINDOW));
  EXPECT_FALSE(GetPickerDialogView()->GetCheckboxForTesting()->visible());

  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_WEB_CONTENTS));
  EXPECT_TRUE(GetPickerDialogView()->GetCheckboxForTesting()->visible());
}

// Verifies that audio share information is recorded in the ID if the checkbox
// is checked.
TEST_F(DesktopMediaPickerViewsTest, DoneWithAudioShare) {
  DesktopMediaID originId(DesktopMediaID::TYPE_WEB_CONTENTS, 222);
  DesktopMediaID returnId = originId;
  returnId.audio_share = true;

  // This matches the real workflow that when a source is generated in
  // media_list, its |audio_share| bit is not set. The bit is set by the picker
  // UI if the audio checkbox is checked.
  EXPECT_CALL(*this, OnPickerDone(returnId));
  media_lists_[DesktopMediaID::TYPE_WEB_CONTENTS]->AddSourceByFullMediaID(
      originId);

  EXPECT_TRUE(ClickSourceTypeButton(DesktopMediaID::TYPE_WEB_CONTENTS));
  GetPickerDialogView()->GetCheckboxForTesting()->SetChecked(true);
  GetPickerDialogView()->GetMediaSourceViewForTesting(0)->OnFocus();

  GetPickerDialogView()->GetDialogClientView()->AcceptWindow();
  base::RunLoop().RunUntilIdle();
}

}  // namespace views
