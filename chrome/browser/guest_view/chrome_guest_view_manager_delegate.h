// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GUEST_VIEW_CHROME_GUEST_VIEW_MANAGER_DELEGATE_H_
#define CHROME_BROWSER_GUEST_VIEW_CHROME_GUEST_VIEW_MANAGER_DELEGATE_H_

#include "base/macros.h"
#include "extensions/browser/guest_view/extensions_guest_view_manager_delegate.h"

namespace extensions {

// Defines a chrome-specific implementation of
// ExtensionsGuestViewManagerDelegate that knows about the concept of a task
// manager and the need for tagging the guest view WebContents by their
// appropriate task manager tag.
class ChromeGuestViewManagerDelegate
    : public ExtensionsGuestViewManagerDelegate {
 public:
  explicit ChromeGuestViewManagerDelegate(content::BrowserContext* context);
  ~ChromeGuestViewManagerDelegate() override;

  // GuestViewManagerDelegate:
  void OnGuestAdded(content::WebContents* guest_web_contents) const override;

 private:
  void RegisterSyntheticFieldTrial(
      content::WebContents* guest_web_contents) const;

  DISALLOW_COPY_AND_ASSIGN(ChromeGuestViewManagerDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_GUEST_VIEW_CHROME_GUEST_VIEW_MANAGER_DELEGATE_H_
