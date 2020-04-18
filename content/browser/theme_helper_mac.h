// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_THEME_HELPER_MAC_H_
#define CONTENT_BROWSER_THEME_HELPER_MAC_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "third_party/blink/public/platform/mac/web_scrollbar_theme.h"

namespace content {

class ThemeHelperMac : public NotificationObserver {
 public:
  // Return pointer to the singleton instance for the current process, or NULL
  // if none.
  static ThemeHelperMac* GetInstance();

  // Returns the value of +[NSScroller preferredScrollStyle] as expressed
  // as the blink enum value.
  static blink::ScrollerStyle GetPreferredScrollerStyle();

 private:
  friend struct base::DefaultSingletonTraits<ThemeHelperMac>;

  ThemeHelperMac();
  ~ThemeHelperMac() override;

  // Overridden from NotificationObserver:
  void Observe(int type,
               const NotificationSource& source,
               const NotificationDetails& details) override;

  NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ThemeHelperMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_THEME_HELPER_MAC_H_
