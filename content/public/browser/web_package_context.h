// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_PACKAGE_CONTEXT_H_
#define CONTENT_PUBLIC_BROWSER_WEB_PACKAGE_CONTEXT_H_

#include "base/optional.h"
#include "base/time/time.h"

namespace content {

// This class will represent the per-StoragePartition WebPackage related data.
// Currently this class has only one method for testing.
class WebPackageContext {
 public:
  // Changes the time which will be used to verify SignedHTTPExchange. This
  // method is for testing. Must be called on IO thread.
  // Need call this method again in the end of the test with nullopt time to
  // reset the the verification time overriding.
  virtual void SetSignedExchangeVerificationTimeForTesting(
      base::Optional<base::Time> time) = 0;

 protected:
  WebPackageContext() {}
  virtual ~WebPackageContext() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_PACKAGE_CONTEXT_H_
