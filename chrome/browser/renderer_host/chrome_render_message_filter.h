// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_
#define CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "ppapi/buildflags/buildflags.h"

class GURL;
class Profile;

namespace chrome_browser_net {
class Predictor;
}

namespace predictors {
class PreconnectManager;
}

namespace content_settings {
class CookieSettings;
}

namespace network_hints {
struct LookupRequest;
}

// This class filters out incoming Chrome-specific IPC messages for the renderer
// process on the IPC thread.
class ChromeRenderMessageFilter : public content::BrowserMessageFilter {
 public:
  ChromeRenderMessageFilter(int render_process_id, Profile* profile);

  // content::BrowserMessageFilter methods:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<ChromeRenderMessageFilter>;

  ~ChromeRenderMessageFilter() override;

  void OnDnsPrefetch(const network_hints::LookupRequest& request);
  void OnPreconnect(const GURL& url, bool allow_credentials, int count);

  void OnAllowDatabase(int render_frame_id,
                       const GURL& origin_url,
                       const GURL& top_origin_url,
                       const base::string16& name,
                       const base::string16& display_name,
                       bool* allowed);
  void OnAllowDOMStorage(int render_frame_id,
                         const GURL& origin_url,
                         const GURL& top_origin_url,
                         bool local,
                         bool* allowed);
  void OnRequestFileSystemAccessSync(int render_frame_id,
                                     const GURL& origin_url,
                                     const GURL& top_origin_url,
                                     IPC::Message* message);
  void OnRequestFileSystemAccessAsync(int render_frame_id,
                                      int request_id,
                                      const GURL& origin_url,
                                      const GURL& top_origin_url);
  void OnRequestFileSystemAccessSyncResponse(IPC::Message* reply_msg,
                                             bool allowed);
  void OnRequestFileSystemAccessAsyncResponse(int render_frame_id,
                                              int request_id,
                                              bool allowed);
  void OnRequestFileSystemAccess(int render_frame_id,
                                 const GURL& origin_url,
                                 const GURL& top_origin_url,
                                 base::Callback<void(bool)> callback);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  static void FileSystemAccessedOnUIThread(int render_process_id,
                                           int render_frame_id,
                                           const GURL& url,
                                           bool allowed,
                                           base::Callback<void(bool)> callback);
  static void FileSystemAccessedResponse(int render_process_id,
                                         int render_frame_id,
                                         const GURL& url,
                                         base::Callback<void(bool)> callback,
                                         bool allowed);
#endif
  void OnAllowIndexedDB(int render_frame_id,
                        const GURL& origin_url,
                        const GURL& top_origin_url,
                        const base::string16& name,
                        bool* allowed);
#if BUILDFLAG(ENABLE_PLUGINS)
  void OnIsCrashReportingEnabled(bool* enabled);
#endif

  const int render_process_id_;

  // The Profile associated with our renderer process.  This should only be
  // accessed on the UI thread!
  Profile* profile_;
  // The Predictor for the associated Profile. It is stored so that it can be
  // used on the IO thread.
  chrome_browser_net::Predictor* predictor_;
  // The PreconnectManager for the associated Profile. It is stored so that it
  // can be used on the IO thread.
  predictors::PreconnectManager* preconnect_manager_;

  // Used to look up permissions at database creation time.
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;

  DISALLOW_COPY_AND_ASSIGN(ChromeRenderMessageFilter);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_
