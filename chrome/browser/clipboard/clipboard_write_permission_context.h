// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CLIPBOARD_CLIPBOARD_WRITE_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_CLIPBOARD_CLIPBOARD_WRITE_PERMISSION_CONTEXT_H_

#include "base/macros.h"
#include "chrome/browser/permissions/permission_context_base.h"

class ClipboardWritePermissionContext : public PermissionContextBase {
 public:
  explicit ClipboardWritePermissionContext(Profile* profile);
  ~ClipboardWritePermissionContext() override;

 private:
  // PermissionContextBase:
  ContentSetting GetPermissionStatusInternal(
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      const GURL& embedding_origin) const override;
  bool IsRestrictedToSecureOrigins() const override;

  DISALLOW_COPY_AND_ASSIGN(ClipboardWritePermissionContext);
};

#endif  // CHROME_BROWSER_CLIPBOARD_CLIPBOARD_WRITE_PERMISSION_CONTEXT_H_
