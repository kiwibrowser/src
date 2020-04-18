// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUPERVISED_USER_ERROR_PAGE_SUPERVISED_USER_ERROR_PAGE_ANDROID_H_
#define COMPONENTS_SUPERVISED_USER_ERROR_PAGE_SUPERVISED_USER_ERROR_PAGE_ANDROID_H_

#include "components/web_restrictions/interfaces/web_restrictions.mojom.h"

namespace supervised_user_error_page {

std::string BuildHtmlFromWebRestrictionsResult(
    const web_restrictions::mojom::ClientResultPtr& result,
    const std::string app_locale);

}  // namespace supervised_user_error_page

#endif  // COMPONENTS_SUPERVISED_USER_ERROR_PAGE_SUPERVISED_USER_ERROR_PAGE_ANDROID_H_
