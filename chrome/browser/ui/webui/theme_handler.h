// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_THEME_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_THEME_HANDLER_H_

#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;

// A class to keep the ThemeSource up to date when theme changes.
class ThemeHandler : public content::WebUIMessageHandler,
                     public content::NotificationObserver {
 public:
  explicit ThemeHandler();
  ~ThemeHandler() override;

 private:
  // content::WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Re/set the CSS caches.
  void InitializeCSSCaches();

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  Profile* GetProfile() const;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ThemeHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_THEME_HANDLER_H_
