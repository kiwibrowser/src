// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_UTILS_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_UTILS_H_

#include <memory>

class ConfirmInfoBarDelegate;

namespace infobars {
class InfoBar;
}

// Returns a confirm infobar that owns |delegate|.
// Visible for testing.
std::unique_ptr<infobars::InfoBar> CreateConfirmInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate);

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_UTILS_H_
