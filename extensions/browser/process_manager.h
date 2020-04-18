// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_PROCESS_MANAGER_H_
#define EXTENSIONS_BROWSER_PROCESS_MANAGER_H_

#include <stdint.h>

#include <map>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/devtools_agent_host_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/event_page_tracker.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension_id.h"
#include "extensions/common/view_type.h"

class GURL;

namespace content {
class BrowserContext;
class DevToolsAgentHost;
class RenderFrameHost;
class SiteInstance;
class WebContents;
};

namespace extensions {

class Extension;
class ExtensionHost;
class ExtensionRegistry;
class ProcessManagerObserver;

// Manages dynamic state of running Chromium extensions. There is one instance
// of this class per Profile. OTR Profiles have a separate instance that keeps
// track of split-mode extensions only.
class ProcessManager : public KeyedService,
                       public content::NotificationObserver,
                       public ExtensionRegistryObserver,
                       public EventPageTracker,
                       public content::DevToolsAgentHostObserver {
 public:
  using ExtensionHostSet = std::set<extensions::ExtensionHost*>;

  static ProcessManager* Get(content::BrowserContext* context);
  ~ProcessManager() override;

  // KeyedService support:
  void Shutdown() override;

  void RegisterRenderFrameHost(content::WebContents* web_contents,
                               content::RenderFrameHost* render_frame_host,
                               const Extension* extension);
  void UnregisterRenderFrameHost(content::RenderFrameHost* render_frame_host);

  // Returns the SiteInstance that the given URL belongs to.
  // TODO(aa): This only returns correct results for extensions and packaged
  // apps, not hosted apps.
  virtual scoped_refptr<content::SiteInstance> GetSiteInstanceForURL(
      const GURL& url);

  using FrameSet = std::set<content::RenderFrameHost*>;
  const FrameSet GetAllFrames() const;

  // Returns all RenderFrameHosts that are registered for the specified
  // extension.
  ProcessManager::FrameSet GetRenderFrameHostsForExtension(
      const std::string& extension_id);

  bool IsRenderFrameHostRegistered(content::RenderFrameHost* render_frame_host);

  void AddObserver(ProcessManagerObserver* observer);
  void RemoveObserver(ProcessManagerObserver* observer);

  // Creates a new UI-less extension instance.  Like CreateViewHost, but not
  // displayed anywhere.  Returns false if no background host can be created,
  // for example for hosted apps and extensions that aren't enabled in
  // Incognito.
  virtual bool CreateBackgroundHost(const Extension* extension,
                                    const GURL& url);

  // Creates background hosts if the embedder is ready and they are not already
  // loaded.
  void MaybeCreateStartupBackgroundHosts();

  // Gets the ExtensionHost for the background page for an extension, or null if
  // the extension isn't running or doesn't have a background page.
  ExtensionHost* GetBackgroundHostForExtension(const std::string& extension_id);

  // Returns the ExtensionHost for the given |render_frame_host|, if there is
  // one.
  ExtensionHost* GetExtensionHostForRenderFrameHost(
      content::RenderFrameHost* render_frame_host);

  // Returns true if the (lazy) background host for the given extension has
  // already been sent the unload event and is shutting down.
  bool IsBackgroundHostClosing(const std::string& extension_id);

  // Returns the extension associated with the specified RenderFrameHost,
  // or null.
  const Extension* GetExtensionForRenderFrameHost(
      content::RenderFrameHost* render_frame_host);

  // Returns the extension associated with the main frame of the given
  // |web_contents|, or null if there isn't one.
  const Extension* GetExtensionForWebContents(
      const content::WebContents* web_contents);

  // Getter and setter for the lazy background page's keepalive count. This is
  // the count of how many outstanding "things" are keeping the page alive.
  // When this reaches 0, we will begin the process of shutting down the page.
  // "Things" include pending events, resource loads, and API calls.
  // Returns -1 if |extension| does not have a lazy background page.
  int GetLazyKeepaliveCount(const Extension* extension);
  void IncrementLazyKeepaliveCount(const Extension* extension);
  void DecrementLazyKeepaliveCount(const Extension* extension);

  // Handles a response to the ShouldSuspend message, used for lazy background
  // pages.
  void OnShouldSuspendAck(const std::string& extension_id,
                          uint64_t sequence_id);

  // Same as above, for the Suspend message.
  void OnSuspendAck(const std::string& extension_id);

  // Tracks network requests for a given RenderFrameHost, used to know
  // when network activity is idle for lazy background pages.
  void OnNetworkRequestStarted(content::RenderFrameHost* render_frame_host,
                               uint64_t request_id);
  void OnNetworkRequestDone(content::RenderFrameHost* render_frame_host,
                            uint64_t request_id);

  // Prevents |extension|'s background page from being closed and sends the
  // onSuspendCanceled() event to it.
  void CancelSuspend(const Extension* extension);

  // Called on shutdown to close our extension hosts.
  void CloseBackgroundHosts();

  // EventPageTracker implementation.
  bool IsEventPageSuspended(const std::string& extension_id) override;
  bool WakeEventPage(const std::string& extension_id,
                     const base::Callback<void(bool)>& callback) override;

  // Sets the time in milliseconds that an extension event page can
  // be idle before it is shut down; must be > 0.
  static void SetEventPageIdleTimeForTesting(unsigned idle_time_msec);

  // Sets the time in milliseconds that an extension event page has
  // between being notified of its impending unload and that unload
  // happening.
  static void SetEventPageSuspendingTimeForTesting(
      unsigned suspending_time_msec);

  // Creates a non-incognito instance for tests. |registry| allows unit tests
  // to inject an ExtensionRegistry that is not managed by the usual
  // BrowserContextKeyedServiceFactory system.
  static ProcessManager* CreateForTesting(content::BrowserContext* context,
                                          ExtensionRegistry* registry);

  // Creates an incognito-context instance for tests.
  static ProcessManager* CreateIncognitoForTesting(
      content::BrowserContext* incognito_context,
      content::BrowserContext* original_context,
      ExtensionRegistry* registry);

  content::BrowserContext* browser_context() const { return browser_context_; }

  const ExtensionHostSet& background_hosts() const {
    return background_hosts_;
  }

  bool startup_background_hosts_created_for_test() const {
    return startup_background_hosts_created_;
  }

 protected:
  static ProcessManager* Create(content::BrowserContext* context);

  // |context| is incognito pass the master context as |original_context|.
  // Otherwise pass the same context for both. Pass the ExtensionRegistry for
  // |context| as |registry|, or override it for testing.
  ProcessManager(content::BrowserContext* context,
                 content::BrowserContext* original_context,
                 ExtensionRegistry* registry);

  // Not owned. Also used by IncognitoProcessManager.
  ExtensionRegistry* extension_registry_;

 private:
  friend class ProcessManagerFactory;
  friend class ProcessManagerTest;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  // Extra information we keep for each extension's background page.
  struct BackgroundPageData;
  struct ExtensionRenderFrameData;
  using BackgroundPageDataMap = std::map<ExtensionId, BackgroundPageData>;
  using ExtensionRenderFrames =
      std::map<content::RenderFrameHost*, ExtensionRenderFrameData>;

  // Load all background pages once the profile data is ready and the pages
  // should be loaded.
  void CreateStartupBackgroundHosts();

  // Called just after |host| is created so it can be registered in our lists.
  void OnBackgroundHostCreated(ExtensionHost* host);

  // Close the given |host| iff it's a background page.
  void CloseBackgroundHost(ExtensionHost* host);

  // If the frame isn't keeping the lazy background page alive, increments the
  // keepalive count to do so.
  void AcquireLazyKeepaliveCountForFrame(
      content::RenderFrameHost* render_frame_host);

  // If the frame is keeping the lazy background page alive, decrements the
  // keepalive count to stop doing it.
  void ReleaseLazyKeepaliveCountForFrame(
      content::RenderFrameHost* render_frame_host);

  // Internal implementation of DecrementLazyKeepaliveCount with an
  // |extension_id| known to have a lazy background page.
  void DecrementLazyKeepaliveCount(const std::string& extension_id);

  // These are called when the extension transitions between idle and active.
  // They control the process of closing the background page when idle.
  void OnLazyBackgroundPageIdle(const std::string& extension_id,
                                uint64_t sequence_id);
  void OnLazyBackgroundPageActive(const std::string& extension_id);
  void CloseLazyBackgroundPageNow(const std::string& extension_id,
                                  uint64_t sequence_id);

  const Extension* GetExtensionForAgentHost(
      content::DevToolsAgentHost* agent_host);

  // content::DevToolsAgentHostObserver overrides.
  void DevToolsAgentHostAttached(
      content::DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostDetached(
      content::DevToolsAgentHost* agent_host) override;

  // Unregister RenderFrameHosts and clear background page data for an extension
  // which has been unloaded.
  void UnregisterExtension(const std::string& extension_id);

  // Clears background page data for this extension.
  void ClearBackgroundPageData(const std::string& extension_id);

  content::NotificationRegistrar registrar_;

  // The set of ExtensionHosts running viewless background extensions.
  ExtensionHostSet background_hosts_;

  // A SiteInstance related to the SiteInstance for all extensions in
  // this profile.  We create it in such a way that a new
  // browsing instance is created.  This controls process grouping.
  scoped_refptr<content::SiteInstance> site_instance_;

  // The browser context associated with the |site_instance_|.
  content::BrowserContext* browser_context_;

  // Contains all active extension-related RenderFrameHost instances for all
  // extensions. We also keep a cache of the host's view type, because that
  // information is not accessible at registration/deregistration time.
  ExtensionRenderFrames all_extension_frames_;

  BackgroundPageDataMap background_page_data_;

  // True if we have created the startup set of background hosts.
  bool startup_background_hosts_created_;

  base::ObserverList<ProcessManagerObserver> observer_list_;

  // ID Counter used to set ProcessManager::BackgroundPageData close_sequence_id
  // members. These IDs are tracked per extension in background_page_data_ and
  // are used to verify that nothing has interrupted the process of closing a
  // lazy background process.
  //
  // Any interruption obtains a new ID by incrementing
  // last_background_close_sequence_id_ and storing it in background_page_data_
  // for a particular extension. Callbacks and round-trip IPC messages store the
  // value of the extension's close_sequence_id at the beginning of the process.
  // Thus comparisons can be done to halt when IDs no longer match.
  //
  // This counter provides unique IDs even when BackgroundPageData objects are
  // reset.
  uint64_t last_background_close_sequence_id_;

  // Tracks pending network requests by opaque ID. This is used to ensure proper
  // keepalive counting in response to request status updates; e.g., if an
  // extension URLRequest is constructed and then destroyed without ever
  // starting, we can receive a completion notification without a corresponding
  // start notification. In that case we want to avoid decrementing keepalive.
  std::map<int, ExtensionHost*> pending_network_requests_;

  // Must be last member, see doc on WeakPtrFactory.
  base::WeakPtrFactory<ProcessManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProcessManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_PROCESS_MANAGER_H_
