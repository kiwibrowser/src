// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_use_measurement/page_load_capping/page_load_capping_infobar_delegate.h"

#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/infobars/mock_infobar_service.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
// A test URL used for simulated navigations that do not trigger real URL
// requests.
const char kTestURL[] = "http://www.test.com";
static constexpr int64_t test_capping_threshold_bytes = 10 * 1024 * 1024;
}  // namespace

class PageLoadCappingInfoBarDelegateTest
    : public ChromeRenderViewHostTestHarness {
 public:
  PageLoadCappingInfoBarDelegateTest() = default;
  ~PageLoadCappingInfoBarDelegateTest() override = default;

  void SetUpTest() {
    MockInfoBarService::CreateForWebContents(web_contents());
    NavigateAndCommit(GURL(kTestURL));
  }

  size_t InfoBarCount() { return infobar_service()->infobar_count(); }

  void RemoveAllInfoBars() { infobar_service()->RemoveAllInfoBars(false); }

  infobars::InfoBar* infobar_at(size_t index) {
    return infobar_service()->infobar_at(index);
  }

  InfoBarService* infobar_service() {
    return InfoBarService::FromWebContents(web_contents());
  }
};

TEST_F(PageLoadCappingInfoBarDelegateTest, ClickingCreatesNewInfobar) {
  SetUpTest();
  EXPECT_TRUE(PageLoadCappingInfoBarDelegate::Create(
      test_capping_threshold_bytes, web_contents()));
  EXPECT_EQ(1u, InfoBarCount());
  infobars::InfoBar* infobar = infobar_at(0);
  EXPECT_TRUE(infobar);
  ConfirmInfoBarDelegate* delegate = nullptr;
  if (infobar)
    delegate = infobar->delegate()->AsConfirmInfoBarDelegate();
  EXPECT_TRUE(delegate);
  // Make sure this is pause delegate.
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PAGE_CAPPING_STOP_MESSAGE),
            delegate->GetButtonLabel(ConfirmInfoBarDelegate::BUTTON_OK));
  // |delegate| and |infobar| will be deleted by this call.
  EXPECT_FALSE(delegate->Accept());
  EXPECT_EQ(1u, InfoBarCount());

  infobar = infobar_at(0);
  ConfirmInfoBarDelegate* stopped_delegate = nullptr;
  if (infobar)
    stopped_delegate = infobar->delegate()->AsConfirmInfoBarDelegate();

  // Make sure this is the resume delegate.
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PAGE_CAPPING_CONTINUE_MESSAGE),
      stopped_delegate->GetButtonLabel(ConfirmInfoBarDelegate::BUTTON_OK));
  EXPECT_TRUE(stopped_delegate);
  // Make sure that they are different infobar instances.
  EXPECT_NE(delegate, stopped_delegate);

  // If this is true, the infobar will be closed by the infobar manager.
  EXPECT_TRUE(stopped_delegate->Accept());
}
