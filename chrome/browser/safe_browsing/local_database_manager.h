// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Safe Browsing Database Manager implementation that manages a local
// database.  This is used by Desktop Chromium.

#ifndef CHROME_BROWSER_SAFE_BROWSING_LOCAL_DATABASE_MANAGER_H_
#define CHROME_BROWSER_SAFE_BROWSING_LOCAL_DATABASE_MANAGER_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "chrome/browser/safe_browsing/protocol_manager.h"
#include "chrome/browser/safe_browsing/safe_browsing_util.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/db/database_manager.h"
#include "components/safe_browsing/db/safebrowsing.pb.h"
#include "components/safe_browsing/db/util.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "url/gurl.h"

namespace base {
class SequencedTaskRunner;
}

namespace safe_browsing {

class SafeBrowsingService;
class SafeBrowsingDatabase;
struct V4ProtocolConfig;

// Implementation that manages a local database on disk.
//
// Construction needs to happen on the main thread.
class LocalSafeBrowsingDatabaseManager
    : public SafeBrowsingDatabaseManager,
      public SafeBrowsingProtocolManagerDelegate {
 public:
  // Bundle of SafeBrowsing state while performing a URL or hash prefix check.
  struct SafeBrowsingCheck {
    // |check_type| should correspond to the type of item that is being
    // checked, either a URL or a binary hash/URL. We store this for two
    // purposes: to know which of Client's methods to call when a result is
    // known, and for logging purposes. It *isn't* used to predict the response
    // list type, that is information that the server gives us.
    SafeBrowsingCheck(const std::vector<GURL>& urls,
                      const std::vector<SBFullHash>& full_hashes,
                      Client* client,
                      ListType check_type,
                      const SBThreatTypeSet& expected_threats);
    ~SafeBrowsingCheck();

    // Either |urls| or |full_hashes| is used to lookup database. |*_results|
    // are parallel vectors containing the results. They are initialized to
    // contain SB_THREAT_TYPE_SAFE.
    // |url_hit_hash| and |url_metadata| are parallel vectors containing full
    // hash and metadata of a database record provided the result. They are
    // initialized to be empty strings.
    std::vector<GURL> urls;
    std::vector<SBThreatType> url_results;
    std::vector<ThreatMetadata> url_metadata;
    std::vector<std::string> url_hit_hash;
    std::vector<SBFullHash> full_hashes;
    std::vector<SBThreatType> full_hash_results;

    SafeBrowsingDatabaseManager::Client* client;
    ExtendedReportingLevel extended_reporting_level;
    bool need_get_hash;
    base::TimeTicks start;  // When check was sent to SB service.
    ListType check_type;    // See comment in constructor.
    SBThreatTypeSet expected_threats;
    std::vector<SBPrefix> prefix_hits;
    std::vector<SBFullHashResult> cache_hits;

    // Invoke one of client's callbacks with these results.
    void OnSafeBrowsingResult();

    // Vends weak pointers for async callbacks on the IO thread, such as
    // timeout checks and replies from checks performed on the SB task runner.
    // TODO(lzheng): We should consider to use this time out check
    // for browsing too (instead of implementing in
    // safe_browsing_resource_handler.cc).
    std::unique_ptr<base::WeakPtrFactory<LocalSafeBrowsingDatabaseManager>>
        weak_ptr_factory_;

   private:
    DISALLOW_COPY_AND_ASSIGN(SafeBrowsingCheck);
  };

  // Creates the safe browsing service.  Need to initialize before using.
  LocalSafeBrowsingDatabaseManager(
      const scoped_refptr<SafeBrowsingService>& service);

  //
  // SafeBrowsingDatabaseManager overrides
  //

  bool IsSupported() const override;
  safe_browsing::ThreatSource GetThreatSource() const override;
  bool ChecksAreAlwaysAsync() const override;
  bool CanCheckResourceType(content::ResourceType resource_type) const override;
  bool CanCheckSubresourceFilter() const override;
  bool CanCheckUrl(const GURL& url) const override;

  bool CheckBrowseUrl(const GURL& url,
                      const SBThreatTypeSet& threat_types,
                      Client* client) override;
  bool CheckUrlForSubresourceFilter(const GURL& url, Client* client) override;
  bool CheckDownloadUrl(const std::vector<GURL>& url_chain,
                        Client* client) override;
  bool CheckExtensionIDs(const std::set<std::string>& extension_ids,
                         Client* client) override;
  bool CheckResourceUrl(const GURL& url, Client* client) override;
  AsyncMatch CheckCsdWhitelistUrl(const GURL& url, Client* client) override;
  bool MatchMalwareIP(const std::string& ip_address) override;
  bool MatchDownloadWhitelistUrl(const GURL& url) override;
  bool MatchDownloadWhitelistString(const std::string& str) override;
  void CancelCheck(Client* client) override;
  void StartOnIOThread(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const V4ProtocolConfig& config) override;
  void StopOnIOThread(bool shutdown) override;
  bool IsDownloadProtectionEnabled() const override;

 protected:
  ~LocalSafeBrowsingDatabaseManager() override;

  // protected for tests.
  void NotifyDatabaseUpdateFinished(bool update_succeeded);

 private:
  // Called on the IO thread when the SafeBrowsingProtocolManager has received
  // the full hash results for prefix hits detected in the database.
  void HandleGetHashResults(SafeBrowsingCheck* check,
                            const std::vector<SBFullHashResult>& full_hashes,
                            const base::TimeDelta& cache_lifetime);
  bool MatchCsdWhitelistUrl(const GURL& url);  // deprecated

  friend class base::RefCountedThreadSafe<LocalSafeBrowsingDatabaseManager>;
  friend class SafeBrowsingServerTest;
  friend class SafeBrowsingServiceTest;
  friend class SafeBrowsingServiceTestHelper;
  friend class LocalDatabaseManagerTest;
  FRIEND_TEST_ALL_PREFIXES(LocalDatabaseManagerTest, GetUrlSeverestThreatType);
  FRIEND_TEST_ALL_PREFIXES(LocalDatabaseManagerTest,
                           ServiceStopWithPendingChecks);

  typedef std::vector<SafeBrowsingCheck*> GetHashRequestors;
  typedef std::map<SBPrefix, GetHashRequestors> GetHashRequests;

  // Clients that we've queued up for checking later once the database is ready.
  struct QueuedCheck {
    QueuedCheck(const ListType check_type,
                Client* client,
                const GURL& url,
                const SBThreatTypeSet& expected_threats,
                const base::TimeTicks& start);
    QueuedCheck(const QueuedCheck& other);
    ~QueuedCheck();
    ListType check_type;
    Client* client;
    GURL url;
    SBThreatTypeSet expected_threats;
    base::TimeTicks start;  // When check was queued.
  };

  // Return the threat type of the severest entry in |full_hashes| which matches
  // |hash|, or SAFE if none match.
  static SBThreatType GetHashSeverestThreatType(
      const SBFullHash& hash,
      const std::vector<SBFullHashResult>& full_hashes);

  // Given a URL, compare all the possible host + path full hashes to the set of
  // provided full hashes.  Returns the threat type of the severest matching
  // result from |full_hashes|, or SAFE if none match.
  static SBThreatType GetUrlSeverestThreatType(
      const GURL& url,
      const std::vector<SBFullHashResult>& full_hashes,
      size_t* index);

  // Called to stop operations on the io_thread. This may be called multiple
  // times during the life of the DatabaseManager. Should be called on IO
  // thread.
  void DoStopOnIOThread();

  // Returns whether |database_| exists and is accessible.
  bool DatabaseAvailable() const;

  // Called on the IO thread.  If the database does not exist, queues up a call
  // on the db thread to create it.  Returns whether the database is available.
  //
  // Note that this is only needed outside the db thread, since functions on the
  // db thread can call GetDatabase() directly.
  bool MakeDatabaseAvailable();

  // Should only be called on db thread as SafeBrowsingDatabase is not
  // threadsafe.
  SafeBrowsingDatabase* GetDatabase();

  // Called on the IO thread with the check result.
  void OnCheckDone(SafeBrowsingCheck* info);

  // Called on the UI thread to prepare hash request.
  void OnRequestFullHash(SafeBrowsingCheck* check);

  // Called on the UI thread to determine what level of extended reporting the
  // current profile is opted into.
  ExtendedReportingLevel GetExtendedReporting();

  // Called on the IO thread to request full hash.
  void RequestFullHash(SafeBrowsingCheck* check);

  // Called on the database thread to retrieve chunks.
  void GetAllChunksFromDatabase(GetChunksCallback callback);

  // Called on the UI thread to prepare GetAllChunksFromDatabase.
  void BeforeGetAllChunksFromDatabase(
      const std::vector<SBListChunkRanges>& lists,
      bool database_error,
      GetChunksCallback callback);

  // Called on the IO thread with the results of all chunks.
  void OnGetAllChunksFromDatabase(const std::vector<SBListChunkRanges>& lists,
                                  bool database_error,
                                  ExtendedReportingLevel reporting_level,
                                  GetChunksCallback callback);

  // Called on the IO thread after the database reports that it added a chunk.
  void OnAddChunksComplete(AddChunksCallback callback);

  // Notification that the database is done loading its bloom filter.  We may
  // have had to queue checks until the database is ready, and if so, this
  // checks them.
  void DatabaseLoadComplete();

  // Called on the database thread to add/remove chunks and host keys.
  void AddDatabaseChunks(
      const std::string& list,
      std::unique_ptr<std::vector<std::unique_ptr<SBChunkData>>> chunks,
      AddChunksCallback callback);

  void DeleteDatabaseChunks(
      std::unique_ptr<std::vector<SBChunkDelete>> chunk_deletes);

  void NotifyClientBlockingComplete(Client* client, bool proceed);

  void DatabaseUpdateFinished(bool update_succeeded);

  // Called on the db thread to close the database.  See CloseDatabase().
  void OnCloseDatabase();

  // Runs on the db thread to reset the database. We assume that resetting the
  // database is a synchronous operation.
  void OnResetDatabase();

  // Internal worker function for processing full hashes.
  void OnHandleGetHashResults(SafeBrowsingCheck* check,
                              const std::vector<SBFullHashResult>& full_hashes);

  // Run one check against |full_hashes|.  Returns |true| if the check
  // finds a match in |full_hashes|.
  bool HandleOneCheck(SafeBrowsingCheck* check,
                      const std::vector<SBFullHashResult>& full_hashes);

  // Invoked by CheckDownloadUrl. It checks the download URL on
  // |safe_browsing_task_runner_|.
  std::vector<SBPrefix> CheckDownloadUrlOnSBThread(
      const std::vector<SBPrefix>& prefixes);

  // The callback function when a safebrowsing check is timed out. Client will
  // be notified that the safebrowsing check is SAFE when this happens.
  void TimeoutCallback(SafeBrowsingCheck* check);

  // Calls the Client's callback on IO thread after CheckDownloadUrl finishes.
  void OnAsyncCheckDone(SafeBrowsingCheck* check,
                        const std::vector<SBPrefix>& prefix_hits);

  // Checks all extension ID hashes on |safe_browsing_task_runner_|.
  std::vector<SBPrefix> CheckExtensionIDsOnSBThread(
      const std::vector<SBPrefix>& prefixes);

  // Checks all resource URL hashes on |safe_browsing_task_runner_|.
  std::vector<SBPrefix> CheckResourceUrlOnSBThread(
      const std::vector<SBPrefix>& prefixes);

  // Helper function that calls safe browsing client and cleans up |checks_|.
  void SafeBrowsingCheckDone(SafeBrowsingCheck* check);

  // Helper function to set |check| with default values and start a safe
  // browsing check with timeout of |timeout|. |task| will be called on
  // success, otherwise TimeoutCallback will be called.
  void StartSafeBrowsingCheck(
      std::unique_ptr<SafeBrowsingCheck> check,
      const base::Callback<std::vector<SBPrefix>(void)>& task);

  // SafeBrowsingProtocolManageDelegate override
  void ResetDatabase() override;
  void UpdateStarted() override;
  void UpdateFinished(bool success) override;
  void GetChunks(GetChunksCallback callback) override;
  void AddChunks(
      const std::string& list,
      std::unique_ptr<std::vector<std::unique_ptr<SBChunkData>>> chunks,
      AddChunksCallback callback) override;
  void DeleteChunks(
      std::unique_ptr<std::vector<SBChunkDelete>> chunk_deletes) override;

  scoped_refptr<SafeBrowsingService> sb_service_;

  std::map<SafeBrowsingCheck*, std::unique_ptr<SafeBrowsingCheck>> checks_;

  // Used for issuing only one GetHash request for a given prefix.
  GetHashRequests gethash_requests_;

  // The persistent database.  We don't use a std::unique_ptr because it
  // needs to be destroyed on a different thread than this object.
  SafeBrowsingDatabase* database_;

  // Lock used to prevent possible data races due to compiler optimizations.
  mutable base::Lock database_lock_;

  // Indicate if download_protection is enabled by command switch
  // so we allow this feature to be exercised.
  bool enable_download_protection_;

  // Indicate if client-side phishing detection whitelist should be enabled
  // or not.
  bool enable_csd_whitelist_;

  // Indicate if the download whitelist should be enabled or not.
  bool enable_download_whitelist_;

  // Indicate if the extension blacklist should be enabled.
  bool enable_extension_blacklist_;

  // Indicate if the csd malware IP blacklist should be enabled.
  bool enable_ip_blacklist_;

  // Indicate if the unwanted software blacklist should be enabled.
  bool enable_unwanted_software_blacklist_;

  // The sequenced task runner for running safe browsing database operations.
  scoped_refptr<base::SequencedTaskRunner> safe_browsing_task_runner_;

  // Indicates if we're currently in an update cycle.
  bool update_in_progress_;

  // When true, newly fetched chunks may not in the database yet since the
  // database is still updating.
  bool database_update_in_progress_;

  // Indicates if we're in the midst of trying to close the database.  If this
  // is true, nothing on the IO thread should access the database.
  bool closing_database_;

  base::circular_deque<QueuedCheck> queued_checks_;

  // Timeout to use for safe browsing checks.
  base::TimeDelta check_timeout_;

  DISALLOW_COPY_AND_ASSIGN(LocalSafeBrowsingDatabaseManager);
};  // class LocalSafeBrowsingDatabaseManager

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_LOCAL_DATABASE_MANAGER_H_
