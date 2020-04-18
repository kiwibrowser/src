// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_UPDATER_EXTENSION_DOWNLOADER_H_
#define EXTENSIONS_BROWSER_UPDATER_EXTENSION_DOWNLOADER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/version.h"
#include "extensions/browser/updater/extension_downloader_delegate.h"
#include "extensions/browser/updater/manifest_fetch_data.h"
#include "extensions/browser/updater/request_queue.h"
#include "extensions/browser/updater/safe_manifest_parser.h"
#include "extensions/common/extension.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
class URLRequestStatus;
}

namespace service_manager {
class Connector;
}

namespace extensions {

struct UpdateDetails {
  UpdateDetails(const std::string& id, const base::Version& version);
  ~UpdateDetails();

  std::string id;
  base::Version version;
};

class ExtensionCache;
class ExtensionDownloaderTestDelegate;
class ExtensionUpdaterTest;

// A class that checks for updates of a given list of extensions, and downloads
// the crx file when updates are found. It uses a |ExtensionDownloaderDelegate|
// that takes ownership of the downloaded crx files, and handles events during
// the update check.
class ExtensionDownloader : public net::URLFetcherDelegate,
                            public OAuth2TokenService::Consumer {
 public:
  // A closure which constructs a new ExtensionDownloader to be owned by the
  // caller.
  using Factory = base::RepeatingCallback<std::unique_ptr<ExtensionDownloader>(
      ExtensionDownloaderDelegate* delegate)>;

  // A closure that returns the account to use for authentication to the
  // webstore.
  using GetWebstoreAccountCallback =
      base::RepeatingCallback<const std::string&()>;

  // |delegate| is stored as a raw pointer and must outlive the
  // ExtensionDownloader.
  ExtensionDownloader(ExtensionDownloaderDelegate* delegate,
                      net::URLRequestContextGetter* request_context,
                      service_manager::Connector* connector);
  ~ExtensionDownloader() override;

  // Adds |extension| to the list of extensions to check for updates.
  // Returns false if the |extension| can't be updated due to invalid details.
  // In that case, no callbacks will be performed on the |delegate_|.
  // The |request_id| is passed on as is to the various |delegate_| callbacks.
  // This is used for example by ExtensionUpdater to keep track of when
  // potentially concurrent update checks complete. |fetch_priority|
  // parameter notifies the downloader the priority of this extension update
  // (either foreground or background).
  bool AddExtension(const Extension& extension,
                    int request_id,
                    ManifestFetchData::FetchPriority fetch_priority);

  // Adds extension |id| to the list of extensions to check for updates.
  // Returns false if the |id| can't be updated due to invalid details.
  // In that case, no callbacks will be performed on the |delegate_|.
  // The |request_id| is passed on as is to the various |delegate_| callbacks.
  // This is used for example by ExtensionUpdater to keep track of when
  // potentially concurrent update checks complete. The |is_corrupt_reinstall|
  // parameter is used to indicate in the request that we detected corruption in
  // the local copy of the extension and we want to perform a reinstall of it.
  // |fetch_priority| parameter notifies the downloader the priority of this
  // extension update (either foreground or background).
  bool AddPendingExtension(const std::string& id,
                           const GURL& update_url,
                           Manifest::Location install_source,
                           bool is_corrupt_reinstall,
                           int request_id,
                           ManifestFetchData::FetchPriority fetch_priority);

  // Schedules a fetch of the manifest of all the extensions added with
  // AddExtension() and AddPendingExtension().
  void StartAllPending(ExtensionCache* cache);

  // Schedules an update check of the blacklist.
  void StartBlacklistUpdate(const std::string& version,
                            const ManifestFetchData::PingData& ping_data,
                            int request_id);

  // Sets GetWebstoreAccountCallback and TokenService instances to be used for
  // OAuth2 authentication on protected Webstore downloads. Both objects must be
  // valid to use for the lifetime of this object.
  void SetWebstoreAuthenticationCapabilities(
      const GetWebstoreAccountCallback& webstore_account_callback,
      OAuth2TokenService* token_service);

  void set_brand_code(const std::string& brand_code) {
    brand_code_ = brand_code;
  }

  void set_manifest_query_params(const std::string& params) {
    manifest_query_params_ = params;
  }

  void set_ping_enabled_domain(const std::string& domain) {
    ping_enabled_domain_ = domain;
  }

  // Sets a test delegate to use by any instances of this class. The |delegate|
  // should outlive all instances.
  static void set_test_delegate(ExtensionDownloaderTestDelegate* delegate);

  // These are needed for unit testing, to help identify the correct mock
  // URLFetcher objects.
  static const int kManifestFetcherId = 1;
  static const int kExtensionFetcherId = 2;

  // Update AppID for extension blacklist.
  static const char kBlacklistAppID[];

  static const int kMaxRetries = 10;

  // Names of the header fields used for traffic management for extension
  // updater.
  static const char kUpdateInteractivityHeader[];
  static const char kUpdateAppIdHeader[];
  static const char kUpdateUpdaterHeader[];

  // Header values for foreground/background update requests.
  static const char kUpdateInteractivityForeground[];
  static const char kUpdateInteractivityBackground[];

 private:
  friend class ExtensionUpdaterTest;

  // These counters are bumped as extensions are added to be fetched. They
  // are then recorded as UMA metrics when all the extensions have been added.
  struct URLStats {
    URLStats()
        : no_url_count(0),
          google_url_count(0),
          other_url_count(0),
          extension_count(0),
          theme_count(0),
          app_count(0),
          platform_app_count(0),
          pending_count(0) {}

    int no_url_count, google_url_count, other_url_count;
    int extension_count, theme_count, app_count, platform_app_count,
        pending_count;
  };

  // We need to keep track of some information associated with a url
  // when doing a fetch.
  struct ExtensionFetch {
    ExtensionFetch();
    ExtensionFetch(const std::string& id,
                   const GURL& url,
                   const std::string& package_hash,
                   const std::string& version,
                   const std::set<int>& request_ids);
    ~ExtensionFetch();

    std::string id;
    GURL url;
    std::string package_hash;
    std::string version;
    std::set<int> request_ids;

    enum CredentialsMode {
      CREDENTIALS_NONE = 0,
      CREDENTIALS_OAUTH2_TOKEN,
      CREDENTIALS_COOKIES,
    };

    // Indicates the type of credentials to include with this fetch.
    CredentialsMode credentials;

    // Counts the number of times OAuth2 authentication has been attempted for
    // this fetch.
    int oauth2_attempt_count;
  };

  // Parameters for special cases that aren't used for most requests.
  struct ExtraParams {
    // Additional data to be passed up in the update request.
    std::string update_url_data;

    // Indicates whether this extension is being reinstalled due to corruption.
    bool is_corrupt_reinstall;

    ExtraParams();
  };

  // Helper for AddExtension() and AddPendingExtension().
  bool AddExtensionData(const std::string& id,
                        const base::Version& version,
                        Manifest::Type extension_type,
                        Manifest::Location extension_location,
                        const GURL& extension_update_url,
                        const ExtraParams& extra,
                        int request_id,
                        ManifestFetchData::FetchPriority fetch_priority);

  // Adds all recorded stats taken so far to histogram counts.
  void ReportStats() const;

  // Begins an update check.
  void StartUpdateCheck(std::unique_ptr<ManifestFetchData> fetch_data);

  // Called by RequestQueue when a new manifest fetch request is started.
  void CreateManifestFetcher();

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Handles the result of a manifest fetch.
  void OnManifestFetchComplete(const GURL& url,
                               const net::URLRequestStatus& status,
                               int response_code,
                               const base::TimeDelta& backoff_delay,
                               const std::string& data);

  // Once a manifest is parsed, this starts fetches of any relevant crx files.
  // If |results| is null, it means something went wrong when parsing it.
  void HandleManifestResults(std::unique_ptr<ManifestFetchData> fetch_data,
                             std::unique_ptr<UpdateManifestResults> results,
                             const base::Optional<std::string>& error);

  // Given a list of potential updates, returns the indices of the ones that are
  // applicable (are actually a new version, etc.) in |result|.
  void DetermineUpdates(const ManifestFetchData& fetch_data,
                        const UpdateManifestResults& possible_updates,
                        std::vector<int>* result);

  // Begins (or queues up) download of an updated extension.
  void FetchUpdatedExtension(std::unique_ptr<ExtensionFetch> fetch_data);

  // Called by RequestQueue when a new extension fetch request is started.
  void CreateExtensionFetcher();

  // Handles the result of a crx fetch.
  void OnCRXFetchComplete(const net::URLFetcher* source,
                          const GURL& url,
                          const net::URLRequestStatus& status,
                          int response_code,
                          const base::TimeDelta& backoff_delay);

  // Invokes OnExtensionDownloadFailed() on the |delegate_| for each extension
  // in the set, with |error| as the reason for failure.
  void NotifyExtensionsDownloadFailed(const std::set<std::string>& id_set,
                                      const std::set<int>& request_ids,
                                      ExtensionDownloaderDelegate::Error error);

  // Send a notification that an update was found for |id| that we'll
  // attempt to download.
  void NotifyUpdateFound(const std::string& id, const std::string& version);

  // Do real work of StartAllPending. If .crx cache is used, this function
  // is called when cache is ready.
  void DoStartAllPending();

  // Notify delegate and remove ping results.
  void NotifyDelegateDownloadFinished(
      std::unique_ptr<ExtensionFetch> fetch_data,
      bool from_cache,
      const base::FilePath& crx_path,
      bool file_ownership_passed);

  // Cached extension installation completed. If it was not successful, we will
  // try to download it from the web store using already fetched manifest.
  void CacheInstallDone(std::unique_ptr<ExtensionFetch> fetch_data,
                        bool installed);

  // Potentially updates an ExtensionFetch's authentication state and returns
  // |true| if the fetch should be retried. Returns |false| if the failure was
  // not related to authentication, leaving the ExtensionFetch data unmodified.
  bool IterateFetchCredentialsAfterFailure(ExtensionFetch* fetch,
                                           const net::URLRequestStatus& status,
                                           int response_code);

  // OAuth2TokenService::Consumer implementation.
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  ManifestFetchData* CreateManifestFetchData(
      const GURL& update_url,
      int request_id,
      ManifestFetchData::FetchPriority fetch_priority);

  // The delegate that receives the crx files downloaded by the
  // ExtensionDownloader, and that fills in optional ping and update url data.
  ExtensionDownloaderDelegate* delegate_;

  // The request context to use for the URLFetchers.
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // The connector to the ServiceManager.
  service_manager::Connector* connector_;

  // Collects UMA samples that are reported when ReportStats() is called.
  URLStats url_stats_;

  // List of data on fetches we're going to do. We limit the number of
  // extensions grouped together in one batch to avoid running into the limits
  // on the length of http GET requests, so there might be multiple
  // ManifestFetchData* objects with the same base_url.
  using FetchMap = std::map<std::pair<int, GURL>,
                            std::vector<std::unique_ptr<ManifestFetchData>>>;
  FetchMap fetches_preparing_;

  // Outstanding url fetch requests for manifests and updates.
  std::unique_ptr<net::URLFetcher> manifest_fetcher_;
  std::unique_ptr<net::URLFetcher> extension_fetcher_;

  // Pending manifests and extensions to be fetched when the appropriate fetcher
  // is available.
  RequestQueue<ManifestFetchData> manifests_queue_;
  RequestQueue<ExtensionFetch> extensions_queue_;

  // Maps an extension-id to its PingResult data.
  std::map<std::string, ExtensionDownloaderDelegate::PingResult> ping_results_;

  // Cache for .crx files.
  ExtensionCache* extension_cache_;

  // Gets the account to use for protected download requests. May be null. If
  // non-null, valid to call for the lifetime of this object.
  GetWebstoreAccountCallback webstore_account_callback_;

  // May be used to fetch access tokens for protected download requests. May be
  // null. If non-null, guaranteed to outlive this object.
  OAuth2TokenService* token_service_;

  // A Webstore download-scoped access token for the |identity_provider_|'s
  // active account, if any.
  std::string access_token_;

  // A pending token fetch request.
  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;

  // Brand code to include with manifest fetch queries if sending ping data.
  std::string brand_code_;

  // Baseline parameters to include with manifest fetch queries.
  std::string manifest_query_params_;

  // Domain to enable ping data. Ping data will be sent with manifest fetches
  // to update URLs which match this domain. Defaults to empty (no domain).
  std::string ping_enabled_domain_;

  // Used to create WeakPtrs to |this|.
  base::WeakPtrFactory<ExtensionDownloader> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionDownloader);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_UPDATER_EXTENSION_DOWNLOADER_H_
