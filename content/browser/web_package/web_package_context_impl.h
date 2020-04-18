// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_WEB_PACKAGE_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_WEB_PACKAGE_WEB_PACKAGE_CONTEXT_IMPL_H_

#include "content/public/browser/web_package_context.h"

#include "content/common/content_export.h"

namespace content {

class WebPackageContextImpl : public WebPackageContext {
 public:
  WebPackageContextImpl();
  ~WebPackageContextImpl() override;
  void SetSignedExchangeVerificationTimeForTesting(
      base::Optional<base::Time> time) override;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_WEB_PACKAGE_CONTEXT_IMPL_H_
