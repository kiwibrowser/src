// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_APP_MENU_ICON_CONTROLLER_H_
#define CHROME_BROWSER_UI_TOOLBAR_APP_MENU_ICON_CONTROLLER_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/upgrade_observer.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"

#if defined(OS_WIN)
#include "chrome/browser/win/enumerate_modules_model.h"
#endif

class Profile;

// AppMenuIconController encapsulates the logic for badging the app menu icon
// as a result of various events - such as available updates, errors, etc.
class AppMenuIconController :
#if defined(OS_WIN)
    public EnumerateModulesModel::Observer,
#endif
    public content::NotificationObserver,
    public UpgradeObserver {
 public:
  enum class IconType {
    NONE,
    UPGRADE_NOTIFICATION,
    GLOBAL_ERROR,
    INCOMPATIBILITY_WARNING,
  };
  enum class Severity {
    NONE,
    LOW,
    MEDIUM,
    HIGH,
  };

  // Delegate interface for receiving icon update notifications.
  class Delegate {
   public:
    // Notifies the UI to update the icon to have the specified |severity|, as
    // well as specifying whether it should |animate|. The |type| parameter
    // specifies the type of change (i.e. the source of the notification).
    virtual void UpdateSeverity(IconType type,
                                Severity severity,
                                bool animate) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // Creates an instance of this class for the given |profile| that will notify
  // |delegate| of updates.
  AppMenuIconController(Profile* profile, Delegate* delegate);
  ~AppMenuIconController() override;

  // Forces an update of the UI based on the current state of the world. This
  // will check whether there are any current pending updates, global errors,
  // etc. and based on that information trigger an appropriate call to the
  // delegate.
  void UpdateDelegate();

 private:
#if defined(OS_WIN)
  // EnumerateModulesModel::Observer:
  void OnScanCompleted() override;
  void OnConflictsAcknowledged() override;
#endif

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // UpgradeObserver:
  void OnUpgradeRecommended() override;

  Profile* profile_;
  Delegate* delegate_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(AppMenuIconController);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_APP_MENU_ICON_CONTROLLER_H_
