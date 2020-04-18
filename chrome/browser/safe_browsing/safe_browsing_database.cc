// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_database.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <iterator>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics_iocounters.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/safe_browsing/safe_browsing_store_file.h"
#include "components/safe_browsing/db/prefix_set.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/sha2.h"
#include "net/base/ip_address.h"
#include "url/gurl.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif

using content::BrowserThread;

namespace safe_browsing {

namespace {

// Filename suffix for the bloom filter.
const base::FilePath::CharType kBloomFilterFileSuffix[] =
    FILE_PATH_LITERAL(" Filter 2");
// Filename suffix for the prefix set.
const base::FilePath::CharType kPrefixSetFileSuffix[] =
    FILE_PATH_LITERAL(" Prefix Set");
// Filename suffix for download store.
const base::FilePath::CharType kDownloadDBFile[] =
    FILE_PATH_LITERAL(" Download");
// Filename suffix for client-side phishing detection whitelist store.
const base::FilePath::CharType kCsdWhitelistDBFile[] =
    FILE_PATH_LITERAL(" Csd Whitelist");
// Filename suffix for the download whitelist store.
const base::FilePath::CharType kDownloadWhitelistDBFile[] =
    FILE_PATH_LITERAL(" Download Whitelist");
// Filename suffix for the extension blacklist store.
const base::FilePath::CharType kExtensionBlacklistDBFile[] =
    FILE_PATH_LITERAL(" Extension Blacklist");
// Filename suffix for the csd malware IP blacklist store.
const base::FilePath::CharType kIPBlacklistDBFile[] =
    FILE_PATH_LITERAL(" IP Blacklist");
// Filename suffix for the unwanted software blacklist store.
const base::FilePath::CharType kUnwantedSoftwareDBFile[] =
    FILE_PATH_LITERAL(" UwS List");
// Filename suffix for the resource blacklist store.
const base::FilePath::CharType kResourceBlacklistDBFile[] =
    FILE_PATH_LITERAL(" Resource Blacklist");

// Filename suffix for browse store.
// TODO(shess): "Safe Browsing Bloom Prefix Set" is full of win.
// Unfortunately, to change the name implies lots of transition code
// for little benefit.  If/when file formats change (say to put all
// the data in one file), that would be a convenient point to rectify
// this.
const base::FilePath::CharType kBrowseDBFile[] = FILE_PATH_LITERAL(" Bloom");

// Maximum number of entries we allow in any of the whitelists.
// If a whitelist on disk contains more entries then all lookups to
// the whitelist will be considered a match.
const size_t kMaxWhitelistSize = 5000;

const size_t kMaxIpPrefixSize = 128;
const size_t kMinIpPrefixSize = 1;

// To save space, the incoming |chunk_id| and |list_id| are combined
// into an |encoded_chunk_id| for storage by shifting the |list_id|
// into the low-order bits.  These functions decode that information.
// TODO(lzheng): It was reasonable when database is saved in sqlite, but
// there should be better ways to save chunk_id and list_id after we use
// SafeBrowsingStoreFile.
int GetListIdBit(const int encoded_chunk_id) {
  return encoded_chunk_id & 1;
}
int DecodeChunkId(int encoded_chunk_id) {
  return encoded_chunk_id >> 1;
}
int EncodeChunkId(const int chunk, const int list_id) {
  DCHECK_NE(list_id, INVALID);
  return chunk << 1 | list_id % 2;
}

// Helper function to compare addprefixes in |store| with |prefixes|.
// The |list_bit| indicates which list (url or hash) to compare.
//
// Returns true if there is a match, |*prefix_hits| (if non-NULL) will contain
// the actual matching prefixes.
bool MatchAddPrefixes(SafeBrowsingStore* store,
                      int list_bit,
                      const std::vector<SBPrefix>& prefixes,
                      std::vector<SBPrefix>* prefix_hits) {
  prefix_hits->clear();
  bool found_match = false;

  SBAddPrefixes add_prefixes;
  store->GetAddPrefixes(&add_prefixes);
  for (SBAddPrefixes::const_iterator iter = add_prefixes.begin();
       iter != add_prefixes.end(); ++iter) {
    for (size_t j = 0; j < prefixes.size(); ++j) {
      const SBPrefix& prefix = prefixes[j];
      if (prefix == iter->prefix && GetListIdBit(iter->chunk_id) == list_bit) {
        prefix_hits->push_back(prefix);
        found_match = true;
      }
    }
  }
  return found_match;
}

// This function generates a chunk range string for |chunks|. It
// outputs one chunk range string per list and writes it to the
// |list_ranges| vector.  We expect |list_ranges| to already be of the
// right size.  E.g., if |chunks| contains chunks with two different
// list ids then |list_ranges| must contain two elements.
void GetChunkRanges(const std::vector<int>& chunks,
                    std::vector<std::string>* list_ranges) {
  // Since there are 2 possible list ids, there must be exactly two
  // list ranges.  Even if the chunk data should only contain one
  // line, this code has to somehow handle corruption.
  DCHECK_EQ(2U, list_ranges->size());

  std::vector<std::vector<int>> decoded_chunks(list_ranges->size());
  for (std::vector<int>::const_iterator iter = chunks.begin();
       iter != chunks.end(); ++iter) {
    int mod_list_id = GetListIdBit(*iter);
    DCHECK_GE(mod_list_id, 0);
    DCHECK_LT(static_cast<size_t>(mod_list_id), decoded_chunks.size());
    decoded_chunks[mod_list_id].push_back(DecodeChunkId(*iter));
  }
  for (size_t i = 0; i < decoded_chunks.size(); ++i) {
    ChunksToRangeString(decoded_chunks[i], &((*list_ranges)[i]));
  }
}

// Helper function to create chunk range lists for Browse related
// lists.
void UpdateChunkRanges(SafeBrowsingStore* store,
                       const std::vector<std::string>& listnames,
                       std::vector<SBListChunkRanges>* lists) {
  if (!store)
    return;

  DCHECK_GT(listnames.size(), 0U);
  DCHECK_LE(listnames.size(), 2U);
  std::vector<int> add_chunks;
  std::vector<int> sub_chunks;
  store->GetAddChunks(&add_chunks);
  store->GetSubChunks(&sub_chunks);

  // Always decode 2 ranges, even if only the first one is expected.
  // The loop below will only load as many into |lists| as |listnames|
  // indicates.
  std::vector<std::string> adds(2);
  std::vector<std::string> subs(2);
  GetChunkRanges(add_chunks, &adds);
  GetChunkRanges(sub_chunks, &subs);

  for (size_t i = 0; i < listnames.size(); ++i) {
    const std::string& listname = listnames[i];
    DCHECK_EQ(GetListId(listname) % 2, static_cast<int>(i % 2));
    DCHECK_NE(GetListId(listname), INVALID);
    lists->push_back(SBListChunkRanges(listname));
    lists->back().adds.swap(adds[i]);
    lists->back().subs.swap(subs[i]);
  }
}

void UpdateChunkRangesForLists(
    SafeBrowsingStore* store,
    const std::string& listname0,
    const std::string& listname1,
    std::vector<SBListChunkRanges>* lists) {
  std::vector<std::string> listnames;
  listnames.push_back(listname0);
  listnames.push_back(listname1);
  UpdateChunkRanges(store, listnames, lists);
}

void UpdateChunkRangesForList(
    SafeBrowsingStore* store,
    const std::string& listname,
    std::vector<SBListChunkRanges>* lists) {
  UpdateChunkRanges(store, std::vector<std::string>(1, listname), lists);
}

// This code always checks for non-zero file size.  This helper makes
// that less verbose.
int64_t GetFileSizeOrZero(const base::FilePath& file_path) {
  int64_t size_64;
  if (!base::GetFileSize(file_path, &size_64))
    return 0;
  return size_64;
}

// Helper for PrefixSetContainsUrlHashes().  Returns true if an un-expired match
// for |full_hash| is found in |cache|, with any matches appended to |results|
// (true can be returned with zero matches).  |expire_base| is used to check the
// cache lifetime of matches, expired matches will be discarded from |cache|.
bool GetCachedFullHash(std::map<SBPrefix, SBCachedFullHashResult>* cache,
                       const SBFullHash& full_hash,
                       const base::Time& expire_base,
                       std::vector<SBFullHashResult>* results) {
  // First check if there is a valid cached result for this prefix.
  std::map<SBPrefix, SBCachedFullHashResult>::iterator citer =
      cache->find(full_hash.prefix);
  if (citer == cache->end())
    return false;

  // Remove expired entries.
  SBCachedFullHashResult& cached_result = citer->second;
  if (cached_result.expire_after <= expire_base) {
    cache->erase(citer);
    return false;
  }

  // Find full-hash matches.
  std::vector<SBFullHashResult>& cached_hashes =
      cached_result.full_hashes;
  for (size_t i = 0; i < cached_hashes.size(); ++i) {
    if (SBFullHashEqual(full_hash, cached_hashes[i].hash))
      results->push_back(cached_hashes[i]);
  }

  return true;
}

SafeBrowsingStoreFile* CreateStore(
    bool enable,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {
  if (!enable)
    return nullptr;
  return new SafeBrowsingStoreFile(task_runner);
}

}  // namespace

// The default SafeBrowsingDatabaseFactory.
class SafeBrowsingDatabaseFactoryImpl : public SafeBrowsingDatabaseFactory {
 public:
  std::unique_ptr<SafeBrowsingDatabase> CreateSafeBrowsingDatabase(
      const scoped_refptr<base::SequencedTaskRunner>& db_task_runner,
      bool enable_download_protection,
      bool enable_client_side_whitelist,
      bool enable_download_whitelist,
      bool enable_extension_blacklist,
      bool enable_ip_blacklist,
      bool enable_unwanted_software_list) override {
    return std::make_unique<SafeBrowsingDatabaseNew>(
        db_task_runner, CreateStore(true, db_task_runner),  // browse_store
        CreateStore(enable_download_protection, db_task_runner),
        CreateStore(enable_client_side_whitelist, db_task_runner),
        CreateStore(enable_download_whitelist, db_task_runner),
        CreateStore(enable_extension_blacklist, db_task_runner),
        CreateStore(enable_ip_blacklist, db_task_runner),
        CreateStore(enable_unwanted_software_list, db_task_runner),
        CreateStore(true, db_task_runner));  // resource_blacklist_store
  }

  SafeBrowsingDatabaseFactoryImpl() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingDatabaseFactoryImpl);
};

// static
SafeBrowsingDatabaseFactory* SafeBrowsingDatabase::factory_ = NULL;

// Factory method, should be called on the Safe Browsing sequenced task runner,
// which is also passed to the function as |current_task_runner|.
// TODO(shess): There's no need for a factory any longer.  Convert
// SafeBrowsingDatabaseNew to SafeBrowsingDatabase, and have Create()
// callers just construct things directly.
std::unique_ptr<SafeBrowsingDatabase> SafeBrowsingDatabase::Create(
    const scoped_refptr<base::SequencedTaskRunner>& current_task_runner,
    bool enable_download_protection,
    bool enable_client_side_whitelist,
    bool enable_download_whitelist,
    bool enable_extension_blacklist,
    bool enable_ip_blacklist,
    bool enable_unwanted_software_list) {
  DCHECK(current_task_runner->RunsTasksInCurrentSequence());
  if (!factory_)
    factory_ = new SafeBrowsingDatabaseFactoryImpl();
  return factory_->CreateSafeBrowsingDatabase(
      current_task_runner, enable_download_protection,
      enable_client_side_whitelist, enable_download_whitelist,
      enable_extension_blacklist, enable_ip_blacklist,
      enable_unwanted_software_list);
}

SafeBrowsingDatabase::~SafeBrowsingDatabase() {}

// static
base::FilePath SafeBrowsingDatabase::BrowseDBFilename(
    const base::FilePath& db_base_filename) {
  return base::FilePath(db_base_filename.value() + kBrowseDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::DownloadDBFilename(
    const base::FilePath& db_base_filename) {
  return base::FilePath(db_base_filename.value() + kDownloadDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::BloomFilterForFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kBloomFilterFileSuffix);
}

// static
base::FilePath SafeBrowsingDatabase::PrefixSetForFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kPrefixSetFileSuffix);
}

// static
base::FilePath SafeBrowsingDatabase::CsdWhitelistDBFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kCsdWhitelistDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::DownloadWhitelistDBFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kDownloadWhitelistDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::ExtensionBlacklistDBFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kExtensionBlacklistDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::IpBlacklistDBFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kIPBlacklistDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::UnwantedSoftwareDBFilename(
    const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kUnwantedSoftwareDBFile);
}

// static
base::FilePath SafeBrowsingDatabase::ResourceBlacklistDBFilename(
      const base::FilePath& db_filename) {
  return base::FilePath(db_filename.value() + kResourceBlacklistDBFile);
}

// static
void SafeBrowsingDatabase::GetDownloadUrlPrefixes(
    const std::vector<GURL>& urls,
    std::vector<SBPrefix>* prefixes) {
  std::vector<SBFullHash> full_hashes;
  for (size_t i = 0; i < urls.size(); ++i)
    UrlToFullHashes(urls[i], false, &full_hashes);

  for (size_t i = 0; i < full_hashes.size(); ++i)
    prefixes->push_back(full_hashes[i].prefix);
}

SafeBrowsingStore* SafeBrowsingDatabaseNew::GetStore(const int list_id) {
  // Stores are not thread safe.
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (list_id == PHISH || list_id == MALWARE) {
    return browse_store_.get();
  } else if (list_id == BINURL) {
    return download_store_.get();
  } else if (list_id == CSDWHITELIST) {
    return csd_whitelist_store_.get();
  } else if (list_id == DOWNLOADWHITELIST) {
    return download_whitelist_store_.get();
  } else if (list_id == EXTENSIONBLACKLIST) {
    return extension_blacklist_store_.get();
  } else if (list_id == IPBLACKLIST) {
    return ip_blacklist_store_.get();
  } else if (list_id == UNWANTEDURL) {
    return unwanted_software_store_.get();
  } else if (list_id == RESOURCEBLACKLIST) {
    return resource_blacklist_store_.get();
  }
  return NULL;
}

// static
void SafeBrowsingDatabase::RecordFailure(FailureType failure_type) {
  UMA_HISTOGRAM_ENUMERATION("SB2.DatabaseFailure", failure_type,
                            FAILURE_DATABASE_MAX);
}

class SafeBrowsingDatabaseNew::ThreadSafeStateManager::ReadTransaction {
 public:
  const SBWhitelist* GetSBWhitelist(SBWhitelistId id) {
    switch (id) {
      case SBWhitelistId::CSD:
        return &outer_->csd_whitelist_;
      case SBWhitelistId::DOWNLOAD:
        return &outer_->download_whitelist_;
    }
    NOTREACHED();
    return nullptr;
  }

  const IPBlacklist* ip_blacklist() { return &outer_->ip_blacklist_; }

  const PrefixSet* GetPrefixSet(PrefixSetId id) {
    switch (id) {
      case PrefixSetId::BROWSE:
        return outer_->browse_prefix_set_.get();
      case PrefixSetId::UNWANTED_SOFTWARE:
        return outer_->unwanted_software_prefix_set_.get();
    }
    NOTREACHED();
    return nullptr;
  }

  PrefixGetHashCache* prefix_gethash_cache() {
    // The cache is special: it is read/write on all threads. Access to it
    // therefore requires a LOCK'ed transaction (i.e. it can't benefit from
    // DONT_LOCK_ON_MAIN_THREAD).
    DCHECK(transaction_lock_);
    return &outer_->prefix_gethash_cache_;
  }

 private:
  // Only ThreadSafeStateManager is allowed to build a ReadTransaction.
  friend class ThreadSafeStateManager;

  enum class AutoLockRequirement {
    LOCK,
    // SBWhitelist's, IPBlacklist's, and PrefixSet's (not caches) are only
    // ever written to on the main task runner (as enforced by
    // ThreadSafeStateManager) and can therefore be read on the main task
    // runner without first acquiring |lock_|.
    DONT_LOCK_ON_MAIN_TASK_RUNNER
  };

  ReadTransaction(const ThreadSafeStateManager* outer,
                  AutoLockRequirement auto_lock_requirement)
      : outer_(outer) {
    DCHECK(outer_);
    if (auto_lock_requirement == AutoLockRequirement::LOCK)
      transaction_lock_.reset(new base::AutoLock(outer_->lock_));
    else
      DCHECK(outer_->db_task_runner_->RunsTasksInCurrentSequence());
  }

  const ThreadSafeStateManager* outer_;
  std::unique_ptr<base::AutoLock> transaction_lock_;

  DISALLOW_COPY_AND_ASSIGN(ReadTransaction);
};

class SafeBrowsingDatabaseNew::ThreadSafeStateManager::WriteTransaction {
 public:
  // Call this method if an error occured with the given whitelist.  This will
  // result in all lookups to the whitelist to return true.
  void WhitelistEverything(SBWhitelistId id) {
    SBWhitelist* whitelist = SBWhitelistForId(id);
    whitelist->second = true;
    whitelist->first.clear();
  }

  void SwapSBWhitelist(SBWhitelistId id,
                       std::vector<SBFullHash>* new_whitelist) {
    SBWhitelist* whitelist = SBWhitelistForId(id);
    whitelist->second = false;
    whitelist->first.swap(*new_whitelist);
  }

  void clear_ip_blacklist() { outer_->ip_blacklist_.clear(); }

  void swap_ip_blacklist(IPBlacklist* new_blacklist) {
    outer_->ip_blacklist_.swap(*new_blacklist);
  }

  void SwapPrefixSet(PrefixSetId id,
                     std::unique_ptr<const PrefixSet> new_prefix_set) {
    switch (id) {
      case PrefixSetId::BROWSE:
        outer_->browse_prefix_set_.swap(new_prefix_set);
        break;
      case PrefixSetId::UNWANTED_SOFTWARE:
        outer_->unwanted_software_prefix_set_.swap(new_prefix_set);
        break;
    }
  }

  void clear_prefix_gethash_cache() { outer_->prefix_gethash_cache_.clear(); }

 private:
  // Only ThreadSafeStateManager is allowed to build a WriteTransaction.
  friend class ThreadSafeStateManager;

  explicit WriteTransaction(ThreadSafeStateManager* outer)
      : outer_(outer), transaction_lock_(outer_->lock_) {
    DCHECK(outer_);
    DCHECK(outer_->db_task_runner_->RunsTasksInCurrentSequence());
  }

  SBWhitelist* SBWhitelistForId(SBWhitelistId id) {
    switch (id) {
      case SBWhitelistId::CSD:
        return &outer_->csd_whitelist_;
      case SBWhitelistId::DOWNLOAD:
        return &outer_->download_whitelist_;
    }
    NOTREACHED();
    return nullptr;
  }

  ThreadSafeStateManager* outer_;
  base::AutoLock transaction_lock_;

  DISALLOW_COPY_AND_ASSIGN(WriteTransaction);
};

SafeBrowsingDatabaseNew::ThreadSafeStateManager::ThreadSafeStateManager(
    const scoped_refptr<const base::SequencedTaskRunner>& db_task_runner)
    : db_task_runner_(db_task_runner) {}

SafeBrowsingDatabaseNew::ThreadSafeStateManager::~ThreadSafeStateManager() {}

SafeBrowsingDatabaseNew::DatabaseStateManager::DatabaseStateManager(
    const scoped_refptr<const base::SequencedTaskRunner>& db_task_runner)
    : db_task_runner_(db_task_runner),
      corruption_detected_(false),
      change_detected_(false) {}

SafeBrowsingDatabaseNew::DatabaseStateManager::~DatabaseStateManager() {}

std::unique_ptr<SafeBrowsingDatabaseNew::ReadTransaction>
SafeBrowsingDatabaseNew::ThreadSafeStateManager::BeginReadTransaction() {
  return base::WrapUnique(
      new ReadTransaction(this, ReadTransaction::AutoLockRequirement::LOCK));
}

std::unique_ptr<SafeBrowsingDatabaseNew::ReadTransaction>
SafeBrowsingDatabaseNew::ThreadSafeStateManager::
    BeginReadTransactionNoLockOnMainTaskRunner() {
  return base::WrapUnique(new ReadTransaction(
      this,
      ReadTransaction::AutoLockRequirement::DONT_LOCK_ON_MAIN_TASK_RUNNER));
}

std::unique_ptr<SafeBrowsingDatabaseNew::WriteTransaction>
SafeBrowsingDatabaseNew::ThreadSafeStateManager::BeginWriteTransaction() {
  return base::WrapUnique(new WriteTransaction(this));
}

SafeBrowsingDatabaseNew::SafeBrowsingDatabaseNew(
    const scoped_refptr<base::SequencedTaskRunner>& db_task_runner,
    SafeBrowsingStore* browse_store,
    SafeBrowsingStore* download_store,
    SafeBrowsingStore* csd_whitelist_store,
    SafeBrowsingStore* download_whitelist_store,
    SafeBrowsingStore* extension_blacklist_store,
    SafeBrowsingStore* ip_blacklist_store,
    SafeBrowsingStore* unwanted_software_store,
    SafeBrowsingStore* resource_blacklist_store)
    : db_task_runner_(db_task_runner),
      state_manager_(db_task_runner_),
      db_state_manager_(db_task_runner_),
      browse_store_(browse_store),
      download_store_(download_store),
      csd_whitelist_store_(csd_whitelist_store),
      download_whitelist_store_(download_whitelist_store),
      extension_blacklist_store_(extension_blacklist_store),
      ip_blacklist_store_(ip_blacklist_store),
      unwanted_software_store_(unwanted_software_store),
      resource_blacklist_store_(resource_blacklist_store),
      reset_factory_(this) {
  DCHECK(browse_store_.get());
}

SafeBrowsingDatabaseNew::~SafeBrowsingDatabaseNew() {
  // The DCHECK is disabled due to crbug.com/338486 .
  // DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
}

void SafeBrowsingDatabaseNew::Init(const base::FilePath& filename_base) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  db_state_manager_.init_filename_base(filename_base);

  // TODO(shess): The various stores are really only necessary while doing
  // updates (see |UpdateFinished()|) or when querying a store directly (see
  // |ContainsDownloadUrl()|).
  // The store variables are also tested to see if a list is enabled.  Perhaps
  // the stores could be refactored into an update object so that they are only
  // live in memory while being actively used.  The sense of enabled probably
  // belongs in protocol_manager or database_manager.

  {
    // NOTE: A transaction here is overkill as there are no pointers to this
    // class on other threads until this function returns, but it's also
    // harmless as that also means there is no possibility of contention on the
    // lock.
    std::unique_ptr<WriteTransaction> txn =
        state_manager_.BeginWriteTransaction();

    txn->clear_prefix_gethash_cache();

    browse_store_->Init(
        BrowseDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));

    if (unwanted_software_store_.get()) {
      unwanted_software_store_->Init(
          UnwantedSoftwareDBFilename(db_state_manager_.filename_base()),
          base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                     base::Unretained(this)));
    }
    LoadPrefixSet(BrowseDBFilename(db_state_manager_.filename_base()),
                  txn.get(), PrefixSetId::BROWSE,
                  FAILURE_BROWSE_PREFIX_SET_READ);
    if (unwanted_software_store_.get()) {
      LoadPrefixSet(
          UnwantedSoftwareDBFilename(db_state_manager_.filename_base()),
          txn.get(), PrefixSetId::UNWANTED_SOFTWARE,
          FAILURE_UNWANTED_SOFTWARE_PREFIX_SET_READ);
    }
  }
  // Note: End the transaction early because LoadWhiteList() and
  // WhitelistEverything() manage their own transactions.

  if (download_store_.get()) {
    download_store_->Init(
        DownloadDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));
  }

  if (csd_whitelist_store_.get()) {
    csd_whitelist_store_->Init(
        CsdWhitelistDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));

    std::vector<SBAddFullHash> full_hashes;
    if (csd_whitelist_store_->GetAddFullHashes(&full_hashes)) {
      LoadWhitelist(full_hashes, SBWhitelistId::CSD);
    } else {
      state_manager_.BeginWriteTransaction()->WhitelistEverything(
          SBWhitelistId::CSD);
    }
  } else {
    state_manager_.BeginWriteTransaction()->WhitelistEverything(
        SBWhitelistId::CSD);  // Just to be safe.
  }

  if (download_whitelist_store_.get()) {
    download_whitelist_store_->Init(
        DownloadWhitelistDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));

    std::vector<SBAddFullHash> full_hashes;
    if (download_whitelist_store_->GetAddFullHashes(&full_hashes)) {
      LoadWhitelist(full_hashes, SBWhitelistId::DOWNLOAD);
    } else {
      state_manager_.BeginWriteTransaction()->WhitelistEverything(
          SBWhitelistId::DOWNLOAD);
    }
  } else {
    state_manager_.BeginWriteTransaction()->WhitelistEverything(
        SBWhitelistId::DOWNLOAD);  // Just to be safe.
  }

  if (extension_blacklist_store_.get()) {
    extension_blacklist_store_->Init(
        ExtensionBlacklistDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));
  }

  if (ip_blacklist_store_.get()) {
    ip_blacklist_store_->Init(
        IpBlacklistDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));

    std::vector<SBAddFullHash> full_hashes;
    if (ip_blacklist_store_->GetAddFullHashes(&full_hashes)) {
      LoadIpBlacklist(full_hashes);
    } else {
      LoadIpBlacklist(std::vector<SBAddFullHash>());  // Clear the list.
    }
  }

  if (resource_blacklist_store_.get()) {
    resource_blacklist_store_->Init(
        ResourceBlacklistDBFilename(db_state_manager_.filename_base()),
        base::Bind(&SafeBrowsingDatabaseNew::HandleCorruptDatabase,
                   base::Unretained(this)));
  }
}

bool SafeBrowsingDatabaseNew::ResetDatabase() {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // Delete files on disk.
  // TODO(shess): Hard to see where one might want to delete without a
  // reset.  Perhaps inline |Delete()|?
  if (!Delete())
    return false;

  // Reset objects in memory.
  std::unique_ptr<WriteTransaction> txn =
      state_manager_.BeginWriteTransaction();
  txn->clear_prefix_gethash_cache();
  txn->SwapPrefixSet(PrefixSetId::BROWSE, nullptr);
  txn->SwapPrefixSet(PrefixSetId::UNWANTED_SOFTWARE, nullptr);
  txn->clear_ip_blacklist();
  txn->WhitelistEverything(SBWhitelistId::CSD);
  txn->WhitelistEverything(SBWhitelistId::DOWNLOAD);
  return true;
}

bool SafeBrowsingDatabaseNew::ContainsBrowseUrl(
    const GURL& url,
    std::vector<SBPrefix>* prefix_hits,
    std::vector<SBFullHashResult>* cache_hits) {
  return PrefixSetContainsUrl(url, PrefixSetId::BROWSE, prefix_hits,
                              cache_hits);
}

bool SafeBrowsingDatabaseNew::ContainsBrowseHashes(
    const std::vector<SBFullHash>& full_hashes,
    std::vector<SBPrefix>* prefix_hits,
    std::vector<SBFullHashResult>* cache_hits) {
  return PrefixSetContainsUrlHashes(full_hashes, PrefixSetId::BROWSE,
                                    prefix_hits, cache_hits);
}

bool SafeBrowsingDatabaseNew::ContainsUnwantedSoftwareUrl(
    const GURL& url,
    std::vector<SBPrefix>* prefix_hits,
    std::vector<SBFullHashResult>* cache_hits) {
  return PrefixSetContainsUrl(url, PrefixSetId::UNWANTED_SOFTWARE, prefix_hits,
                              cache_hits);
}

bool SafeBrowsingDatabaseNew::ContainsUnwantedSoftwareHashes(
    const std::vector<SBFullHash>& full_hashes,
    std::vector<SBPrefix>* prefix_hits,
    std::vector<SBFullHashResult>* cache_hits) {
  return PrefixSetContainsUrlHashes(full_hashes, PrefixSetId::UNWANTED_SOFTWARE,
                                    prefix_hits, cache_hits);
}

bool SafeBrowsingDatabaseNew::PrefixSetContainsUrl(
    const GURL& url,
    PrefixSetId prefix_set_id,
    std::vector<SBPrefix>* prefix_hits,
    std::vector<SBFullHashResult>* cache_hits) {
  std::vector<SBFullHash> full_hashes;
  UrlToFullHashes(url, false, &full_hashes);
  return PrefixSetContainsUrlHashes(full_hashes, prefix_set_id, prefix_hits,
                                    cache_hits);
}

bool SafeBrowsingDatabaseNew::PrefixSetContainsUrlHashes(
    const std::vector<SBFullHash>& full_hashes,
    PrefixSetId prefix_set_id,
    std::vector<SBPrefix>* prefix_hits,
    std::vector<SBFullHashResult>* cache_hits) {
  // Clear the results first.
  prefix_hits->clear();
  cache_hits->clear();

  if (full_hashes.empty())
    return false;

  // Used to determine cache expiration.
  const base::Time now = base::Time::Now();

  {
    std::unique_ptr<ReadTransaction> txn =
        state_manager_.BeginReadTransaction();

    // |prefix_set| is empty until it is either read from disk, or the first
    // update populates it.  Bail out without a hit if not yet available.
    const PrefixSet* prefix_set = txn->GetPrefixSet(prefix_set_id);
    if (!prefix_set)
      return false;

    for (size_t i = 0; i < full_hashes.size(); ++i) {
      if (!GetCachedFullHash(txn->prefix_gethash_cache(), full_hashes[i], now,
                             cache_hits)) {
        // No valid cached result, check the database.
        if (prefix_set->Exists(full_hashes[i]))
          prefix_hits->push_back(full_hashes[i].prefix);
      }
    }
  }

  // Multiple full hashes could share prefix, remove duplicates.
  std::sort(prefix_hits->begin(), prefix_hits->end());
  prefix_hits->erase(std::unique(prefix_hits->begin(), prefix_hits->end()),
                     prefix_hits->end());

  return !prefix_hits->empty() || !cache_hits->empty();
}

bool SafeBrowsingDatabaseNew::ContainsDownloadUrlPrefixes(
    const std::vector<SBPrefix>& prefixes,
    std::vector<SBPrefix>* prefix_hits) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // Ignore this check when download checking is not enabled.
  if (!download_store_.get())
    return false;

  return MatchAddPrefixes(download_store_.get(), BINURL % 2, prefixes,
                          prefix_hits);
}

bool SafeBrowsingDatabaseNew::ContainsCsdWhitelistedUrl(const GURL& url) {
  std::vector<SBFullHash> full_hashes;
  UrlToFullHashes(url, true, &full_hashes);
  return ContainsWhitelistedHashes(SBWhitelistId::CSD, full_hashes);
}

bool SafeBrowsingDatabaseNew::ContainsDownloadWhitelistedUrl(const GURL& url) {
  std::vector<SBFullHash> full_hashes;
  UrlToFullHashes(url, true, &full_hashes);
  return ContainsWhitelistedHashes(SBWhitelistId::DOWNLOAD, full_hashes);
}

bool SafeBrowsingDatabaseNew::ContainsExtensionPrefixes(
    const std::vector<SBPrefix>& prefixes,
    std::vector<SBPrefix>* prefix_hits) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (!extension_blacklist_store_)
    return false;

  return MatchAddPrefixes(extension_blacklist_store_.get(),
                          EXTENSIONBLACKLIST % 2, prefixes, prefix_hits);
}

bool SafeBrowsingDatabaseNew::ContainsMalwareIP(const std::string& ip_address) {
  net::IPAddress address;
  if (!V4ProtocolManagerUtil::GetIPV6AddressFromString(ip_address, &address)) {
    return false;
  }

  std::unique_ptr<ReadTransaction> txn = state_manager_.BeginReadTransaction();
  const IPBlacklist* ip_blacklist = txn->ip_blacklist();
  for (IPBlacklist::const_iterator it = ip_blacklist->begin();
       it != ip_blacklist->end(); ++it) {
    const std::string& mask = it->first;
    DCHECK_EQ(mask.size(), address.size());
    std::string subnet(net::IPAddress::kIPv6AddressSize, '\0');
    for (size_t i = 0; i < net::IPAddress::kIPv6AddressSize; ++i) {
      subnet[i] = address.bytes()[i] & mask[i];
    }
    const std::string hash = base::SHA1HashString(subnet);
    DVLOG(2) << "Lookup Malware IP: "
             << " ip:" << ip_address
             << " mask:" << base::HexEncode(mask.data(), mask.size())
             << " subnet:" << base::HexEncode(subnet.data(), subnet.size())
             << " hash:" << base::HexEncode(hash.data(), hash.size());
    if (it->second.count(hash) > 0) {
      return true;
    }
  }
  return false;
}

bool SafeBrowsingDatabaseNew::ContainsResourceUrlPrefixes(
    const std::vector<SBPrefix>& prefixes,
    std::vector<SBPrefix>* prefix_hits) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (!resource_blacklist_store_)
    return false;

  return MatchAddPrefixes(resource_blacklist_store_.get(),
                          RESOURCEBLACKLIST % 2, prefixes, prefix_hits);
}

bool SafeBrowsingDatabaseNew::ContainsDownloadWhitelistedString(
    const std::string& str) {
  std::vector<SBFullHash> hashes;
  hashes.push_back(SBFullHashForString(str));
  return ContainsWhitelistedHashes(SBWhitelistId::DOWNLOAD, hashes);
}

bool SafeBrowsingDatabaseNew::ContainsWhitelistedHashes(
    SBWhitelistId whitelist_id,
    const std::vector<SBFullHash>& hashes) {
  std::unique_ptr<ReadTransaction> txn = state_manager_.BeginReadTransaction();
  const SBWhitelist* whitelist = txn->GetSBWhitelist(whitelist_id);
  if (whitelist->second)
    return true;
  for (std::vector<SBFullHash>::const_iterator it = hashes.begin();
       it != hashes.end(); ++it) {
    if (std::binary_search(whitelist->first.begin(), whitelist->first.end(),
                           *it, SBFullHashLess)) {
      return true;
    }
  }
  return false;
}

// Helper to insert add-chunk entries.
void SafeBrowsingDatabaseNew::InsertAddChunk(SafeBrowsingStore* store,
                                             const ListType list_id,
                                             const SBChunkData& chunk_data) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(store);

  // The server can give us a chunk that we already have because
  // it's part of a range.  Don't add it again.
  const int chunk_id = chunk_data.ChunkNumber();
  const int encoded_chunk_id = EncodeChunkId(chunk_id, list_id);
  if (store->CheckAddChunk(encoded_chunk_id))
    return;

  store->SetAddChunk(encoded_chunk_id);
  if (chunk_data.IsPrefix()) {
    const size_t c = chunk_data.PrefixCount();
    for (size_t i = 0; i < c; ++i) {
      store->WriteAddPrefix(encoded_chunk_id, chunk_data.PrefixAt(i));
    }
  } else {
    const size_t c = chunk_data.FullHashCount();
    for (size_t i = 0; i < c; ++i) {
      store->WriteAddHash(encoded_chunk_id, chunk_data.FullHashAt(i));
    }
  }
}

// Helper to insert sub-chunk entries.
void SafeBrowsingDatabaseNew::InsertSubChunk(SafeBrowsingStore* store,
                                             const ListType list_id,
                                             const SBChunkData& chunk_data) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(store);

  // The server can give us a chunk that we already have because
  // it's part of a range.  Don't add it again.
  const int chunk_id = chunk_data.ChunkNumber();
  const int encoded_chunk_id = EncodeChunkId(chunk_id, list_id);
  if (store->CheckSubChunk(encoded_chunk_id))
    return;

  store->SetSubChunk(encoded_chunk_id);
  if (chunk_data.IsPrefix()) {
    const size_t c = chunk_data.PrefixCount();
    for (size_t i = 0; i < c; ++i) {
      const int add_chunk_id = chunk_data.AddChunkNumberAt(i);
      const int encoded_add_chunk_id = EncodeChunkId(add_chunk_id, list_id);
      store->WriteSubPrefix(encoded_chunk_id, encoded_add_chunk_id,
                            chunk_data.PrefixAt(i));
    }
  } else {
    const size_t c = chunk_data.FullHashCount();
    for (size_t i = 0; i < c; ++i) {
      const int add_chunk_id = chunk_data.AddChunkNumberAt(i);
      const int encoded_add_chunk_id = EncodeChunkId(add_chunk_id, list_id);
      store->WriteSubHash(encoded_chunk_id, encoded_add_chunk_id,
                          chunk_data.FullHashAt(i));
    }
  }
}

void SafeBrowsingDatabaseNew::InsertChunks(
    const std::string& list_name,
    const std::vector<std::unique_ptr<SBChunkData>>& chunks) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (db_state_manager_.corruption_detected() || chunks.empty())
    return;

  const base::TimeTicks before = base::TimeTicks::Now();

  // TODO(shess): The caller should just pass list_id.
  const ListType list_id = GetListId(list_name);

  SafeBrowsingStore* store = GetStore(list_id);
  if (!store)
    return;

  db_state_manager_.set_change_detected();

  // TODO(shess): I believe that the list is always add or sub.  Can this use
  // that productively?
  store->BeginChunk();
  for (const auto& chunk : chunks) {
    if (chunk->IsAdd()) {
      InsertAddChunk(store, list_id, *chunk);
    } else if (chunk->IsSub()) {
      InsertSubChunk(store, list_id, *chunk);
    } else {
      NOTREACHED();
    }
  }
  store->FinishChunk();

  UMA_HISTOGRAM_TIMES("SB2.ChunkInsert", base::TimeTicks::Now() - before);
}

void SafeBrowsingDatabaseNew::DeleteChunks(
    const std::vector<SBChunkDelete>& chunk_deletes) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (db_state_manager_.corruption_detected() || chunk_deletes.empty())
    return;

  const std::string& list_name = chunk_deletes.front().list_name;
  const ListType list_id = GetListId(list_name);

  SafeBrowsingStore* store = GetStore(list_id);
  if (!store)
    return;

  db_state_manager_.set_change_detected();

  for (size_t i = 0; i < chunk_deletes.size(); ++i) {
    std::vector<int> chunk_numbers;
    RangesToChunks(chunk_deletes[i].chunk_del, &chunk_numbers);
    for (size_t j = 0; j < chunk_numbers.size(); ++j) {
      const int encoded_chunk_id = EncodeChunkId(chunk_numbers[j], list_id);
      if (chunk_deletes[i].is_sub_del)
        store->DeleteSubChunk(encoded_chunk_id);
      else
        store->DeleteAddChunk(encoded_chunk_id);
    }
  }
}

void SafeBrowsingDatabaseNew::CacheHashResults(
    const std::vector<SBPrefix>& prefixes,
    const std::vector<SBFullHashResult>& full_hits,
    const base::TimeDelta& cache_lifetime) {
  const base::Time expire_after = base::Time::Now() + cache_lifetime;

  std::unique_ptr<ReadTransaction> txn = state_manager_.BeginReadTransaction();
  PrefixGetHashCache* prefix_gethash_cache = txn->prefix_gethash_cache();

  // Create or reset all cached results for these prefixes.
  for (size_t i = 0; i < prefixes.size(); ++i) {
    (*prefix_gethash_cache)[prefixes[i]] = SBCachedFullHashResult(expire_after);
  }

  // Insert any fullhash hits. Note that there may be one, multiple, or no
  // fullhashes for any given entry in |prefixes|.
  for (size_t i = 0; i < full_hits.size(); ++i) {
    const SBPrefix prefix = full_hits[i].hash.prefix;
    (*prefix_gethash_cache)[prefix].full_hashes.push_back(full_hits[i]);
  }
}

bool SafeBrowsingDatabaseNew::UpdateStarted(
    std::vector<SBListChunkRanges>* lists) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(lists);

  // If |BeginUpdate()| fails, reset the database.
  if (!browse_store_->BeginUpdate()) {
    RecordFailure(FAILURE_BROWSE_DATABASE_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (download_store_.get() && !download_store_->BeginUpdate()) {
    RecordFailure(FAILURE_DOWNLOAD_DATABASE_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (csd_whitelist_store_.get() && !csd_whitelist_store_->BeginUpdate()) {
    RecordFailure(FAILURE_WHITELIST_DATABASE_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (download_whitelist_store_.get() &&
      !download_whitelist_store_->BeginUpdate()) {
    RecordFailure(FAILURE_WHITELIST_DATABASE_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (extension_blacklist_store_ &&
      !extension_blacklist_store_->BeginUpdate()) {
    RecordFailure(FAILURE_EXTENSION_BLACKLIST_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (ip_blacklist_store_ && !ip_blacklist_store_->BeginUpdate()) {
    RecordFailure(FAILURE_IP_BLACKLIST_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (unwanted_software_store_ && !unwanted_software_store_->BeginUpdate()) {
    RecordFailure(FAILURE_UNWANTED_SOFTWARE_DATABASE_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  if (resource_blacklist_store_ && !resource_blacklist_store_->BeginUpdate()) {
    RecordFailure(FAILURE_RESOURCE_BLACKLIST_UPDATE_BEGIN);
    HandleCorruptDatabase();
    return false;
  }

  // Cached fullhash results must be cleared on every database update (whether
  // successful or not).
  state_manager_.BeginWriteTransaction()->clear_prefix_gethash_cache();

  UpdateChunkRangesForLists(browse_store_.get(), kMalwareList, kPhishingList,
                            lists);

  // NOTE(shess): |download_store_| used to contain kBinHashList, which has been
  // deprecated.  Code to delete the list from the store shows ~15k hits/day as
  // of Feb 2014, so it has been removed.  Everything _should_ be resilient to
  // extra data of that sort.
  UpdateChunkRangesForList(download_store_.get(), kBinUrlList, lists);

  UpdateChunkRangesForList(csd_whitelist_store_.get(), kCsdWhiteList, lists);

  UpdateChunkRangesForList(download_whitelist_store_.get(), kDownloadWhiteList,
                           lists);

  UpdateChunkRangesForList(extension_blacklist_store_.get(),
                           kExtensionBlacklist, lists);

  UpdateChunkRangesForList(ip_blacklist_store_.get(), kIPBlacklist, lists);

  UpdateChunkRangesForList(unwanted_software_store_.get(), kUnwantedUrlList,
                           lists);

  UpdateChunkRangesForList(resource_blacklist_store_.get(), kResourceBlacklist,
                           lists);

  db_state_manager_.reset_corruption_detected();
  db_state_manager_.reset_change_detected();
  return true;
}

void SafeBrowsingDatabaseNew::UpdateFinished(bool update_succeeded) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // The update may have failed due to corrupt storage (for instance,
  // an excessive number of invalid add_chunks and sub_chunks).
  // Double-check that the databases are valid.
  // TODO(shess): Providing a checksum for the add_chunk and sub_chunk
  // sections would allow throwing a corruption error in
  // UpdateStarted().
  if (!update_succeeded) {
    if (!browse_store_->CheckValidity())
      DLOG(ERROR) << "Safe-browsing browse database corrupt.";

    if (download_store_.get() && !download_store_->CheckValidity())
      DLOG(ERROR) << "Safe-browsing download database corrupt.";

    if (csd_whitelist_store_.get() && !csd_whitelist_store_->CheckValidity())
      DLOG(ERROR) << "Safe-browsing csd whitelist database corrupt.";

    if (download_whitelist_store_.get() &&
        !download_whitelist_store_->CheckValidity()) {
      DLOG(ERROR) << "Safe-browsing download whitelist database corrupt.";
    }

    if (extension_blacklist_store_ &&
        !extension_blacklist_store_->CheckValidity()) {
      DLOG(ERROR) << "Safe-browsing extension blacklist database corrupt.";
    }

    if (ip_blacklist_store_ && !ip_blacklist_store_->CheckValidity()) {
      DLOG(ERROR) << "Safe-browsing IP blacklist database corrupt.";
    }

    if (unwanted_software_store_ &&
        !unwanted_software_store_->CheckValidity()) {
      DLOG(ERROR) << "Unwanted software url list database corrupt.";
    }

    if (resource_blacklist_store_ &&
        !resource_blacklist_store_->CheckValidity()) {
      DLOG(ERROR) << "Resources blacklist url list database corrupt.";
    }
  }

  if (db_state_manager_.corruption_detected())
    return;

  // Unroll the transaction if there was a protocol error or if the
  // transaction was empty.  This will leave the prefix set, the
  // pending hashes, and the prefix miss cache in place.
  if (!update_succeeded || !db_state_manager_.change_detected()) {
    // Track empty updates to answer questions at http://crbug.com/72216 .
    if (update_succeeded && !db_state_manager_.change_detected())
      UMA_HISTOGRAM_COUNTS("SB2.DatabaseUpdateKilobytes", 0);
    browse_store_->CancelUpdate();
    if (download_store_.get())
      download_store_->CancelUpdate();
    if (csd_whitelist_store_.get())
      csd_whitelist_store_->CancelUpdate();
    if (download_whitelist_store_.get())
      download_whitelist_store_->CancelUpdate();
    if (extension_blacklist_store_)
      extension_blacklist_store_->CancelUpdate();
    if (ip_blacklist_store_)
      ip_blacklist_store_->CancelUpdate();
    if (unwanted_software_store_)
      unwanted_software_store_->CancelUpdate();
    if (resource_blacklist_store_)
      resource_blacklist_store_->CancelUpdate();
    return;
  }

  if (download_store_) {
    UpdateHashPrefixStore(DownloadDBFilename(db_state_manager_.filename_base()),
                          download_store_.get(),
                          FAILURE_DOWNLOAD_DATABASE_UPDATE_FINISH);
  }

  UpdatePrefixSetUrlStore(BrowseDBFilename(db_state_manager_.filename_base()),
                          browse_store_.get(), PrefixSetId::BROWSE,
                          FAILURE_BROWSE_DATABASE_UPDATE_FINISH,
                          FAILURE_BROWSE_PREFIX_SET_WRITE, true);

  UpdateWhitelistStore(
      CsdWhitelistDBFilename(db_state_manager_.filename_base()),
      csd_whitelist_store_.get(), SBWhitelistId::CSD);
  UpdateWhitelistStore(
      DownloadWhitelistDBFilename(db_state_manager_.filename_base()),
      download_whitelist_store_.get(), SBWhitelistId::DOWNLOAD);

  if (extension_blacklist_store_) {
    UpdateHashPrefixStore(
        ExtensionBlacklistDBFilename(db_state_manager_.filename_base()),
        extension_blacklist_store_.get(),
        FAILURE_EXTENSION_BLACKLIST_UPDATE_FINISH);
  }

  if (ip_blacklist_store_)
    UpdateIpBlacklistStore();

  if (unwanted_software_store_) {
    UpdatePrefixSetUrlStore(
        UnwantedSoftwareDBFilename(db_state_manager_.filename_base()),
        unwanted_software_store_.get(), PrefixSetId::UNWANTED_SOFTWARE,
        FAILURE_UNWANTED_SOFTWARE_DATABASE_UPDATE_FINISH,
        FAILURE_UNWANTED_SOFTWARE_PREFIX_SET_WRITE, true);
  }

  if (resource_blacklist_store_) {
    UpdateHashPrefixStore(
        ResourceBlacklistDBFilename(db_state_manager_.filename_base()),
        resource_blacklist_store_.get(),
        FAILURE_RESOURCE_BLACKLIST_UPDATE_FINISH);
  }
}

void SafeBrowsingDatabaseNew::UpdateWhitelistStore(
    const base::FilePath& store_filename,
    SafeBrowsingStore* store,
    SBWhitelistId whitelist_id) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (!store)
    return;

  // Note: |builder| will not be empty.  The current data store implementation
  // stores all full-length hashes as both full and prefix hashes.
  PrefixSetBuilder builder;
  std::vector<SBAddFullHash> full_hashes;
  if (!store->FinishUpdate(&builder, &full_hashes)) {
    RecordFailure(FAILURE_WHITELIST_DATABASE_UPDATE_FINISH);
    state_manager_.BeginWriteTransaction()->WhitelistEverything(whitelist_id);
    return;
  }

  RecordFileSizeHistogram(store_filename);

#if defined(OS_MACOSX)
  base::mac::SetFileBackupExclusion(store_filename);
#endif

  LoadWhitelist(full_hashes, whitelist_id);
}

void SafeBrowsingDatabaseNew::UpdateHashPrefixStore(
    const base::FilePath& store_filename,
    SafeBrowsingStore* store,
    FailureType failure_type) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // These results are not used after this call. Simply ignore the
  // returned value after FinishUpdate(...).
  PrefixSetBuilder builder;
  std::vector<SBAddFullHash> add_full_hashes_result;

  if (!store->FinishUpdate(&builder, &add_full_hashes_result))
    RecordFailure(failure_type);

  RecordFileSizeHistogram(store_filename);

#if defined(OS_MACOSX)
  base::mac::SetFileBackupExclusion(store_filename);
#endif
}

void SafeBrowsingDatabaseNew::UpdatePrefixSetUrlStore(
    const base::FilePath& db_filename,
    SafeBrowsingStore* url_store,
    PrefixSetId prefix_set_id,
    FailureType finish_failure_type,
    FailureType write_failure_type,
    bool store_full_hashes_in_prefix_set) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(url_store);

  // Measure the amount of IO during the filter build.
  base::IoCounters io_before, io_after;
  std::unique_ptr<base::ProcessMetrics> metric(
      base::ProcessMetrics::CreateCurrentProcessMetrics());

  // IoCounters are currently not supported on Mac, and may not be
  // available for Linux, so we check the result and only show IO
  // stats if they are available.
  const bool got_counters = metric->GetIOCounters(&io_before);

  const base::TimeTicks before = base::TimeTicks::Now();

  // TODO(shess): Perhaps refactor to let builder accumulate full hashes on the
  // fly?  Other clients use the SBAddFullHash vector, but AFAICT they only use
  // the SBFullHash portion.  It would need an accessor on PrefixSet.
  PrefixSetBuilder builder;
  std::vector<SBAddFullHash> add_full_hashes;
  if (!url_store->FinishUpdate(&builder, &add_full_hashes)) {
    RecordFailure(finish_failure_type);
    return;
  }

  std::unique_ptr<const PrefixSet> new_prefix_set;
  if (store_full_hashes_in_prefix_set) {
    std::vector<SBFullHash> full_hash_results;
    for (size_t i = 0; i < add_full_hashes.size(); ++i) {
      full_hash_results.push_back(add_full_hashes[i].full_hash);
    }

    new_prefix_set = builder.GetPrefixSet(full_hash_results);
  } else {
    // TODO(gab): Ensure that stores which do not want full hashes just don't
    // have full hashes in the first place and remove
    // |store_full_hashes_in_prefix_set| and the code specialization incurred
    // here.
    new_prefix_set = builder.GetPrefixSetNoHashes();
  }

  // Swap in the newly built filter.
  state_manager_.BeginWriteTransaction()->SwapPrefixSet(
      prefix_set_id, std::move(new_prefix_set));

  UMA_HISTOGRAM_LONG_TIMES("SB2.BuildFilter", base::TimeTicks::Now() - before);

  WritePrefixSet(db_filename, prefix_set_id, write_failure_type);

  // Gather statistics.
  if (got_counters && metric->GetIOCounters(&io_after)) {
    UMA_HISTOGRAM_COUNTS("SB2.BuildReadKilobytes",
                         static_cast<int>(io_after.ReadTransferCount -
                                          io_before.ReadTransferCount) /
                             1024);
    UMA_HISTOGRAM_COUNTS("SB2.BuildWriteKilobytes",
                         static_cast<int>(io_after.WriteTransferCount -
                                          io_before.WriteTransferCount) /
                             1024);
    UMA_HISTOGRAM_COUNTS("SB2.BuildReadOperations",
                         static_cast<int>(io_after.ReadOperationCount -
                                          io_before.ReadOperationCount));
    UMA_HISTOGRAM_COUNTS("SB2.BuildWriteOperations",
                         static_cast<int>(io_after.WriteOperationCount -
                                          io_before.WriteOperationCount));
  }

  RecordFileSizeHistogram(db_filename);

#if defined(OS_MACOSX)
  base::mac::SetFileBackupExclusion(db_filename);
#endif
}

void SafeBrowsingDatabaseNew::UpdateIpBlacklistStore() {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // Note: prefixes will not be empty.  The current data store implementation
  // stores all full-length hashes as both full and prefix hashes.
  PrefixSetBuilder builder;
  std::vector<SBAddFullHash> full_hashes;
  if (!ip_blacklist_store_->FinishUpdate(&builder, &full_hashes)) {
    RecordFailure(FAILURE_IP_BLACKLIST_UPDATE_FINISH);
    LoadIpBlacklist(std::vector<SBAddFullHash>());  // Clear the list.
    return;
  }

  const base::FilePath ip_blacklist_filename =
      IpBlacklistDBFilename(db_state_manager_.filename_base());

  RecordFileSizeHistogram(ip_blacklist_filename);

#if defined(OS_MACOSX)
  base::mac::SetFileBackupExclusion(ip_blacklist_filename);
#endif

  LoadIpBlacklist(full_hashes);
}

void SafeBrowsingDatabaseNew::HandleCorruptDatabase() {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // Reset the database after the current task has unwound (but only
  // reset once within the scope of a given task).
  if (!reset_factory_.HasWeakPtrs()) {
    RecordFailure(FAILURE_DATABASE_CORRUPT);
    db_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&SafeBrowsingDatabaseNew::OnHandleCorruptDatabase,
                       reset_factory_.GetWeakPtr()));
  }
}

void SafeBrowsingDatabaseNew::OnHandleCorruptDatabase() {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  RecordFailure(FAILURE_DATABASE_CORRUPT_HANDLER);
  db_state_manager_.set_corruption_detected();  // Stop updating the database.
  ResetDatabase();

  // NOTE(shess): ResetDatabase() should remove the corruption, so this should
  // only happen once.  If you are here because you are hitting this after a
  // restart, then I would be very interested in working with you to figure out
  // what is happening, since it may affect real users.
  DLOG(FATAL) << "SafeBrowsing database was corrupt and reset";
}

// TODO(shess): I'm not clear why this code doesn't have any
// real error-handling.
void SafeBrowsingDatabaseNew::LoadPrefixSet(const base::FilePath& db_filename,
                                            WriteTransaction* txn,
                                            PrefixSetId prefix_set_id,
                                            FailureType read_failure_type) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(txn);
  DCHECK(!db_state_manager_.filename_base().empty());

  // Only use the prefix set if database is present and non-empty.
  if (!GetFileSizeOrZero(db_filename))
    return;

  // Cleanup any stale bloom filter (no longer used).
  // TODO(shess): Track existence to drive removal of this code?
  const base::FilePath bloom_filter_filename =
      BloomFilterForFilename(db_filename);
  base::DeleteFile(bloom_filter_filename, false);

  const base::TimeTicks before = base::TimeTicks::Now();
  std::unique_ptr<const PrefixSet> new_prefix_set =
      PrefixSet::LoadFile(PrefixSetForFilename(db_filename));
  if (!new_prefix_set.get())
    RecordFailure(read_failure_type);
  txn->SwapPrefixSet(prefix_set_id, std::move(new_prefix_set));
  UMA_HISTOGRAM_TIMES("SB2.PrefixSetLoad", base::TimeTicks::Now() - before);
}

bool SafeBrowsingDatabaseNew::Delete() {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(!db_state_manager_.filename_base().empty());

  // TODO(shess): This is a mess.  SafeBrowsingFileStore::Delete() closes the
  // store before calling DeleteStore().  DeleteStore() deletes transient files
  // in addition to the main file.  Probably all of these should be converted to
  // a helper which calls Delete() if the store exists, else DeleteStore() on
  // the generated filename.

  // TODO(shess): Determine if the histograms are useful in any way.  I cannot
  // recall any action taken as a result of their values, in which case it might
  // make more sense to histogram an overall thumbs-up/-down and just dig deeper
  // if something looks wrong.

  const bool r1 = browse_store_->Delete();
  if (!r1)
    RecordFailure(FAILURE_DATABASE_STORE_DELETE);

  const bool r2 = download_store_.get() ? download_store_->Delete() : true;
  if (!r2)
    RecordFailure(FAILURE_DATABASE_STORE_DELETE);

  const bool r3 =
      csd_whitelist_store_.get() ? csd_whitelist_store_->Delete() : true;
  if (!r3)
    RecordFailure(FAILURE_DATABASE_STORE_DELETE);

  const bool r4 = download_whitelist_store_.get()
                      ? download_whitelist_store_->Delete()
                      : true;
  if (!r4)
    RecordFailure(FAILURE_DATABASE_STORE_DELETE);

  const base::FilePath browse_filename =
      BrowseDBFilename(db_state_manager_.filename_base());
  const base::FilePath bloom_filter_filename =
      BloomFilterForFilename(browse_filename);
  const bool r5 = base::DeleteFile(bloom_filter_filename, false);
  if (!r5)
    RecordFailure(FAILURE_DATABASE_FILTER_DELETE);

  const base::FilePath browse_prefix_set_filename =
      PrefixSetForFilename(browse_filename);
  const bool r6 = base::DeleteFile(browse_prefix_set_filename, false);
  if (!r6)
    RecordFailure(FAILURE_BROWSE_PREFIX_SET_DELETE);

  const base::FilePath extension_blacklist_filename =
      ExtensionBlacklistDBFilename(db_state_manager_.filename_base());
  const bool r7 = base::DeleteFile(extension_blacklist_filename, false);
  if (!r7)
    RecordFailure(FAILURE_EXTENSION_BLACKLIST_DELETE);

  const bool r8 = base::DeleteFile(
      IpBlacklistDBFilename(db_state_manager_.filename_base()), false);
  if (!r8)
    RecordFailure(FAILURE_IP_BLACKLIST_DELETE);

  const bool r9 = base::DeleteFile(
      UnwantedSoftwareDBFilename(db_state_manager_.filename_base()), false);
  if (!r9)
    RecordFailure(FAILURE_UNWANTED_SOFTWARE_PREFIX_SET_DELETE);

  const bool r10 = base::DeleteFile(
      ResourceBlacklistDBFilename(db_state_manager_.filename_base()), false);
  if (!r10)
    RecordFailure(FAILURE_RESOURCE_BLACKLIST_DELETE);

  return r1 && r2 && r3 && r4 && r5 && r6 && r7 && r8 && r9 && r10;
}

void SafeBrowsingDatabaseNew::WritePrefixSet(const base::FilePath& db_filename,
                                             PrefixSetId prefix_set_id,
                                             FailureType write_failure_type) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  // Do not grab the lock to avoid contention while writing to disk. This is
  // safe as only this task runner can ever modify |state_manager_|'s prefix
  // sets anyways.
  std::unique_ptr<ReadTransaction> txn =
      state_manager_.BeginReadTransactionNoLockOnMainTaskRunner();
  const PrefixSet* prefix_set = txn->GetPrefixSet(prefix_set_id);

  if (!prefix_set)
    return;

  const base::FilePath prefix_set_filename = PrefixSetForFilename(db_filename);

  const base::TimeTicks before = base::TimeTicks::Now();
  const bool write_ok = prefix_set->WriteFile(prefix_set_filename);
  UMA_HISTOGRAM_TIMES("SB2.PrefixSetWrite", base::TimeTicks::Now() - before);

  RecordFileSizeHistogram(prefix_set_filename);

  if (!write_ok)
    RecordFailure(write_failure_type);

#if defined(OS_MACOSX)
  base::mac::SetFileBackupExclusion(prefix_set_filename);
#endif
}

void SafeBrowsingDatabaseNew::LoadWhitelist(
    const std::vector<SBAddFullHash>& full_hashes,
    SBWhitelistId whitelist_id) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (full_hashes.size() > kMaxWhitelistSize) {
    state_manager_.BeginWriteTransaction()->WhitelistEverything(whitelist_id);
    return;
  }

  std::vector<SBFullHash> new_whitelist;
  new_whitelist.reserve(full_hashes.size());
  for (std::vector<SBAddFullHash>::const_iterator it = full_hashes.begin();
       it != full_hashes.end(); ++it) {
    new_whitelist.push_back(it->full_hash);
  }
  std::sort(new_whitelist.begin(), new_whitelist.end(), SBFullHashLess);

  state_manager_.BeginWriteTransaction()->SwapSBWhitelist(whitelist_id,
                                                          &new_whitelist);
}

void SafeBrowsingDatabaseNew::LoadIpBlacklist(
    const std::vector<SBAddFullHash>& full_hashes) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  IPBlacklist new_blacklist;
  for (std::vector<SBAddFullHash>::const_iterator it = full_hashes.begin();
       it != full_hashes.end(); ++it) {
    const char* full_hash = it->full_hash.full_hash;
    DCHECK_EQ(crypto::kSHA256Length, arraysize(it->full_hash.full_hash));
    // The format of the IP blacklist is:
    // SHA-1(IPv6 prefix) + uint8_t(prefix size) + 11 unused bytes.
    std::string hashed_ip_prefix(full_hash, base::kSHA1Length);
    size_t prefix_size = static_cast<uint8_t>(full_hash[base::kSHA1Length]);
    if (prefix_size > kMaxIpPrefixSize || prefix_size < kMinIpPrefixSize) {
      RecordFailure(FAILURE_IP_BLACKLIST_UPDATE_INVALID);
      new_blacklist.clear();  // Load empty blacklist.
      break;
    }

    // We precompute the mask for the given subnet size to speed up lookups.
    // Basically we need to create a 16B long string which has the highest
    // |size| bits sets to one.
    std::string mask(net::IPAddress::kIPv6AddressSize, '\0');
    mask.replace(0, prefix_size / 8, prefix_size / 8, '\xFF');
    if ((prefix_size % 8) != 0) {
      mask[prefix_size / 8] = 0xFF << (8 - (prefix_size % 8));
    }
    DVLOG(2) << "Inserting malicious IP: "
             << " raw:" << base::HexEncode(full_hash, crypto::kSHA256Length)
             << " mask:" << base::HexEncode(mask.data(), mask.size())
             << " prefix_size:" << prefix_size
             << " hashed_ip:" << base::HexEncode(hashed_ip_prefix.data(),
                                                 hashed_ip_prefix.size());
    new_blacklist[mask].insert(hashed_ip_prefix);
  }

  state_manager_.BeginWriteTransaction()->swap_ip_blacklist(&new_blacklist);
}

SafeBrowsingDatabaseNew::PrefixGetHashCache*
SafeBrowsingDatabaseNew::GetUnsynchronizedPrefixGetHashCacheForTesting() {
  return state_manager_.BeginReadTransaction()->prefix_gethash_cache();
}

void SafeBrowsingDatabaseNew::RecordFileSizeHistogram(
    const base::FilePath& file_path) {
  const int64_t file_size = GetFileSizeOrZero(file_path);
  const int file_size_kilobytes = static_cast<int>(file_size / 1024);

  base::FilePath::StringType filename = file_path.BaseName().value();

  // Default to logging DB sizes unless |file_path| points at PrefixSet storage.
  std::string histogram_name("SB2.DatabaseSizeKilobytes");
  if (base::EndsWith(filename, kPrefixSetFileSuffix,
                     base::CompareCase::SENSITIVE)) {
    histogram_name = "SB2.PrefixSetSizeKilobytes";
    // Clear the PrefixSet suffix to have the histogram suffix selector below
    // work the same for PrefixSet-based storage as it does for simple safe
    // browsing stores.
    // The size of the kPrefixSetFileSuffix is the size of its array minus 1 as
    // the array includes the terminating '\0'.
    const size_t kPrefixSetSuffixSize = arraysize(kPrefixSetFileSuffix) - 1;
    filename.erase(filename.size() - kPrefixSetSuffixSize);
  }

  // Changes to histogram suffixes below need to be mirrored in the
  // SafeBrowsingLists suffix enum in histograms.xml.
  if (base::EndsWith(filename, kBrowseDBFile, base::CompareCase::SENSITIVE))
    histogram_name.append(".Browse");
  else if (base::EndsWith(filename, kDownloadDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".Download");
  else if (base::EndsWith(filename, kCsdWhitelistDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".CsdWhitelist");
  else if (base::EndsWith(filename, kDownloadWhitelistDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".DownloadWhitelist");
  else if (base::EndsWith(filename, kExtensionBlacklistDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".ExtensionBlacklist");
  else if (base::EndsWith(filename, kIPBlacklistDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".IPBlacklist");
  else if (base::EndsWith(filename, kUnwantedSoftwareDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".UnwantedSoftware");
  else if (base::EndsWith(filename, kResourceBlacklistDBFile,
                          base::CompareCase::SENSITIVE))
    histogram_name.append(".ResourceBlacklist");
  else
    NOTREACHED();  // Add support for new lists above.

  // Histogram properties as in UMA_HISTOGRAM_COUNTS macro.
  base::HistogramBase* histogram_pointer = base::Histogram::FactoryGet(
      histogram_name, 1, 1000000, 50,
      base::HistogramBase::kUmaTargetedHistogramFlag);

  histogram_pointer->Add(file_size_kilobytes);
}

}  // namespace safe_browsing
