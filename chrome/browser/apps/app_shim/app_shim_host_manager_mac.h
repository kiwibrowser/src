// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_APP_SHIM_APP_SHIM_HOST_MANAGER_MAC_H_
#define CHROME_BROWSER_APPS_APP_SHIM_APP_SHIM_HOST_MANAGER_MAC_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/apps/app_shim/unix_domain_socket_acceptor.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace apps {
class ExtensionAppShimHandler;
}

namespace base {
class FilePath;
}

namespace test {
class AppShimHostManagerTestApi;
}

// The AppShimHostManager receives connections from app shims on a UNIX
// socket (|acceptor_|) and creates a helper object to manage the connection.
class AppShimHostManager : public apps::UnixDomainSocketAcceptor::Delegate,
                           public base::RefCountedThreadSafe<
                               AppShimHostManager,
                               content::BrowserThread::DeleteOnUIThread> {
 public:
  AppShimHostManager();

  // Init passes this AppShimHostManager to PostTask which requires it to have
  // a non-zero refcount. Therefore, Init cannot be called in the constructor
  // since the refcount is zero at that point.
  void Init();

  apps::ExtensionAppShimHandler* extension_app_shim_handler() {
    return extension_app_shim_handler_.get();
  }

 private:
  friend class base::RefCountedThreadSafe<AppShimHostManager>;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<AppShimHostManager>;
  friend class test::AppShimHostManagerTestApi;
  virtual ~AppShimHostManager();

  // UnixDomainSocketAcceptor::Delegate implementation.
  void OnClientConnected(
      mojo::edk::ScopedInternalPlatformHandle handle) override;
  void OnListenError() override;

  // The |acceptor_| must be created on a thread which allows blocking I/O.
  void InitOnBackgroundThread();

  // Called on the IO thread to begin listening for connections from app shims.
  void ListenOnIOThread();

  base::FilePath directory_in_tmp_;

  std::unique_ptr<apps::UnixDomainSocketAcceptor> acceptor_;

  std::unique_ptr<apps::ExtensionAppShimHandler> extension_app_shim_handler_;

  DISALLOW_COPY_AND_ASSIGN(AppShimHostManager);
};

#endif  // CHROME_BROWSER_APPS_APP_SHIM_APP_SHIM_HOST_MANAGER_MAC_H_
