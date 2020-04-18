// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_APP_APPLICATION_CONTEXT_H_
#define IOS_WEB_VIEW_INTERNAL_APP_APPLICATION_CONTEXT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/sequence_checker.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace net {
class URLRequestContextGetter;
}

namespace net_log {
class ChromeNetLog;
}

class PrefService;

namespace ios_web_view {

class WebViewIOThread;

// Exposes application global state objects.
class ApplicationContext {
 public:
  static ApplicationContext* GetInstance();

  // Gets the preferences associated with this application.
  PrefService* GetLocalState();

  // Gets the URL request context associated with this application.
  net::URLRequestContextGetter* GetSystemURLRequestContext();

  // Gets the locale used by the application.
  const std::string& GetApplicationLocale();

  // Creates state tied to application threads. It is expected this will be
  // called from web::WebMainParts::PreCreateThreads.
  void PreCreateThreads();

  // Saves application context state if |local_state_| exists. This should be
  // called during shutdown to save application state.
  void SaveState();

  // Destroys state tied to application threads. It is expected this will be
  // called from web::WebMainParts::PostDestroyThreads.
  void PostDestroyThreads();

 private:
  friend struct base::DefaultSingletonTraits<ApplicationContext>;

  ApplicationContext();
  ~ApplicationContext();

  // Gets the ChromeNetLog.
  net_log::ChromeNetLog* GetNetLog();

  // Gets the WebViewIOThread.
  WebViewIOThread* GetWebViewIOThread();

  // Sets the locale used by the application.
  void SetApplicationLocale(const std::string& locale);

  SEQUENCE_CHECKER(sequence_checker_);
  std::unique_ptr<PrefService> local_state_;
  std::unique_ptr<net_log::ChromeNetLog> net_log_;
  std::unique_ptr<WebViewIOThread> web_view_io_thread_;
  std::string application_locale_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationContext);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_APP_APPLICATION_CONTEXT_H_
