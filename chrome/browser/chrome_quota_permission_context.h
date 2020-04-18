// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_QUOTA_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_CHROME_QUOTA_PERMISSION_CONTEXT_H_

#include "base/compiler_specific.h"
#include "content/public/browser/quota_permission_context.h"

class ChromeQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  ChromeQuotaPermissionContext();

  // The callback will be dispatched on the IO thread.
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              const PermissionCallback& callback) override;

  void DispatchCallbackOnIOThread(
      const PermissionCallback& callback,
      QuotaPermissionResponse response);

 private:
  ~ChromeQuotaPermissionContext() override;
};

#endif  // CHROME_BROWSER_CHROME_QUOTA_PERMISSION_CONTEXT_H_
