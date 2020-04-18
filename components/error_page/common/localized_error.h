// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ERROR_PAGE_COMMON_LOCALIZED_ERROR_H_
#define COMPONENTS_ERROR_PAGE_COMMON_LOCALIZED_ERROR_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
}

namespace error_page {

struct ErrorPageParams;

class LocalizedError {
 public:
  // Fills |error_strings| with values to be used to build an error page used
  // on HTTP errors, like 404 or connection reset.
  static void GetStrings(int error_code,
                         const std::string& error_domain,
                         const GURL& failed_url,
                         bool is_post,
                         bool stale_copy_in_cache,
                         bool can_show_network_diagnostics_dialog,
                         bool is_incognito,
                         const std::string& locale,
                         std::unique_ptr<error_page::ErrorPageParams> params,
                         base::DictionaryValue* strings);

  // Returns a description of the encountered error.
  static base::string16 GetErrorDetails(const std::string& error_domain,
                                        int error_code,
                                        bool is_post);

  // Returns true if an error page exists for the specified parameters.
  static bool HasStrings(const std::string& error_domain, int error_code);

 private:

  DISALLOW_IMPLICIT_CONSTRUCTORS(LocalizedError);
};

}  // namespace error_page

#endif  // COMPONENTS_ERROR_PAGE_COMMON_LOCALIZED_ERROR_H_
