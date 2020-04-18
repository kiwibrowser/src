// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"

#include <utility>

#import "chrome/browser/ui/cocoa/infobars/infobar_controller.h"

InfoBarCocoa::InfoBarCocoa(std::unique_ptr<infobars::InfoBarDelegate> delegate)
    : infobars::InfoBar(std::move(delegate)), weak_ptr_factory_(this) {}

InfoBarCocoa::~InfoBarCocoa() {
  if (controller())
    [controller() infobarWillClose];
}

infobars::InfoBarManager* InfoBarCocoa::OwnerCocoa() { return owner(); }

base::WeakPtr<InfoBarCocoa> InfoBarCocoa::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}
