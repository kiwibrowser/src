// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_use_measurement/page_load_capping/page_load_capping_infobar_delegate.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// The infobar that allows the user to resume resource loading on the page.
class ResumeDelegate : public PageLoadCappingInfoBarDelegate {
 public:
  ResumeDelegate() = default;
  ~ResumeDelegate() override = default;

 private:
  // PageLoadCappingInfoBarDelegate:
  base::string16 GetMessageText() const override {
    return l10n_util::GetStringUTF16(IDS_PAGE_CAPPING_STOPPED_TITLE);
  }
  base::string16 GetButtonLabel(InfoBarButton button) const override {
    DCHECK_EQ(ConfirmInfoBarDelegate::BUTTON_OK, button);
    return l10n_util::GetStringUTF16(IDS_PAGE_CAPPING_CONTINUE_MESSAGE);
  }
  bool Accept() override {
    // TODO(ryansturm): Add functionality to resume page loads.
    // https://crbug.com/797979
    return true;
  }

  DISALLOW_COPY_AND_ASSIGN(ResumeDelegate);
};

// The infobar that allows the user to pause resoruce loading on the page.
class PauseDelegate : public PageLoadCappingInfoBarDelegate {
 public:
  explicit PauseDelegate(int64_t bytes_threshold)
      : bytes_threshold_(bytes_threshold) {}
  ~PauseDelegate() override = default;

 private:
  // PageLoadCappingInfoBarDelegate:
  base::string16 GetMessageText() const override {
    return l10n_util::GetStringFUTF16Int(
        IDS_PAGE_CAPPING_TITLE,
        static_cast<int>(bytes_threshold_ / 1024 / 1024));
  }

  base::string16 GetButtonLabel(InfoBarButton button) const override {
    DCHECK_EQ(ConfirmInfoBarDelegate::BUTTON_OK, button);
    return l10n_util::GetStringUTF16(IDS_PAGE_CAPPING_STOP_MESSAGE);
  }

  bool Accept() override {
    // TODO(ryansturm): Add functionality to pause page loads.
    // https://crbug.com/797979

    auto* infobar_manager = infobar()->owner();
    // |this| will be gone after this call.
    infobar_manager->ReplaceInfoBar(infobar(),
                                    infobar_manager->CreateConfirmInfoBar(
                                        std::make_unique<ResumeDelegate>()));

    return false;
  }

 private:
  // The amount of bytes that was exceeded to trigger this infobar.
  int64_t bytes_threshold_;

  DISALLOW_COPY_AND_ASSIGN(PauseDelegate);
};

}  // namespace

// static
bool PageLoadCappingInfoBarDelegate::Create(
    int64_t bytes_threshold,
    content::WebContents* web_contents) {
  auto* infobar_service = InfoBarService::FromWebContents(web_contents);

  // WrapUnique is used to allow for a private constructor.
  return infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::make_unique<PauseDelegate>(bytes_threshold)));
}

PageLoadCappingInfoBarDelegate::~PageLoadCappingInfoBarDelegate() = default;

PageLoadCappingInfoBarDelegate::PageLoadCappingInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
PageLoadCappingInfoBarDelegate::GetIdentifier() const {
  return PAGE_LOAD_CAPPING_INFOBAR_DELEGATE;
}

int PageLoadCappingInfoBarDelegate::GetIconId() const {
// TODO(ryansturm): Make data saver resources available on other platforms.
// https://crbug.com/820594
#if defined(OS_ANDROID)
  return IDR_ANDROID_INFOBAR_PREVIEWS;
#else
  return kNoIconID;
#endif
}

bool PageLoadCappingInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return true;
}

int PageLoadCappingInfoBarDelegate::GetButtons() const {
  return ConfirmInfoBarDelegate::BUTTON_OK;
}
