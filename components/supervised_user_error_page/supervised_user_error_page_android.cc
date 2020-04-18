// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/supervised_user_error_page/supervised_user_error_page_android.h"

#include "base/macros.h"
#include "components/grit/components_resources.h"
#include "components/supervised_user_error_page/supervised_user_error_page.h"
#include "ui/base/resource/resource_bundle.h"

namespace supervised_user_error_page {

std::string BuildHtmlFromWebRestrictionsResult(
    const web_restrictions::mojom::ClientResultPtr& result,
    const std::string app_locale) {
  // Check if Webview has the resources it needs to build the error page.
  // It only will have it is built as Monochrome.
  if (ui::ResourceBundle::GetSharedInstance()
          .GetRawDataResource(IDR_SUPERVISED_USER_BLOCK_INTERSTITIAL_HTML)
          .empty()) {
    return std::string();
  }
  return BuildHtml(
      result->intParams["Allow access requests"],
      result->stringParams["Profile image URL"],
      result->stringParams["Second profile image URL"],
      result->stringParams["Custodian"],
      result->stringParams["Custodian email"],
      result->stringParams["Second custodian"],
      result->stringParams["Second custodian email"],
      result->intParams["Is child account"],
      /* is_deprecated = */ false,
      static_cast<FilteringBehaviorReason>(result->intParams["Reason"]),
      app_locale);
}

}  //  namespace supervised_user_error_page
