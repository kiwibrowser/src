// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/ui_test_input.h"
#include "chrome/browser/vr/elements/ui_element_name.h"

#include "base/logging.h"
#include "base/macros.h"

namespace vr {

UiElementName UserFriendlyElementNameToUiElementName(
    UserFriendlyElementName name) {
  switch (name) {
    case UserFriendlyElementName::kUrl:
      return kUrlBarOriginRegion;
    case UserFriendlyElementName::kBackButton:
      return kUrlBarBackButton;
    case UserFriendlyElementName::kForwardButton:
      return kOverflowMenuForwardButton;
    case UserFriendlyElementName::kReloadButton:
      return kOverflowMenuReloadButton;
    case UserFriendlyElementName::kOverflowMenu:
      return kUrlBarOverflowButton;
    case UserFriendlyElementName::kPageInfoButton:
      return kUrlBarSecurityButton;
    case UserFriendlyElementName::kBrowsingDialog:
      return k2dBrowsingHostedUiContent;
    default:
      NOTREACHED();
      return kNone;
  }
}

}  // namespace vr
