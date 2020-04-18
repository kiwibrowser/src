// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_AMP_REDIRECTION_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_AMP_REDIRECTION_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "url/gurl.h"

namespace previews {

// Contains the parameters for converting URLs for one domain.
struct AMPConverterEntry;

// Checks whether an URL is whitelisted for AMP redirection previews and
// implements the scheme used to convert URL to its AMP version.
class PreviewsAMPConverter {
 public:
  PreviewsAMPConverter();

  virtual ~PreviewsAMPConverter();

  // Returns true if |url| can be converted to its AMP version. |new_amp_url|
  // will contain the AMP URL.
  bool GetAMPURL(const GURL& url, GURL* new_amp_url) const;

 private:
  // Maps the hostname to its AMP conversion scheme.
  std::map<std::string, std::unique_ptr<AMPConverterEntry>> amp_converter_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(PreviewsAMPConverter);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_AMP_REDIRECTION_H_
