// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UKM_DEBUG_PAGE_REQUEST_JOB_H_
#define COMPONENTS_UKM_DEBUG_PAGE_REQUEST_JOB_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/browser/url_data_source.h"

namespace ukm {

class UkmService;

namespace debug {

// Implements the chrome://ukm page for debugging UKM state.
class DebugPage : public content::URLDataSource {
 public:
  typedef base::RepeatingCallback<UkmService*()> ServiceGetter;

  explicit DebugPage(ServiceGetter service_getter);

  // content::URLDataSource:
  std::string GetSource() const override;
  std::string GetMimeType(const std::string& path) const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  bool AllowCaching() const override;

 private:
  ~DebugPage() override;

  ServiceGetter service_getter_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DebugPage);
};

}  // namespace debug
}  // namespace ukm

#endif  // COMPONENTS_UKM_DEBUG_PAGE_REQUEST_JOB_H_
