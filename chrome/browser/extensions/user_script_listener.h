// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_USER_SCRIPT_LISTENER_H_
#define CHROME_BROWSER_EXTENSIONS_USER_SCRIPT_LISTENER_H_

#include <list>
#include <map>

#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/common/resource_type.h"
#include "extensions/browser/extension_registry_observer.h"

class GURL;
class URLPattern;

namespace content {
class ResourceThrottle;
}

namespace extensions {
class Extension;

// This class handles delaying of resource loads that depend on unloaded user
// scripts. For each request that comes in, we check if it depends on a user
// script, and if so, whether that user script is ready; if not, we delay the
// request.
//
// This class lives mostly on the IO thread. It listens on the UI thread for
// updates to loaded extensions.
class UserScriptListener : public base::RefCountedThreadSafe<
                               UserScriptListener,
                               content::BrowserThread::DeleteOnUIThread>,
                           public content::NotificationObserver,
                           public ExtensionRegistryObserver {
 public:
  UserScriptListener();

  // Constructs a ResourceThrottle if the UserScriptListener needs to delay the
  // given URL.  Otherwise, this method returns NULL.
  content::ResourceThrottle* CreateResourceThrottle(
      const GURL& url,
      content::ResourceType resource_type);

 private:
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<UserScriptListener>;

  typedef std::list<URLPattern> URLPatterns;

  ~UserScriptListener() override;

  bool ShouldDelayRequest(const GURL& url,
                          content::ResourceType resource_type);
  void StartDelayedRequests();

  // Update user_scripts_ready_ based on the status of all profiles. On a
  // transition from false to true, we resume all delayed requests.
  void CheckIfAllUserScriptsReady();

  // Resume any requests that we delayed in order to wait for user scripts.
  void UserScriptsReady(void* profile_id);

  // Clean up per-profile information related to the given profile.
  void ProfileDestroyed(void* profile_id);

  // Appends new url patterns to our list, also setting user_scripts_ready_
  // to false.
  void AppendNewURLPatterns(void* profile_id, const URLPatterns& new_patterns);

  // Replaces our url pattern list. This is only used when patterns have been
  // deleted, so user_scripts_ready_ remains unchanged.
  void ReplaceURLPatterns(void* profile_id, const URLPatterns& patterns);

  // True if all user scripts from all profiles are ready.
  bool user_scripts_ready_;

  // Stores a throttle per URL request that we have delayed.
  class Throttle;
  using WeakThrottle = base::WeakPtr<Throttle>;
  using WeakThrottleList = base::circular_deque<WeakThrottle>;
  WeakThrottleList throttles_;

  // Per-profile bookkeeping so we know when all user scripts are ready.
  struct ProfileData;
  typedef std::map<void*, ProfileData> ProfileDataMap;
  ProfileDataMap profile_data_;

  // --- UI thread:

  // Helper to collect the extension's user script URL patterns in a list and
  // return it.
  void CollectURLPatterns(const Extension* extension,
                          URLPatterns* patterns);

  // content::NotificationObserver
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;
  void OnShutdown(ExtensionRegistry* registry) override;

  ScopedObserver<extensions::ExtensionRegistry,
                 extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(UserScriptListener);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_USER_SCRIPT_LISTENER_H_
