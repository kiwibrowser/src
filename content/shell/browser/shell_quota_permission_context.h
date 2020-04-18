// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_SHELL_QUOTA_PERMISSION_CONTEXT_H_
#define CONTENT_SHELL_BROWSER_SHELL_QUOTA_PERMISSION_CONTEXT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/quota_permission_context.h"

namespace content {

class ShellQuotaPermissionContext : public QuotaPermissionContext {
 public:
  ShellQuotaPermissionContext();

  // The callback will be dispatched on the IO thread.
  void RequestQuotaPermission(const StorageQuotaParams& params,
                              int render_process_id,
                              const PermissionCallback& callback) override;

 private:
  ~ShellQuotaPermissionContext() override;

  DISALLOW_COPY_AND_ASSIGN(ShellQuotaPermissionContext);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_SHELL_QUOTA_PERMISSION_CONTEXT_H_
