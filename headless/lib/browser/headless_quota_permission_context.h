// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_QUOTA_PERMISSION_CONTEXT_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_QUOTA_PERMISSION_CONTEXT_H_

#include "base/macros.h"
#include "content/public/browser/quota_permission_context.h"

namespace headless {

class HeadlessQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  HeadlessQuotaPermissionContext();

  // The callback will be dispatched on the IO thread.
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              const PermissionCallback& callback) override;

 private:
  ~HeadlessQuotaPermissionContext() override;

  DISALLOW_COPY_AND_ASSIGN(HeadlessQuotaPermissionContext);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_QUOTA_PERMISSION_CONTEXT_H_
