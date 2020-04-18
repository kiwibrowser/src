// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NET_ERROR_COMMON_ERROR_PAGE_PARAMS_H_
#define COMPONENTS_NET_ERROR_COMMON_ERROR_PAGE_PARAMS_H_

#include <memory>
#include <string>

#include "url/gurl.h"

namespace base {
class ListValue;
}

namespace error_page {

// Optional parameters that affect the display of an error page.
struct ErrorPageParams {
  ErrorPageParams();
  ~ErrorPageParams();

  // Overrides whether reloading is suggested.
  bool suggest_reload;
  int reload_tracking_id;

  // Overrides default suggestions.  Each entry must be a DictionaryValuethat
  // contains a "header" entry.  A "body" entry may optionally be specified.
  // JSTemplate evaluation will be applied when added to the DOM.  If NULL, the
  // default suggestions will be used.
  std::unique_ptr<base::ListValue> override_suggestions;

  // Prefix to prepend to search terms.  Search box is only shown if this is
  // a valid url.  The search terms will be appended to the end of this URL to
  // conduct a search.
  GURL search_url;
  // Default search terms.  Ignored if |search_url| is invalid.
  std::string search_terms;
  int search_tracking_id;
};

}  // namespace error_page

#endif  // COMPONENTS_NET_ERROR_COMMON_ERROR_PAGE_PARAMS_H_
