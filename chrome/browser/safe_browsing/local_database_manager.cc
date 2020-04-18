// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/local_database_manager.h"

#include <limits>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/debug/leak_tracker.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prerender/prerender_field_trial.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/safe_browsing/client_side_detection_service.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/protocol_manager.h"
#include "chrome/browser/safe_browsing/safe_browsing_database.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/common/safebrowsing_switches.h"
#include "components/safe_browsing/db/util.h"
#include "content/public/browser/browser_thread.h"
#include "url/url_constants.h"

using content::BrowserThread;

namespace safe_browsing {

namespace {

// Timeout for match checks, e.g. download URLs, hashes.
const int kCheckTimeoutMs = 10000;

// Records disposition information about the check.  |hit| should be
// |true| if there were any prefix hits in |full_hashes|.
void RecordGetHashCheckStatus(
    bool hit,
    ListType check_type,
    const std::vector<SBFullHashResult>& full_hashes) {
  SafeBrowsingProtocolManager::ResultType result;
  if (full_hashes.empty()) {
    result = SafeBrowsingProtocolManager::GET_HASH_FULL_HASH_EMPTY;
  } else if (hit) {
    result = SafeBrowsingProtocolManager::GET_HASH_FULL_HASH_HIT;
  } else {
    result = SafeBrowsingProtocolManager::GET_HASH_FULL_HASH_MISS;
  }
  bool is_download = check_type == BINURL;
  SafeBrowsingProtocolManager::RecordGetHashResult(is_download, result);
}

bool IsExpectedThreat(const SBThreatType threat_type,
                      const SBThreatTypeSet& expected_threats) {
  return base::ContainsKey(expected_threats, threat_type);
}

// Returns threat level of the list. Lists with lower threat levels are more
// severe than lists with higher threat levels. Zero is the severest threat
// level possible.
int GetThreatSeverity(ListType threat) {
  switch (threat) {
    case MALWARE:             // Falls through.
    case PHISH:               // Falls through.
    case BINURL:              // Falls through.
    case CSDWHITELIST:        // Falls through.
    case DOWNLOADWHITELIST:   // Falls through.
    case EXTENSIONBLACKLIST:  // Falls through.
    case IPBLACKLIST:
      return 0;
    case UNWANTEDURL:
      // UNWANTEDURL is considered less severe than other threats.
      return 1;
    case RESOURCEBLACKLIST:
      // RESOURCEBLACKLIST is even less severe than UNWANTEDURL.
      return 2;
    case INVALID:
      return std::numeric_limits<int>::max();
  }
  NOTREACHED();
  return -1;
}

// Return the severest list id from the results in |full_hashes| which matches
// |hash|, or INVALID if none match.
ListType GetHashSeverestThreatListType(
    const SBFullHash& hash,
    const std::vector<SBFullHashResult>& full_hashes,
    size_t* index) {
  ListType pending_threat = INVALID;
  int pending_threat_severity = GetThreatSeverity(INVALID);
  for (size_t i = 0; i < full_hashes.size(); ++i) {
    if (SBFullHashEqual(hash, full_hashes[i].hash)) {
      const ListType threat = static_cast<ListType>(full_hashes[i].list_id);
      int threat_severity = GetThreatSeverity(threat);
      if (threat_severity < pending_threat_severity) {
        pending_threat = threat;
        pending_threat_severity = threat_severity;
        if (index)
          *index = i;
      }
      if (pending_threat_severity == 0)
        return pending_threat;
    }
  }
  return pending_threat;
}

// Given a URL, compare all the possible host + path full hashes to the set of
// provided full hashes.  Returns the list id of the severest matching result
// from |full_hashes|, or INVALID if none match.
ListType GetUrlSeverestThreatListType(
    const GURL& url,
    const std::vector<SBFullHashResult>& full_hashes,
    size_t* index) {
  if (full_hashes.empty())
    return INVALID;

  std::vector<std::string> patterns;
  V4ProtocolManagerUtil::GeneratePatternsToCheck(url, &patterns);

  ListType pending_threat = INVALID;
  int pending_threat_severity = GetThreatSeverity(INVALID);
  for (size_t i = 0; i < patterns.size(); ++i) {
    ListType threat = GetHashSeverestThreatListType(
        SBFullHashForString(patterns[i]), full_hashes, index);
    int threat_severity = GetThreatSeverity(threat);
    if (threat_severity < pending_threat_severity) {
      pending_threat = threat;
      pending_threat_severity = threat_severity;
    }
    if (pending_threat_severity == 0)
      return pending_threat;
  }
  return pending_threat;
}

SBThreatType GetThreatTypeFromListType(ListType list_type) {
  switch (list_type) {
    case PHISH:
      return SB_THREAT_TYPE_URL_PHISHING;
    case MALWARE:
      return SB_THREAT_TYPE_URL_MALWARE;
    case UNWANTEDURL:
      return SB_THREAT_TYPE_URL_UNWANTED;
    case BINURL:
      return SB_THREAT_TYPE_URL_BINARY_MALWARE;
    case EXTENSIONBLACKLIST:
      return SB_THREAT_TYPE_EXTENSION;
    case RESOURCEBLACKLIST:
      return SB_THREAT_TYPE_BLACKLISTED_RESOURCE;
    default:
      DVLOG(1) << "Unknown safe browsing list id " << list_type;
      return SB_THREAT_TYPE_SAFE;
  }
}

}  // namespace

// static
SBThreatType LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
    const SBFullHash& hash,
    const std::vector<SBFullHashResult>& full_hashes) {
  return GetThreatTypeFromListType(
      GetHashSeverestThreatListType(hash, full_hashes, NULL));
}

// static
SBThreatType LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
    const GURL& url,
    const std::vector<SBFullHashResult>& full_hashes,
    size_t* index) {
  return GetThreatTypeFromListType(
      GetUrlSeverestThreatListType(url, full_hashes, index));
}

LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck::SafeBrowsingCheck(
    const std::vector<GURL>& urls,
    const std::vector<SBFullHash>& full_hashes,
    Client* client,
    ListType check_type,
    const SBThreatTypeSet& expected_threats)
    : urls(urls),
      url_results(urls.size(), SB_THREAT_TYPE_SAFE),
      url_metadata(urls.size()),
      url_hit_hash(urls.size()),
      full_hashes(full_hashes),
      full_hash_results(full_hashes.size(), SB_THREAT_TYPE_SAFE),
      client(client),
      need_get_hash(false),
      check_type(check_type),
      expected_threats(expected_threats) {
  DCHECK_EQ(urls.empty(), !full_hashes.empty())
      << "Exactly one of urls and full_hashes must be set";
}

LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck::~SafeBrowsingCheck() {}

void LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck::
    OnSafeBrowsingResult() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(client);
  DCHECK_EQ(urls.size(), url_results.size());
  DCHECK_EQ(full_hashes.size(), full_hash_results.size());
  if (!urls.empty()) {
    DCHECK(full_hashes.empty());
    switch (check_type) {
      case MALWARE:
      case PHISH:
      case UNWANTEDURL:
        DCHECK_EQ(1u, urls.size());
        client->OnCheckBrowseUrlResult(urls[0], url_results[0],
                                       url_metadata[0]);
        break;
      case BINURL:
        DCHECK_EQ(urls.size(), url_results.size());
        client->OnCheckDownloadUrlResult(
            urls, *std::max_element(url_results.begin(), url_results.end()));
        break;
      case RESOURCEBLACKLIST:
        DCHECK_EQ(1u, urls.size());
        client->OnCheckResourceUrlResult(urls[0], url_results[0],
                                         url_hit_hash[0]);
        break;
      default:
        NOTREACHED();
    }
  } else if (!full_hashes.empty()) {
    switch (check_type) {
      case EXTENSIONBLACKLIST: {
        std::set<std::string> unsafe_extension_ids;
        for (size_t i = 0; i < full_hashes.size(); ++i) {
          std::string extension_id = SBFullHashToString(full_hashes[i]);
          if (full_hash_results[i] == SB_THREAT_TYPE_EXTENSION)
            unsafe_extension_ids.insert(extension_id);
        }
        client->OnCheckExtensionsResult(unsafe_extension_ids);
        break;
      }
      default:
        NOTREACHED();
    }
  } else {
    NOTREACHED();
  }
}

LocalSafeBrowsingDatabaseManager::LocalSafeBrowsingDatabaseManager(
    const scoped_refptr<SafeBrowsingService>& service)
    : sb_service_(service),
      database_(NULL),
      enable_download_protection_(false),
      enable_csd_whitelist_(false),
      enable_download_whitelist_(false),
      enable_extension_blacklist_(false),
      enable_ip_blacklist_(false),
      enable_unwanted_software_blacklist_(true),
      update_in_progress_(false),
      database_update_in_progress_(false),
      closing_database_(false),
      check_timeout_(base::TimeDelta::FromMilliseconds(kCheckTimeoutMs)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(sb_service_.get() != NULL);

  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  enable_download_protection_ = !cmdline->HasSwitch(
      safe_browsing::switches::kSbDisableDownloadProtection);

  // We only download the csd-whitelist if client-side phishing detection is
  // enabled.
  enable_csd_whitelist_ =
      !cmdline->HasSwitch(::switches::kDisableClientSidePhishingDetection);

  // We download the download-whitelist if download protection is enabled.
  enable_download_whitelist_ = enable_download_protection_;

  // TODO(kalman): there really shouldn't be a flag for this.
  enable_extension_blacklist_ = !cmdline->HasSwitch(
      safe_browsing::switches::kSbDisableExtensionBlacklist);

  // The client-side IP blacklist feature is tightly integrated with client-side
  // phishing protection for now.
  enable_ip_blacklist_ = enable_csd_whitelist_;
}

LocalSafeBrowsingDatabaseManager::~LocalSafeBrowsingDatabaseManager() {
  // The DCHECK is disabled due to crbug.com/438754.
  // DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // We should have already been shut down. If we're still enabled, then the
  // database isn't going to be closed properly, which could lead to corruption.
  DCHECK(!enabled_);
}

bool LocalSafeBrowsingDatabaseManager::IsSupported() const {
  return true;
}

safe_browsing::ThreatSource LocalSafeBrowsingDatabaseManager::GetThreatSource()
    const {
  return safe_browsing::ThreatSource::LOCAL_PVER3;
}

bool LocalSafeBrowsingDatabaseManager::ChecksAreAlwaysAsync() const {
  return false;
}

bool LocalSafeBrowsingDatabaseManager::CanCheckResourceType(
    content::ResourceType resource_type) const {
  // We check all types since most checks are fast.
  return true;
}

bool LocalSafeBrowsingDatabaseManager::CanCheckSubresourceFilter() const {
  return false;
}

bool LocalSafeBrowsingDatabaseManager::CanCheckUrl(const GURL& url) const {
  return url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(url::kFtpScheme) ||
         url.SchemeIsWSOrWSS();
}

bool LocalSafeBrowsingDatabaseManager::CheckDownloadUrl(
    const std::vector<GURL>& url_chain,
    Client* client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!enabled_ || !enable_download_protection_)
    return true;

  // We need to check the database for url prefix, and later may fetch the url
  // from the safebrowsing backends. These need to be asynchronous.
  std::unique_ptr<SafeBrowsingCheck> check =
      std::make_unique<SafeBrowsingCheck>(
          url_chain, std::vector<SBFullHash>(), client, BINURL,
          CreateSBThreatTypeSet({SB_THREAT_TYPE_URL_BINARY_MALWARE}));
  std::vector<SBPrefix> prefixes;
  SafeBrowsingDatabase::GetDownloadUrlPrefixes(url_chain, &prefixes);
  StartSafeBrowsingCheck(
      std::move(check),
      base::Bind(&LocalSafeBrowsingDatabaseManager::CheckDownloadUrlOnSBThread,
                 this, prefixes));
  return false;
}

bool LocalSafeBrowsingDatabaseManager::CheckExtensionIDs(
    const std::set<std::string>& extension_ids,
    Client* client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!enabled_ || !enable_extension_blacklist_)
    return true;

  std::vector<SBFullHash> extension_id_hashes;
  std::transform(extension_ids.begin(), extension_ids.end(),
                 std::back_inserter(extension_id_hashes), StringToSBFullHash);
  std::vector<SBPrefix> prefixes;
  for (const SBFullHash& hash : extension_id_hashes)
    prefixes.push_back(hash.prefix);

  std::unique_ptr<SafeBrowsingCheck> check =
      std::make_unique<SafeBrowsingCheck>(
          std::vector<GURL>(), extension_id_hashes, client, EXTENSIONBLACKLIST,
          CreateSBThreatTypeSet({SB_THREAT_TYPE_EXTENSION}));
  StartSafeBrowsingCheck(
      std::move(check),
      base::Bind(&LocalSafeBrowsingDatabaseManager::CheckExtensionIDsOnSBThread,
                 this, prefixes));
  return false;
}

bool LocalSafeBrowsingDatabaseManager::CheckResourceUrl(const GURL& url,
                                                        Client* client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!enabled_ || !CanCheckUrl(url))
    return true;

  SBThreatTypeSet expected_threats =
      CreateSBThreatTypeSet({SB_THREAT_TYPE_BLACKLISTED_RESOURCE});

  if (!MakeDatabaseAvailable()) {
    QueuedCheck queued_check(RESOURCEBLACKLIST, client, url, expected_threats,
                             base::TimeTicks::Now());
    queued_checks_.push_back(queued_check);
    return false;
  }

  std::unique_ptr<SafeBrowsingCheck> check = base::WrapUnique(
      new SafeBrowsingCheck({url}, std::vector<SBFullHash>(), client,
                            RESOURCEBLACKLIST, expected_threats));

  std::vector<SBPrefix> prefixes;
  SafeBrowsingDatabase::GetDownloadUrlPrefixes(check->urls, &prefixes);
  StartSafeBrowsingCheck(
      std::move(check),
      base::Bind(&LocalSafeBrowsingDatabaseManager::CheckResourceUrlOnSBThread,
                 this, prefixes));
  return false;
}

bool LocalSafeBrowsingDatabaseManager::MatchMalwareIP(
    const std::string& ip_address) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!enabled_ || !enable_ip_blacklist_ || !MakeDatabaseAvailable()) {
    return false;  // Fail open.
  }
  return database_->ContainsMalwareIP(ip_address);
}

AsyncMatch LocalSafeBrowsingDatabaseManager::CheckCsdWhitelistUrl(
    const GURL& url,
    Client* Client) {
  // Pver3 DB does not support actual partial-hash whitelists, so we emulate
  // it.  All this code will go away soon (~M62).
  return (MatchCsdWhitelistUrl(url) ? AsyncMatch::MATCH : AsyncMatch::NO_MATCH);
}

bool LocalSafeBrowsingDatabaseManager::MatchCsdWhitelistUrl(const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!enabled_ || !enable_csd_whitelist_ || !MakeDatabaseAvailable()) {
    // There is something funky going on here -- for example, perhaps the user
    // has not restarted since enabling metrics reporting, so we haven't
    // enabled the csd whitelist yet.  Just to be safe we return true in this
    // case.
    return true;
  }
  return database_->ContainsCsdWhitelistedUrl(url);
}

bool LocalSafeBrowsingDatabaseManager::MatchDownloadWhitelistUrl(
    const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!enabled_ || !enable_download_whitelist_ || !MakeDatabaseAvailable()) {
    return true;
  }
  return database_->ContainsDownloadWhitelistedUrl(url);
}

bool LocalSafeBrowsingDatabaseManager::MatchDownloadWhitelistString(
    const std::string& str) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!enabled_ || !enable_download_whitelist_ || !MakeDatabaseAvailable()) {
    return true;
  }
  return database_->ContainsDownloadWhitelistedString(str);
}

bool LocalSafeBrowsingDatabaseManager::CheckBrowseUrl(
    const GURL& url,
    const SBThreatTypeSet& expected_threats,
    Client* client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!expected_threats.empty());
  DCHECK(SBThreatTypeSetIsValidForCheckBrowseUrl(expected_threats));
  if (!enabled_)
    return true;

  if (!CanCheckUrl(url))
    return true;

  const base::TimeTicks start = base::TimeTicks::Now();
  if (!MakeDatabaseAvailable()) {
    QueuedCheck queued_check(MALWARE,  // or PHISH
                             client, url, expected_threats, start);
    queued_checks_.push_back(queued_check);
    return false;
  }

  std::vector<SBFullHash> full_hashes;
  UrlToFullHashes(url, false, &full_hashes);

  // Cache hits should, in general, be the same for both (ignoring potential
  // cache evictions in the second call for entries that were just about to be
  // evicted in the first call).
  // TODO(gab): Refactor SafeBrowsingDatabase to avoid depending on this here.
  std::vector<SBFullHashResult> cache_hits;
  std::vector<SBPrefix> browse_prefix_hits;
  database_->ContainsBrowseHashes(full_hashes, &browse_prefix_hits,
                                  &cache_hits);

  std::vector<SBPrefix> unwanted_prefix_hits;
  std::vector<SBFullHashResult> unused_cache_hits;
  database_->ContainsUnwantedSoftwareHashes(full_hashes, &unwanted_prefix_hits,
                                            &unused_cache_hits);

  // Merge the two pre-sorted prefix hits lists.
  // TODO(gab): Refactor SafeBrowsingDatabase for it to return this merged list
  // by default rather than building it here.
  std::vector<SBPrefix> prefix_hits(browse_prefix_hits.size() +
                                    unwanted_prefix_hits.size());
  std::merge(browse_prefix_hits.begin(), browse_prefix_hits.end(),
             unwanted_prefix_hits.begin(), unwanted_prefix_hits.end(),
             prefix_hits.begin());
  prefix_hits.erase(std::unique(prefix_hits.begin(), prefix_hits.end()),
                    prefix_hits.end());

  if (prefix_hits.empty() && cache_hits.empty())
    return true;  // URL is okay.

  // Needs to be asynchronous, since we could be in the constructor of a
  // ResourceDispatcherHost event handler which can't pause there.
  // This check will ping the Safe Browsing servers and get all lists which it
  // matches. These lists will then be filtered against the |expected_threats|
  // and the result callback for MALWARE (which is the same as for PHISH and
  // UNWANTEDURL) will eventually be invoked with the final decision.
  SafeBrowsingCheck* check = new SafeBrowsingCheck(
      std::vector<GURL>(1, url), std::vector<SBFullHash>(), client, MALWARE,
      expected_threats);
  check->need_get_hash = cache_hits.empty();
  check->prefix_hits.swap(prefix_hits);
  check->cache_hits.swap(cache_hits);
  checks_[check] = base::WrapUnique(check);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::OnCheckDone, this,
                     check));

  return false;
}

bool LocalSafeBrowsingDatabaseManager::CheckUrlForSubresourceFilter(
    const GURL& url,
    Client* client) {
  // The check for the Subresource Filter in only implemented for pver4.
  NOTREACHED();
  return true;
}

void LocalSafeBrowsingDatabaseManager::CancelCheck(Client* client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (const auto& check : checks_) {
    // We can't delete matching checks here because the db thread has a copy of
    // the pointer.  Instead, we simply NULL out the client, and when the db
    // thread calls us back, we'll clean up the check.
    if (check.first->client == client)
      check.first->client = NULL;
  }

  // Scan the queued clients store. Clients may be here if they requested a URL
  // check before the database has finished loading.
  for (auto it = queued_checks_.begin(); it != queued_checks_.end();) {
    // In this case it's safe to delete matches entirely since nothing has a
    // pointer to them.
    if (it->client == client)
      it = queued_checks_.erase(it);
    else
      ++it;
  }
}

void LocalSafeBrowsingDatabaseManager::HandleGetHashResults(
    SafeBrowsingCheck* check,
    const std::vector<SBFullHashResult>& full_hashes,
    const base::TimeDelta& cache_lifetime) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!enabled_)
    return;

  // If the service has been shut down, |check| should have been deleted.
  DCHECK(checks_.find(check) != checks_.end());

  // |start| is set before calling |GetFullHash()|, which should be
  // the only path which gets to here.
  DCHECK(!check->start.is_null());
  UMA_HISTOGRAM_LONG_TIMES("SB2.Network",
                           base::TimeTicks::Now() - check->start);

  std::vector<SBPrefix> prefixes = check->prefix_hits;
  OnHandleGetHashResults(check, full_hashes);  // 'check' is deleted here.

  // Cache the GetHash results.
  if (!cache_lifetime.is_zero() && MakeDatabaseAvailable())
    database_->CacheHashResults(prefixes, full_hashes, cache_lifetime);
}

void LocalSafeBrowsingDatabaseManager::GetChunks(GetChunksCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  DCHECK(!callback.is_null());
  safe_browsing_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &LocalSafeBrowsingDatabaseManager::GetAllChunksFromDatabase, this,
          callback));
}

void LocalSafeBrowsingDatabaseManager::AddChunks(
    const std::string& list,
    std::unique_ptr<std::vector<std::unique_ptr<SBChunkData>>> chunks,
    AddChunksCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  DCHECK(!callback.is_null());
  safe_browsing_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::AddDatabaseChunks, this,
                     list, std::move(chunks), callback));
}

void LocalSafeBrowsingDatabaseManager::DeleteChunks(
    std::unique_ptr<std::vector<SBChunkDelete>> chunk_deletes) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  safe_browsing_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::DeleteDatabaseChunks,
                     this, std::move(chunk_deletes)));
}

void LocalSafeBrowsingDatabaseManager::UpdateStarted() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  DCHECK(!update_in_progress_);
  update_in_progress_ = true;
}

void LocalSafeBrowsingDatabaseManager::UpdateFinished(bool update_succeeded) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  if (update_in_progress_) {
    update_in_progress_ = false;
    safe_browsing_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &LocalSafeBrowsingDatabaseManager::DatabaseUpdateFinished, this,
            update_succeeded));
  }
}

void LocalSafeBrowsingDatabaseManager::ResetDatabase() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  safe_browsing_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::OnResetDatabase, this));
}

void LocalSafeBrowsingDatabaseManager::StartOnIOThread(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const V4ProtocolConfig& config) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  SafeBrowsingDatabaseManager::StartOnIOThread(url_loader_factory, config);

  if (enabled_)
    return;

  // Only get a new task runner if there isn't one already. If the service has
  // previously been started and stopped, a task runner could already exist.
  if (!safe_browsing_task_runner_) {
    safe_browsing_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BACKGROUND,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }

  enabled_ = true;

  MakeDatabaseAvailable();
}

void LocalSafeBrowsingDatabaseManager::StopOnIOThread(bool shutdown) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DoStopOnIOThread();
  if (shutdown) {
    sb_service_ = NULL;
  }

  SafeBrowsingDatabaseManager::StopOnIOThread(shutdown);
}

void LocalSafeBrowsingDatabaseManager::NotifyDatabaseUpdateFinished(
    bool update_succeeded) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  update_complete_callback_list_.Notify();
}

LocalSafeBrowsingDatabaseManager::QueuedCheck::QueuedCheck(
    const ListType check_type,
    Client* client,
    const GURL& url,
    const SBThreatTypeSet& expected_threats,
    const base::TimeTicks& start)
    : check_type(check_type),
      client(client),
      url(url),
      expected_threats(expected_threats),
      start(start) {}

LocalSafeBrowsingDatabaseManager::QueuedCheck::QueuedCheck(
    const QueuedCheck& other) = default;

LocalSafeBrowsingDatabaseManager::QueuedCheck::~QueuedCheck() {}

void LocalSafeBrowsingDatabaseManager::DoStopOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!enabled_)
    return;

  enabled_ = false;

  // Delete queued checks, calling back any clients with 'SB_THREAT_TYPE_SAFE'.
  while (!queued_checks_.empty()) {
    QueuedCheck queued = queued_checks_.front();
    if (queued.client) {
      SafeBrowsingCheck sb_check(std::vector<GURL>(1, queued.url),
                                 std::vector<SBFullHash>(), queued.client,
                                 queued.check_type, queued.expected_threats);
      sb_check.OnSafeBrowsingResult();
    }
    queued_checks_.pop_front();
  }

  // Close the database.  Cases to avoid:
  //  * If |closing_database_| is true, continuing will queue up a second
  //    request, |closing_database_| will be reset after handling the first
  //    request, and if any functions on the db thread recreate the database, we
  //    could start using it on the IO thread and then have the second request
  //    handler delete it out from under us.
  //  * If |database_| is NULL, then either no creation request is in flight, in
  //    which case we don't need to do anything, or one is in flight, in which
  //    case the database will be recreated before our deletion request is
  //    handled, and could be used on the IO thread in that time period, leading
  //    to the same problem as above.
  // Checking DatabaseAvailable() avoids both of these.
  if (DatabaseAvailable()) {
    closing_database_ = true;
    safe_browsing_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&LocalSafeBrowsingDatabaseManager::OnCloseDatabase,
                       this));
  }

  // Delete pending checks, calling back any clients with 'SB_THREAT_TYPE_SAFE'.
  // We have to do this after the db thread returns because methods on it can
  // have copies of these pointers, so deleting them might lead to accessing
  // garbage.
  for (const auto& check : checks_) {
    if (check.first->client)
      check.first->OnSafeBrowsingResult();
  }

  checks_.clear();
  gethash_requests_.clear();
}

bool LocalSafeBrowsingDatabaseManager::DatabaseAvailable() const {
  base::AutoLock lock(database_lock_);
  return !closing_database_ && (database_ != NULL);
}

bool LocalSafeBrowsingDatabaseManager::MakeDatabaseAvailable() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enabled_);
  if (DatabaseAvailable())
    return true;
  safe_browsing_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          base::IgnoreResult(&LocalSafeBrowsingDatabaseManager::GetDatabase),
          this));
  return false;
}

SafeBrowsingDatabase* LocalSafeBrowsingDatabaseManager::GetDatabase() {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());

  if (database_)
    return database_;

  const base::TimeTicks before = base::TimeTicks::Now();
  std::unique_ptr<SafeBrowsingDatabase> database = SafeBrowsingDatabase::Create(
      safe_browsing_task_runner_, enable_download_protection_,
      enable_csd_whitelist_, enable_download_whitelist_,
      enable_extension_blacklist_, enable_ip_blacklist_,
      enable_unwanted_software_blacklist_);

  database->Init(SafeBrowsingService::GetBaseFilename());
  {
    // Acquiring the lock here guarantees correct ordering between the writes to
    // the new database object above, and the setting of |database_| below.
    base::AutoLock lock(database_lock_);
    database_ = database.release();
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::DatabaseLoadComplete,
                     this));

  UMA_HISTOGRAM_TIMES("SB2.DatabaseOpen", base::TimeTicks::Now() - before);
  return database_;
}

void LocalSafeBrowsingDatabaseManager::OnCheckDone(SafeBrowsingCheck* check) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!enabled_)
    return;

  // If the service has been shut down, |check| should have been deleted.
  DCHECK(checks_.find(check) != checks_.end());

  if (check->client && check->need_get_hash) {
    // We have a partial match so we need to query Google for the full hash.
    // Clean up will happen in HandleGetHashResults.

    // See if we have a GetHash request already in progress for this particular
    // prefix. If so, we just append ourselves to the list of interested parties
    // when the results arrive. We only do this for checks involving one prefix,
    // since that is the common case (multiple prefixes will issue the request
    // as normal).
    if (check->prefix_hits.size() == 1) {
      SBPrefix prefix = check->prefix_hits[0];
      GetHashRequests::iterator it = gethash_requests_.find(prefix);
      if (it != gethash_requests_.end()) {
        // There's already a request in progress.
        it->second.push_back(check);
        return;
      }

      // No request in progress, so we're the first for this prefix.
      GetHashRequestors requestors;
      requestors.push_back(check);
      gethash_requests_[prefix] = requestors;
    }

    // Reset the start time so that we can measure the network time without the
    // database time.
    check->start = base::TimeTicks::Now();

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&LocalSafeBrowsingDatabaseManager::OnRequestFullHash,
                       this, check));
  } else {
    // We may have cached results for previous GetHash queries.  Since
    // this data comes from cache, don't histogram hits.
    HandleOneCheck(check, check->cache_hits);
  }
}

void LocalSafeBrowsingDatabaseManager::OnRequestFullHash(
    SafeBrowsingCheck* check) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  check->extended_reporting_level = GetExtendedReporting();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::RequestFullHash, this,
                     check));
}

ExtendedReportingLevel
LocalSafeBrowsingDatabaseManager::GetExtendedReporting() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Determine if the last used profile is opted into extended reporting.
  // Note: It is possible that the last used profile is not the one triggers
  // the hash request, but not very likely.
  ExtendedReportingLevel extended_reporting_level = SBER_LEVEL_OFF;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (profile_manager) {
    Profile* profile = profile_manager->GetLastUsedProfile();
    extended_reporting_level =
        profile ? GetExtendedReportingLevel(*profile->GetPrefs())
                : SBER_LEVEL_OFF;
  }
  return extended_reporting_level;
}

void LocalSafeBrowsingDatabaseManager::RequestFullHash(
    SafeBrowsingCheck* check) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!enabled_)
    return;

  bool is_download = check->check_type == BINURL;
  sb_service_->protocol_manager()->GetFullHash(
      check->prefix_hits,
      base::Bind(&LocalSafeBrowsingDatabaseManager::HandleGetHashResults,
                 base::Unretained(this), check),
      is_download, check->extended_reporting_level);
}

void LocalSafeBrowsingDatabaseManager::GetAllChunksFromDatabase(
    GetChunksCallback callback) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());

  bool database_error = true;
  std::vector<SBListChunkRanges> lists;
  DCHECK(!database_update_in_progress_);
  database_update_in_progress_ = true;
  GetDatabase();  // This guarantees that |database_| is non-NULL.
  if (database_->UpdateStarted(&lists)) {
    database_error = false;
  } else {
    database_->UpdateFinished(false);
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &LocalSafeBrowsingDatabaseManager::BeforeGetAllChunksFromDatabase,
          this, lists, database_error, callback));
}

void LocalSafeBrowsingDatabaseManager::BeforeGetAllChunksFromDatabase(
    const std::vector<SBListChunkRanges>& lists,
    bool database_error,
    GetChunksCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ExtendedReportingLevel extended_reporting_level = GetExtendedReporting();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &LocalSafeBrowsingDatabaseManager::OnGetAllChunksFromDatabase, this,
          lists, database_error, extended_reporting_level, callback));
}

void LocalSafeBrowsingDatabaseManager::OnGetAllChunksFromDatabase(
    const std::vector<SBListChunkRanges>& lists,
    bool database_error,
    ExtendedReportingLevel reporting_level,
    GetChunksCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (enabled_)
    callback.Run(lists, database_error, reporting_level);
}

void LocalSafeBrowsingDatabaseManager::OnAddChunksComplete(
    AddChunksCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (enabled_)
    callback.Run();
}

void LocalSafeBrowsingDatabaseManager::DatabaseLoadComplete() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!enabled_)
    return;

  LOCAL_HISTOGRAM_COUNTS("SB.QueueDepth", queued_checks_.size());
  if (queued_checks_.empty())
    return;

  // If the database isn't already available, calling CheckUrl() in the loop
  // below will add the check back to the queue, and we'll infinite-loop.
  DCHECK(DatabaseAvailable());
  while (!queued_checks_.empty()) {
    QueuedCheck check = queued_checks_.front();
    DCHECK(!check.start.is_null());
    LOCAL_HISTOGRAM_TIMES("SB.QueueDelay",
                          base::TimeTicks::Now() - check.start);
    // If CheckUrl() determines the URL is safe immediately, it doesn't call the
    // client's handler function (because normally it's being directly called by
    // the client).  Since we're not the client, we have to convey this result.
    if (check.client &&
        CheckBrowseUrl(check.url, check.expected_threats, check.client)) {
      SafeBrowsingCheck sb_check(std::vector<GURL>(1, check.url),
                                 std::vector<SBFullHash>(), check.client,
                                 check.check_type, check.expected_threats);
      sb_check.OnSafeBrowsingResult();
    }
    queued_checks_.pop_front();
  }
}

void LocalSafeBrowsingDatabaseManager::AddDatabaseChunks(
    const std::string& list_name,
    std::unique_ptr<std::vector<std::unique_ptr<SBChunkData>>> chunks,
    AddChunksCallback callback) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());
  if (chunks)
    GetDatabase()->InsertChunks(list_name, *chunks);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::OnAddChunksComplete,
                     this, callback));
}

void LocalSafeBrowsingDatabaseManager::DeleteDatabaseChunks(
    std::unique_ptr<std::vector<SBChunkDelete>> chunk_deletes) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());
  if (chunk_deletes)
    GetDatabase()->DeleteChunks(*chunk_deletes);
}

void LocalSafeBrowsingDatabaseManager::DatabaseUpdateFinished(
    bool update_succeeded) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());
  GetDatabase()->UpdateFinished(update_succeeded);
  DCHECK(database_update_in_progress_);
  database_update_in_progress_ = false;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &LocalSafeBrowsingDatabaseManager::NotifyDatabaseUpdateFinished, this,
          update_succeeded));
}

void LocalSafeBrowsingDatabaseManager::OnCloseDatabase() {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(closing_database_);

  // Because |closing_database_| is true, nothing on the IO thread will be
  // accessing the database, so it's safe to delete and then NULL the pointer.
  delete database_;
  database_ = NULL;

  // Acquiring the lock here guarantees correct ordering between the resetting
  // of |database_| above and of |closing_database_| below, which ensures there
  // won't be a window during which the IO thread falsely believes the database
  // is available.
  base::AutoLock lock(database_lock_);
  closing_database_ = false;
}

void LocalSafeBrowsingDatabaseManager::OnResetDatabase() {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());

  GetDatabase()->ResetDatabase();
}

void LocalSafeBrowsingDatabaseManager::OnHandleGetHashResults(
    SafeBrowsingCheck* check,
    const std::vector<SBFullHashResult>& full_hashes) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ListType check_type = check->check_type;
  SBPrefix prefix = check->prefix_hits[0];
  GetHashRequests::iterator it = gethash_requests_.find(prefix);
  if (check->prefix_hits.size() > 1 || it == gethash_requests_.end()) {
    const bool hit = HandleOneCheck(check, full_hashes);
    RecordGetHashCheckStatus(hit, check_type, full_hashes);
    return;
  }

  // Call back all interested parties, noting if any has a hit.
  GetHashRequestors& requestors = it->second;
  bool hit = false;
  for (GetHashRequestors::iterator r = requestors.begin();
       r != requestors.end(); ++r) {
    if (HandleOneCheck(*r, full_hashes))
      hit = true;
  }
  RecordGetHashCheckStatus(hit, check_type, full_hashes);

  gethash_requests_.erase(it);
}

bool LocalSafeBrowsingDatabaseManager::HandleOneCheck(
    SafeBrowsingCheck* check,
    const std::vector<SBFullHashResult>& full_hashes) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(check);

  bool is_threat = false;

  // TODO(shess): GetHashSeverestThreadListType() contains a loop,
  // GetUrlSeverestThreatListType() a loop around that loop.  Having another
  // loop out here concerns me.  It is likely that SAFE is an expected outcome,
  // which means all of those loops run to completion.  Refactoring this to
  // generate a set of sorted items to compare in sequence would probably
  // improve things.
  //
  // Additionally, the set of patterns generated from the urls is very similar
  // to the patterns generated in ContainsBrowseUrl() and other database checks,
  // which are called from this code.  Refactoring that across the checks could
  // interact well with batching the checks here.

  std::vector<SBFullHashResult> expected_full_hashes;
  for (const auto& full_hash : full_hashes) {
    ListType type = static_cast<ListType>(full_hash.list_id);
    if (IsExpectedThreat(GetThreatTypeFromListType(type),
                         check->expected_threats)) {
      expected_full_hashes.push_back(full_hash);
    }
  }

  if (expected_full_hashes.empty()) {
    SafeBrowsingCheckDone(check);
    return false;
  }

  for (size_t i = 0; i < check->urls.size(); ++i) {
    size_t threat_index;
    SBThreatType threat = GetUrlSeverestThreatType(
        check->urls[i], expected_full_hashes, &threat_index);
    if (threat != SB_THREAT_TYPE_SAFE) {
      check->url_results[i] = threat;
      check->url_metadata[i] = expected_full_hashes[threat_index].metadata;
      const SBFullHash& hash = expected_full_hashes[threat_index].hash;
      check->url_hit_hash[i] =
          std::string(hash.full_hash, arraysize(hash.full_hash));
      is_threat = true;
    }
  }

  for (size_t i = 0; i < check->full_hashes.size(); ++i) {
    SBThreatType threat =
        GetHashSeverestThreatType(check->full_hashes[i], expected_full_hashes);
    if (threat != SB_THREAT_TYPE_SAFE) {
      check->full_hash_results[i] = threat;
      is_threat = true;
    }
  }

  SafeBrowsingCheckDone(check);
  return is_threat;
}

void LocalSafeBrowsingDatabaseManager::OnAsyncCheckDone(
    SafeBrowsingCheck* check,
    const std::vector<SBPrefix>& prefix_hits) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(enable_download_protection_);

  check->prefix_hits = prefix_hits;
  if (check->prefix_hits.empty()) {
    SafeBrowsingCheckDone(check);
  } else {
    check->need_get_hash = true;
    OnCheckDone(check);
  }
}

std::vector<SBPrefix>
LocalSafeBrowsingDatabaseManager::CheckDownloadUrlOnSBThread(
    const std::vector<SBPrefix>& prefixes) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(enable_download_protection_);

  std::vector<SBPrefix> prefix_hits;
  const bool result =
      database_->ContainsDownloadUrlPrefixes(prefixes, &prefix_hits);
  DCHECK_EQ(result, !prefix_hits.empty());
  return prefix_hits;
}

std::vector<SBPrefix>
LocalSafeBrowsingDatabaseManager::CheckExtensionIDsOnSBThread(
    const std::vector<SBPrefix>& prefixes) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());

  std::vector<SBPrefix> prefix_hits;
  const bool result =
      database_->ContainsExtensionPrefixes(prefixes, &prefix_hits);
  DCHECK_EQ(result, !prefix_hits.empty());
  return prefix_hits;
}

std::vector<SBPrefix>
LocalSafeBrowsingDatabaseManager::CheckResourceUrlOnSBThread(
    const std::vector<SBPrefix>& prefixes) {
  DCHECK(safe_browsing_task_runner_->RunsTasksInCurrentSequence());

  std::vector<SBPrefix> prefix_hits;
  const bool result =
      database_->ContainsResourceUrlPrefixes(prefixes, &prefix_hits);
  DCHECK_EQ(result, !prefix_hits.empty());
  return prefix_hits;
}

void LocalSafeBrowsingDatabaseManager::TimeoutCallback(
    SafeBrowsingCheck* check) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(check);

  if (!enabled_)
    return;

  DCHECK(checks_.find(check) != checks_.end());
  if (check->client) {
    check->OnSafeBrowsingResult();
    check->client = NULL;
  }
}

void LocalSafeBrowsingDatabaseManager::SafeBrowsingCheckDone(
    SafeBrowsingCheck* check) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(check);

  if (!enabled_)
    return;

  DCHECK(checks_.find(check) != checks_.end());
  if (check->client)
    check->OnSafeBrowsingResult();
  checks_.erase(check);
}

void LocalSafeBrowsingDatabaseManager::StartSafeBrowsingCheck(
    std::unique_ptr<SafeBrowsingCheck> check,
    const base::Callback<std::vector<SBPrefix>(void)>& task) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  check->weak_ptr_factory_.reset(
      new base::WeakPtrFactory<LocalSafeBrowsingDatabaseManager>(this));
  SafeBrowsingCheck* check_ptr = check.get();
  checks_[check_ptr] = std::move(check);

  base::PostTaskAndReplyWithResult(
      safe_browsing_task_runner_.get(), FROM_HERE, task,
      base::Bind(&LocalSafeBrowsingDatabaseManager::OnAsyncCheckDone,
                 check_ptr->weak_ptr_factory_->GetWeakPtr(), check_ptr));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&LocalSafeBrowsingDatabaseManager::TimeoutCallback,
                     check_ptr->weak_ptr_factory_->GetWeakPtr(), check_ptr),
      check_timeout_);
}

bool LocalSafeBrowsingDatabaseManager::IsDownloadProtectionEnabled() const {
  return enable_download_protection_;
}

}  // namespace safe_browsing
