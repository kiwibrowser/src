// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INFOBARS_MOCK_INFOBAR_SERVICE_H_
#define CHROME_BROWSER_INFOBARS_MOCK_INFOBAR_SERVICE_H_

#include "chrome/browser/infobars/infobar_service.h"

// A mock infobar service that creates do-nothing infobar objects.  This should
// be used in tests that need to verify that infobars are created, but are not
// trying to verify the visual appearance/platform-specific behavior of
// infobars.
class MockInfoBarService : public InfoBarService {
 public:
  // Creates a MockInfoBarService and attaches it as the InfoBarService for
  // |web_contents|.
  static void CreateForWebContents(content::WebContents* web_contents);

  std::unique_ptr<infobars::InfoBar> CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate> delegate) override;

 private:
  explicit MockInfoBarService(content::WebContents* web_contents);
};

#endif  // CHROME_BROWSER_INFOBARS_MOCK_INFOBAR_SERVICE_H_
