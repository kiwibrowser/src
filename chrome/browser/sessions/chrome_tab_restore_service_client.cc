// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/chrome_tab_restore_service_client.h"

#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/common/url_constants.h"
#include "components/sessions/content/content_live_tab.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#endif

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser_live_tab_context.h"
#else
#include "chrome/browser/ui/android/tab_model/android_live_tab_context.h"
#endif

namespace {

void RecordAppLaunch(Profile* profile, const GURL& url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile)
          ->enabled_extensions()
          .GetAppByURL(url);
  if (!extension)
    return;

  extensions::RecordAppLaunchType(
      extension_misc::APP_LAUNCH_NTP_RECENTLY_CLOSED, extension->GetType());
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}

}  // namespace

ChromeTabRestoreServiceClient::ChromeTabRestoreServiceClient(Profile* profile)
    : profile_(profile) {}

ChromeTabRestoreServiceClient::~ChromeTabRestoreServiceClient() {}

sessions::LiveTabContext* ChromeTabRestoreServiceClient::CreateLiveTabContext(
    const std::string& app_name,
    const gfx::Rect& bounds,
    ui::WindowShowState show_state,
    const std::string& workspace) {
#if defined(OS_ANDROID)
  // Android does not support creating a LiveTabContext here.
  NOTREACHED();
  return nullptr;
#else
  return BrowserLiveTabContext::Create(profile_, app_name, bounds, show_state,
                                       workspace);
#endif
}

sessions::LiveTabContext*
ChromeTabRestoreServiceClient::FindLiveTabContextForTab(
    const sessions::LiveTab* tab) {
#if defined(OS_ANDROID)
  return AndroidLiveTabContext::FindContextForWebContents(
      static_cast<const sessions::ContentLiveTab*>(tab)->web_contents());
#else
  return BrowserLiveTabContext::FindContextForWebContents(
      static_cast<const sessions::ContentLiveTab*>(tab)->web_contents());
#endif
}

sessions::LiveTabContext*
ChromeTabRestoreServiceClient::FindLiveTabContextWithID(SessionID desired_id) {
#if defined(OS_ANDROID)
  return AndroidLiveTabContext::FindContextWithID(desired_id);
#else
  return BrowserLiveTabContext::FindContextWithID(desired_id);
#endif
}

bool ChromeTabRestoreServiceClient::ShouldTrackURLForRestore(const GURL& url) {
  return ::ShouldTrackURLForRestore(url);
}

std::string ChromeTabRestoreServiceClient::GetExtensionAppIDForTab(
    sessions::LiveTab* tab) {
  std::string extension_app_id;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::TabHelper* extensions_tab_helper =
      extensions::TabHelper::FromWebContents(
          static_cast<sessions::ContentLiveTab*>(tab)->web_contents());
  // extensions_tab_helper is NULL in some browser tests.
  if (extensions_tab_helper) {
    const extensions::Extension* extension =
        extensions_tab_helper->extension_app();
    if (extension)
      extension_app_id = extension->id();
  }
#endif

  return extension_app_id;
}

base::FilePath ChromeTabRestoreServiceClient::GetPathToSaveTo() {
  return profile_->GetPath();
}

GURL ChromeTabRestoreServiceClient::GetNewTabURL() {
  return GURL(chrome::kChromeUINewTabURL);
}

bool ChromeTabRestoreServiceClient::HasLastSession() {
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile_);
  Profile::ExitType exit_type = profile_->GetLastSessionExitType();
  // The previous session crashed and wasn't restored, or was a forced
  // shutdown. Both of which won't have notified us of the browser close so
  // that we need to load the windows from session service (which will have
  // saved them).
  return (!profile_->restored_last_session() && session_service &&
          (exit_type == Profile::EXIT_CRASHED ||
           exit_type == Profile::EXIT_SESSION_ENDED));
#else
  return false;
#endif
}

void ChromeTabRestoreServiceClient::GetLastSession(
    const sessions::GetLastSessionCallback& callback,
    base::CancelableTaskTracker* tracker) {
  DCHECK(HasLastSession());
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  SessionServiceFactory::GetForProfile(profile_)
      ->GetLastSession(callback, tracker);
#endif
}

void ChromeTabRestoreServiceClient::OnTabRestored(const GURL& url) {
  RecordAppLaunch(profile_, url);
}
