// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The Safe Browsing service is responsible for downloading anti-phishing and
// anti-malware tables and checking urls against them.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_SERVICE_H_
#define CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/sequenced_task_runner_helpers.h"
#include "chrome/browser/net/proxy_config_monitor.h"
#include "chrome/browser/safe_browsing/services_delegate.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/db/util.h"
#include "components/safe_browsing/db/v4_feature_list.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"

#if defined(FULL_SAFE_BROWSING)
#include "chrome/browser/safe_browsing/incident_reporting/delayed_analysis_callback.h"
#endif

class PrefChangeRegistrar;
class PrefService;
class Profile;

namespace content {
class DownloadManager;
}

namespace net {
class URLRequest;
class URLRequestContextGetter;
}  // namespace net

namespace network {
namespace mojom {
class NetworkContext;
}
class SharedURLLoaderFactory;
}  // namespace network

namespace prefs {
namespace mojom {
class TrackedPreferenceValidationDelegate;
}
}  // namespace prefs

namespace safe_browsing {
class ClientSideDetectionService;
class DownloadProtectionService;
class PasswordProtectionService;
struct ResourceRequestInfo;
struct SafeBrowsingProtocolConfig;
class SafeBrowsingDatabaseManager;
class SafeBrowsingNavigationObserverManager;
class SafeBrowsingNetworkContext;
class SafeBrowsingPingManager;
class SafeBrowsingProtocolManager;
class SafeBrowsingProtocolManagerDelegate;
class SafeBrowsingServiceFactory;
class SafeBrowsingUIManager;
class SafeBrowsingURLRequestContextGetter;
class TriggerManager;
struct V4ProtocolConfig;

// Construction needs to happen on the main thread.
// The SafeBrowsingService owns both the UI and Database managers which do
// the heavylifting of safebrowsing service. Both of these managers stay
// alive until SafeBrowsingService is destroyed, however, they are disabled
// permanently when Shutdown method is called.
class SafeBrowsingService : public base::RefCountedThreadSafe<
                                SafeBrowsingService,
                                content::BrowserThread::DeleteOnUIThread>,
                            public content::NotificationObserver {
 public:
  // Makes the passed |factory| the factory used to instanciate
  // a SafeBrowsingService. Useful for tests.
  static void RegisterFactory(SafeBrowsingServiceFactory* factory) {
    factory_ = factory;
  }

  static base::FilePath GetCookieFilePathForTesting();

  static base::FilePath GetBaseFilename();

  // Create an instance of the safe browsing service.
  static SafeBrowsingService* CreateSafeBrowsingService();

  // Called on the UI thread to initialize the service.
  void Initialize();

  // Called on the main thread to let us know that the io_thread is going away.
  void ShutDown();

  // Called on UI thread to decide if the download file's sha256 hash
  // should be calculated for safebrowsing.
  bool DownloadBinHashNeeded() const;

  // Create a protocol config struct.
  virtual SafeBrowsingProtocolConfig GetProtocolConfig() const;

  // Create a v4 protocol config struct.
  virtual V4ProtocolConfig GetV4ProtocolConfig() const;

  // Returns the client_name field for both V3 and V4 protocol manager configs.
  std::string GetProtocolConfigClientName() const;

  // NOTE(vakh): This is not the most reliable way to find out if extended
  // reporting has been enabled. That's why it starts with estimated_. It
  // returns true if any of the profiles have extended reporting enabled. It may
  // be called on any thread. That can lead to a race condition, but that's
  // acceptable.
  ExtendedReportingLevel estimated_extended_reporting_by_prefs() const {
    return estimated_extended_reporting_by_prefs_;
  }

  // Get current enabled status. Must be called on IO thread.
  bool enabled() const {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    return enabled_;
  }

  // Whether the service is enabled by the current set of profiles.
  bool enabled_by_prefs() const {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return enabled_by_prefs_;
  }

  ClientSideDetectionService* safe_browsing_detection_service() const {
    return services_delegate_->GetCsdService();
  }

  // The DownloadProtectionService is not valid after the SafeBrowsingService
  // is destroyed.
  DownloadProtectionService* download_protection_service() const {
    return services_delegate_->GetDownloadService();
  }

  scoped_refptr<net::URLRequestContextGetter> url_request_context();

  // NetworkContext and URLLoaderFactory used for safe browsing requests.
  // Called on UI thread.
  network::mojom::NetworkContext* GetNetworkContext();
  virtual scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  // Flushes above two interfaces to avoid races in tests.
  void FlushNetworkInterfaceForTesting();

  // Called to get a SharedURLLoaderFactory that can be used on the IO thread.
  scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactoryOnIOThread();

  const scoped_refptr<SafeBrowsingUIManager>& ui_manager() const;

  // This returns either the v3 or the v4 database manager, depending on
  // the experiment settings.
  const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager() const;

  scoped_refptr<SafeBrowsingNavigationObserverManager>
  navigation_observer_manager();

  SafeBrowsingProtocolManager* protocol_manager() const;

  // Called on UI thread.
  SafeBrowsingPingManager* ping_manager() const;

  // This may be NULL if v4 is not enabled by experiment.
  const scoped_refptr<SafeBrowsingDatabaseManager>& v4_local_database_manager()
      const;

  TriggerManager* trigger_manager() const;

  // Gets PasswordProtectionService by profile.
  PasswordProtectionService* GetPasswordProtectionService(
      Profile* profile) const;

  // Returns a preference validation delegate that adds incidents to the
  // incident reporting service for validation failures. Returns NULL if the
  // service is not applicable for the given profile.
  std::unique_ptr<prefs::mojom::TrackedPreferenceValidationDelegate>
  CreatePreferenceValidationDelegate(Profile* profile) const;

  // Registers |callback| to be run after some delay following process launch.
  // |callback| will be dropped if the service is not applicable for the
  // process.
  void RegisterDelayedAnalysisCallback(const DelayedAnalysisCallback& callback);

  // Adds |download_manager| to the set monitored by safe browsing.
  void AddDownloadManager(content::DownloadManager* download_manager);

  // Observes resource requests made by the renderer and reports suspicious
  // activity.
  void OnResourceRequest(const net::URLRequest* request);

  // Type for subscriptions to SafeBrowsing service state.
  typedef base::CallbackList<void(void)>::Subscription StateSubscription;
  typedef base::CallbackList<void(void)>::Subscription ShutdownSubscription;

  // Adds a listener for when SafeBrowsing preferences might have changed.
  // To get the current state, the callback should call enabled_by_prefs().
  // Should only be called on the UI thread.
  std::unique_ptr<StateSubscription> RegisterStateCallback(
      const base::Callback<void(void)>& callback);

  // Sends serialized download report to backend.
  virtual void SendSerializedDownloadReport(const std::string& report);

 protected:
  // Creates the safe browsing service.  Need to initialize before using.
  SafeBrowsingService(V4FeatureList::V4UsageStatus v4_usage_status =
                          V4FeatureList::V4UsageStatus::V4_DISABLED);

  ~SafeBrowsingService() override;

  virtual SafeBrowsingDatabaseManager* CreateDatabaseManager();

  virtual SafeBrowsingUIManager* CreateUIManager();

  // Registers all the delayed analysis with the incident reporting service.
  // This is where you register your process-wide, profile-independent analysis.
  virtual void RegisterAllDelayedAnalysis();

  // Return a ptr to DatabaseManager's delegate, or NULL if it doesn't have one.
  virtual SafeBrowsingProtocolManagerDelegate* GetProtocolManagerDelegate();

  std::unique_ptr<ServicesDelegate> services_delegate_;

 private:
  friend class SafeBrowsingServiceFactoryImpl;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<SafeBrowsingService>;
  friend class SafeBrowsingBlockingPageTest;
  friend class SafeBrowsingBlockingQuietPageTest;
  friend class SafeBrowsingServerTest;
  friend class SafeBrowsingServiceTest;
  friend class SafeBrowsingUIManagerTest;
  friend class SafeBrowsingURLRequestContextGetter;
  friend class TestSafeBrowsingService;
  friend class TestSafeBrowsingServiceFactory;

  // Called to initialize objects that are used on the io_thread.  This may be
  // called multiple times during the life of the SafeBrowsingService.
  void StartOnIOThread();

  // Called to stop or shutdown operations on the io_thread. This may be called
  // multiple times to stop during the life of the SafeBrowsingService. If
  // shutdown is true, then the operations on the io thread are shutdown
  // permanently and cannot be restarted.
  void StopOnIOThread(bool shutdown);

  // Start up SafeBrowsing objects. This can be called at browser start, or when
  // the user checks the "Enable SafeBrowsing" option in the Advanced options
  // UI.
  void Start();

  // Stops the SafeBrowsingService. This can be called when the safe browsing
  // preference is disabled. When shutdown is true, operation is permanently
  // shutdown and cannot be restarted.
  void Stop(bool shutdown);

  // content::NotificationObserver override
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Starts following the safe browsing preference on |pref_service|.
  void AddPrefService(PrefService* pref_service);

  // Stop following the safe browsing preference on |pref_service|.
  void RemovePrefService(PrefService* pref_service);

  // Checks if any profile is currently using the safe browsing service, and
  // starts or stops the service accordingly.
  void RefreshState();

  // Process the observed resource requests on the UI thread.
  void ProcessResourceRequest(const ResourceRequestInfo& request);

  void CreateTriggerManager();

  // Called on the UI thread to create a URLLoaderFactory interface ptr for
  // the IO thread.
  void CreateURLLoaderFactoryForIO(
      network::mojom::URLLoaderFactoryRequest request);

  // Creates a configured NetworkContextParams when the network service is in
  // use.
  network::mojom::NetworkContextParamsPtr CreateNetworkContextParams();

  // The factory used to instantiate a SafeBrowsingService object.
  // Useful for tests, so they can provide their own implementation of
  // SafeBrowsingService.
  static SafeBrowsingServiceFactory* factory_;

  // The SafeBrowsingURLRequestContextGetter used to access
  // |url_request_context_|. Accessed on UI thread.
  // This is only valid if the network service is disabled.
  scoped_refptr<SafeBrowsingURLRequestContextGetter>
      url_request_context_getter_;

  std::unique_ptr<ProxyConfigMonitor> proxy_config_monitor_;

  // If the network service is disabled, this is a wrapper around
  // |url_request_context_getter_|. Otherwise it's what owns the
  // URLRequestContext inside the network service. This is used by
  // SimpleURLLoader for safe browsing requests.
  std::unique_ptr<safe_browsing::SafeBrowsingNetworkContext> network_context_;

  // A SharedURLLoaderFactory and its interfaceptr used on the IO thread.
  network::mojom::URLLoaderFactoryPtr url_loader_factory_on_io_;
  scoped_refptr<network::WeakWrapperSharedURLLoaderFactory>
      shared_url_loader_factory_on_io_;

#if defined(SAFE_BROWSING_DB_LOCAL)
  // Handles interaction with SafeBrowsing servers. Accessed on IO thread.
  std::unique_ptr<SafeBrowsingProtocolManager> protocol_manager_;
#endif

  // Provides phishing and malware statistics. Accessed on UI thread.
  std::unique_ptr<SafeBrowsingPingManager> ping_manager_;

  // Whether SafeBrowsing Extended Reporting is enabled by the current set of
  // profiles. Updated on the UI thread.
  ExtendedReportingLevel estimated_extended_reporting_by_prefs_;

  // Whether the service has been shutdown.
  bool shutdown_;

  // Whether the service is running. 'enabled_' is used by SafeBrowsingService
  // on the IO thread during normal operations.
  bool enabled_;

  // Whether SafeBrowsing is enabled by the current set of profiles.
  // Accessed on UI thread.
  bool enabled_by_prefs_;

  // Whether SafeBrowsing needs to be enabled in V4Only mode. In this mode, all
  // SafeBrowsing decisions are made using the PVer4 implementation.
  bool use_v4_only_;

  // Whether the PVer4 implementation needs to be instantiated. Note that even
  // if the PVer4 implementation has been instantiated, it is used only if
  // |use_v4_only_| is true.
  bool v4_enabled_;

  // Tracks existing PrefServices, and the safe browsing preference on each.
  // This is used to determine if any profile is currently using the safe
  // browsing service, and to start it up or shut it down accordingly.
  // Accessed on UI thread.
  std::map<PrefService*, std::unique_ptr<PrefChangeRegistrar>> prefs_map_;

  // Used to track creation and destruction of profiles on the UI thread.
  content::NotificationRegistrar profiles_registrar_;

  // Callbacks when SafeBrowsing state might have changed.
  // Should only be accessed on the UI thread.
  base::CallbackList<void(void)> state_callback_list_;

  // The UI manager handles showing interstitials.  Accessed on both UI and IO
  // thread.
  scoped_refptr<SafeBrowsingUIManager> ui_manager_;

  // The database manager handles the database and download logic.  Accessed on
  // both UI and IO thread.
  scoped_refptr<SafeBrowsingDatabaseManager> database_manager_;

  // The navigation observer manager handles attribution of safe browsing
  // events.
  scoped_refptr<SafeBrowsingNavigationObserverManager>
      navigation_observer_manager_;

  std::unique_ptr<TriggerManager> trigger_manager_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingService);
};

// Factory for creating SafeBrowsingService.  Useful for tests.
class SafeBrowsingServiceFactory {
 public:
  SafeBrowsingServiceFactory() {}
  virtual ~SafeBrowsingServiceFactory() {}
  virtual SafeBrowsingService* CreateSafeBrowsingService() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingServiceFactory);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_SERVICE_H_
