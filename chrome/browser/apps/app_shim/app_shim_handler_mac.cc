// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_shim/app_shim_handler_mac.h"

#include <map>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/apps/app_shim/apps_page_shim_handler.h"
#include "chrome/browser/apps/app_window_registry_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/common/mac/app_mode_common.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"

namespace apps {

namespace {

void TerminateIfNoAppWindows() {
  bool app_windows_left =
      AppWindowRegistryUtil::IsAppWindowVisibleInAnyProfile(0);
  if (!app_windows_left) {
    chrome::AttemptExit();
  }
}

class AppShimHandlerRegistry : public content::NotificationObserver {
 public:
  static AppShimHandlerRegistry* GetInstance() {
    return base::Singleton<
        AppShimHandlerRegistry,
        base::LeakySingletonTraits<AppShimHandlerRegistry>>::get();
  }

  AppShimHandler* GetForAppMode(const std::string& app_mode_id) const {
    HandlerMap::const_iterator it = handlers_.find(app_mode_id);
    if (it != handlers_.end())
      return it->second;

    return default_handler_;
  }

  bool SetForAppMode(const std::string& app_mode_id, AppShimHandler* handler) {
    bool inserted_or_removed = handler ?
        handlers_.insert(HandlerMap::value_type(app_mode_id, handler)).second :
        handlers_.erase(app_mode_id) == 1;
    DCHECK(inserted_or_removed);
    return inserted_or_removed;
  }

  void SetDefaultHandler(AppShimHandler* handler) {
    DCHECK_NE(default_handler_ == NULL, handler == NULL);
    default_handler_ = handler;
  }

  void MaybeTerminate() {
    if (!browser_session_running_) {
      // Post this to give AppWindows a chance to remove themselves from the
      // registry.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&TerminateIfNoAppWindows));
    }
  }

  bool ShouldRestoreSession() {
    return !browser_session_running_;
  }

 private:
  friend struct base::DefaultSingletonTraits<AppShimHandlerRegistry>;
  typedef std::map<std::string, AppShimHandler*> HandlerMap;

  AppShimHandlerRegistry()
      : default_handler_(NULL),
        browser_session_running_(false) {
    registrar_.Add(
        this, chrome::NOTIFICATION_BROWSER_OPENED,
        content::NotificationService::AllBrowserContextsAndSources());
    registrar_.Add(
        this, chrome::NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST,
        content::NotificationService::AllBrowserContextsAndSources());
    registrar_.Add(
        this, chrome::NOTIFICATION_BROWSER_CLOSE_CANCELLED,
        content::NotificationService::AllBrowserContextsAndSources());
    SetForAppMode(app_mode::kAppListModeId, &apps_page_shim_handler_);
  }

  ~AppShimHandlerRegistry() override {}

  // content::NotificationObserver override:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    switch (type) {
      case chrome::NOTIFICATION_BROWSER_OPENED:
      case chrome::NOTIFICATION_BROWSER_CLOSE_CANCELLED:
        browser_session_running_ = true;
        break;
      case chrome::NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST:
        browser_session_running_ = false;
        break;
      default:
        NOTREACHED();
    }
  }

  HandlerMap handlers_;
  AppShimHandler* default_handler_;
  AppsPageShimHandler apps_page_shim_handler_;
  content::NotificationRegistrar registrar_;
  bool browser_session_running_;

  DISALLOW_COPY_AND_ASSIGN(AppShimHandlerRegistry);
};

}  // namespace

// static
void AppShimHandler::RegisterHandler(const std::string& app_mode_id,
                                     AppShimHandler* handler) {
  DCHECK(handler);
  AppShimHandlerRegistry::GetInstance()->SetForAppMode(app_mode_id, handler);
}

// static
void AppShimHandler::RemoveHandler(const std::string& app_mode_id) {
  AppShimHandlerRegistry::GetInstance()->SetForAppMode(app_mode_id, NULL);
}

// static
AppShimHandler* AppShimHandler::GetForAppMode(const std::string& app_mode_id) {
  return AppShimHandlerRegistry::GetInstance()->GetForAppMode(app_mode_id);
}

// static
void AppShimHandler::SetDefaultHandler(AppShimHandler* handler) {
  AppShimHandlerRegistry::GetInstance()->SetDefaultHandler(handler);
}

// static
void AppShimHandler::MaybeTerminate() {
  AppShimHandlerRegistry::GetInstance()->MaybeTerminate();
}

// static
bool AppShimHandler::ShouldRestoreSession() {
  return AppShimHandlerRegistry::GetInstance()->ShouldRestoreSession();
}

}  // namespace apps
