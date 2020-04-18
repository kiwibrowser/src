// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/cast_dialog_sink_button.h"

#include "chrome/browser/ui/media_router/ui_media_sink.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/button.h"

namespace media_router {

namespace {

class MockButtonListener : public views::ButtonListener {
 public:
  MockButtonListener() = default;
  ~MockButtonListener() override = default;

  MOCK_METHOD2(ButtonPressed,
               void(views::Button* sender, const ui::Event& event));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockButtonListener);
};

class CastDialogSinkButtonTest : public ChromeViewsTestBase {
 public:
  CastDialogSinkButtonTest() = default;
  ~CastDialogSinkButtonTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastDialogSinkButtonTest);
};

void CheckActionTextForState(UIMediaSinkState state,
                             base::string16 expected_text) {
  UIMediaSink sink;
  sink.state = state;
  CastDialogSinkButton button(nullptr, sink);
  EXPECT_EQ(expected_text, button.GetActionText());
}

}  // namespace

TEST_F(CastDialogSinkButtonTest, GetActionText) {
  // TODO(crbug.com/826089): Determine what the text should be for other states.
  CheckActionTextForState(
      UIMediaSinkState::AVAILABLE,
      l10n_util::GetStringUTF16(IDS_MEDIA_ROUTER_START_CASTING_BUTTON));
  CheckActionTextForState(
      UIMediaSinkState::CONNECTED,
      l10n_util::GetStringUTF16(IDS_MEDIA_ROUTER_STOP_CASTING_BUTTON));
}

}  // namespace media_router
