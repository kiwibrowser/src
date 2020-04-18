// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BANNERS_APP_BANNER_INFOBAR_DELEGATE_DESKTOP_H_
#define CHROME_BROWSER_BANNERS_APP_BANNER_INFOBAR_DELEGATE_DESKTOP_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "third_party/blink/public/common/manifest/manifest.h"

namespace content {
class WebContents;
}

namespace extensions {
class BookmarkAppHelper;
}

namespace infobars {
class InfoBar;
}

namespace banners {

class AppBannerManager;

class AppBannerInfoBarDelegateDesktop : public ConfirmInfoBarDelegate {

 public:
  static infobars::InfoBar* Create(
      content::WebContents* web_contents,
      base::WeakPtr<AppBannerManager> weak_manager,
      extensions::BookmarkAppHelper* bookmark_app_helper,
      const blink::Manifest& manifest);

 private:
  AppBannerInfoBarDelegateDesktop(
      base::WeakPtr<AppBannerManager> weak_manager,
      extensions::BookmarkAppHelper* bookmark_app_helper,
      const blink::Manifest& manifest);
  ~AppBannerInfoBarDelegateDesktop() override;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  void InfoBarDismissed() override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;

  base::WeakPtr<AppBannerManager> weak_manager_;
  extensions::BookmarkAppHelper* bookmark_app_helper_;
  blink::Manifest manifest_;
  bool has_user_interaction_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerInfoBarDelegateDesktop);
};

}  // namespace banners

#endif  // CHROME_BROWSER_BANNERS_APP_BANNER_INFOBAR_DELEGATE_DESKTOP_H_
