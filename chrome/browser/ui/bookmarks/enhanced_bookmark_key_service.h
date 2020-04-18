// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BOOKMARKS_ENHANCED_BOOKMARK_KEY_SERVICE_H_
#define CHROME_BROWSER_UI_BOOKMARKS_ENHANCED_BOOKMARK_KEY_SERVICE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
class Extension;
}  // namespace extensions

// Maintains assignment of bookmark keybinding on the enhanced bookmarks
// extension when not assigned to other extensions.
class EnhancedBookmarkKeyService : public content::NotificationObserver,
                                   public KeyedService {
 public:
  EnhancedBookmarkKeyService(content::BrowserContext* context);
  ~EnhancedBookmarkKeyService() override;

 private:
  // Overridden from content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  const extensions::Extension* GetEnhancedBookmarkExtension() const;

  // The content notification registrar for listening to extension key events.
  content::NotificationRegistrar registrar_;

  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(EnhancedBookmarkKeyService);
};

#endif  // CHROME_BROWSER_UI_BOOKMARKS_ENHANCED_BOOKMARK_KEY_SERVICE_H_
