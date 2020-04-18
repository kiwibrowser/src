// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/common/subresource_filter_utils.h"

#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/url_utils.h"
#include "url/gurl.h"

namespace subresource_filter {

bool ShouldUseParentActivation(const GURL& url) {
  if (content::IsBrowserSideNavigationEnabled())
    return !content::IsURLHandledByNetworkStack(url);

  // TODO(csharrison): It is not always true that a data URL can use its
  // parent's activation in OOPIF mode, where the resulting data frame will
  // be same-process to its initiator. See crbug.com/739777 for more
  // information.
  return (url.SchemeIs(url::kDataScheme) || url == url::kAboutBlankURL ||
          url == content::kAboutSrcDocURL);
}

}  // namespace subresource_filter
