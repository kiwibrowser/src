// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/infobars/mock_infobar_service.h"

#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"

// static
void MockInfoBarService::CreateForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  const void* user_data_key = UserDataKey();
  DCHECK(!web_contents->GetUserData(user_data_key));
  web_contents->SetUserData(
      user_data_key, base::WrapUnique(new MockInfoBarService(web_contents)));
}

std::unique_ptr<infobars::InfoBar> MockInfoBarService::CreateConfirmInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate) {
  return std::make_unique<infobars::InfoBar>(std::move(delegate));
}

MockInfoBarService::MockInfoBarService(content::WebContents* web_contents)
    : InfoBarService(web_contents) {}
