// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_NAVIGATION_HANDLE_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_NAVIGATION_HANDLE_H_

#include <memory>
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace content {

class AppCacheNavigationHandleCore;
class ChromeAppCacheService;

// This class is used to manage the lifetime of AppCacheHosts created during
// navigation. This is a UI thread class, with a pendant class on the IO
// thread, the AppCacheNavigationHandleCore.
//
// The lifetime of the AppCacheNavigationHandle, the
// AppCacheNavigationHandleCore and the AppCacheHost are the following :
//   1) We create a AppCacheNavigationHandle on the UI thread with a
//      app cache host id of -1. This also leads to the creation of a
//      AppCacheNavigationHandleCore with an id of -1. Every time
//      an AppCacheNavigationHandle instance is created the global host id is
//      decremented by 1.
//
//   2) When the navigation request is sent to the IO thread, we include a
//      pointer to the AppCacheNavigationHandleCore.
//
//   3. The AppCacheHost instance is created and its ownership is passed to the
//      AppCacheNavigationHandleCore instance. Now the app cache host id is
//      updated.
//
//   4) The AppCacheNavigationHandleCore instance informs the
//      AppCacheNavigationHandle instance on the UI thread that the app cache
//      host id was updated.
//
//   5) When the navigation is ready to commit, the NavigationRequest will
//      update the RequestNavigationParams based on the id from the
//      AppCacheNavigationHandle.
//
//   6. The commit leads to AppCache registrations happening from the renderer.
//      This is via the IPC message AppCacheHostMsg_RegisterHost. The
//      AppCacheDispatcherHost class which handles these IPCs will be informed
//      about these hosts when the navigation commits. It will ignore the
//      host registrations as they have already been registered. The
//      ownership of the AppCacheHost is passed from the
//      AppCacheNavigationHandle core to the AppCacheBackend.

//   7) When the navigation finishes, the AppCacheNavigationHandle is
//      destroyed. The destructor of the AppCacheNavigationHandle posts a
//      task to destroy the AppacheNavigationHandleCore on the IO thread.

class AppCacheNavigationHandle {
 public:
  AppCacheNavigationHandle(ChromeAppCacheService* appcache_service);
  ~AppCacheNavigationHandle();

  int appcache_host_id() const { return appcache_host_id_; }
  AppCacheNavigationHandleCore* core() const { return core_.get(); }

  // Called when a navigation is committed. The |process_id| parameter is
  // is the process id of the renderer.
  void CommitNavigation(int process_id);

 private:
  int appcache_host_id_;
  std::unique_ptr<AppCacheNavigationHandleCore> core_;
  base::WeakPtrFactory<AppCacheNavigationHandle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppCacheNavigationHandle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_NAVIGATION_HANDLE_H_
