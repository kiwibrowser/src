// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/infobars/infobar_utils.h"

#include <memory>
#include <utility>

#include "components/infobars/core/confirm_infobar_delegate.h"
#include "ios/chrome/browser/infobars/confirm_infobar_controller.h"
#include "ios/chrome/browser/infobars/infobar.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

std::unique_ptr<infobars::InfoBar> CreateConfirmInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate) {
  ConfirmInfoBarController* controller =
      [[ConfirmInfoBarController alloc] initWithInfoBarDelegate:delegate.get()];
  return std::make_unique<InfoBarIOS>(controller, std::move(delegate));
}
