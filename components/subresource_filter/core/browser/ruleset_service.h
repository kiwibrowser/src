// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_BROWSER_RULESET_SERVICE_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_BROWSER_RULESET_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/version.h"

class PrefService;
class PrefRegistrySimple;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace subresource_filter {

class RulesetIndexer;
class RulesetServiceDelegate;

// Encapsulates information about a version of unindexed subresource
// filtering rules on disk.
struct UnindexedRulesetInfo {
  UnindexedRulesetInfo();
  ~UnindexedRulesetInfo();

  // The version of the ruleset contents. Because the wire format of unindexed
  // rules is expected to be stable over time (at least backwards compatible),
  // the unindexed ruleset is uniquely identified by its content version.
  //
  // The version string must not be empty, but can be any string otherwise.
  // There is no ordering defined on versions.
  std::string content_version;

  // The path to the file containing the unindexed subresource filtering rules.
  base::FilePath ruleset_path;

  // The (optional) path to a file containing the applicable license, which will
  // be copied next to the indexed ruleset. For convenience, the lack of license
  // can be indicated not only by setting |license_path| to empty, but also by
  // setting it to any non existent path.
  base::FilePath license_path;
};

// Encapsulates the combination of the binary format version of the indexed
// ruleset, and the version of the ruleset contents.
//
// In contrast to the unindexed ruleset, the binary format of the index data
// structures is expected to evolve over time, so the indexed ruleset is
// identified by a pair of versions: the content version of the rules that have
// been indexed; and the binary format version of the indexed data structures.
struct IndexedRulesetVersion {
  IndexedRulesetVersion();
  IndexedRulesetVersion(const std::string& content_version, int format_version);
  ~IndexedRulesetVersion();
  IndexedRulesetVersion& operator=(const IndexedRulesetVersion&);

  static void RegisterPrefs(PrefRegistrySimple* registry);
  static int CurrentFormatVersion();

  bool IsValid() const;
  bool IsCurrentFormatVersion() const;

  void SaveToPrefs(PrefService* local_state) const;
  void ReadFromPrefs(PrefService* local_state);

  std::string content_version;
  int format_version = 0;
};

// Contains all utility functions that govern how files pertaining to indexed
// ruleset version should be organized on disk.
//
// The various indexed ruleset versions are kept in a two-level directory
// hierarchy based on their format and content version numbers, like so:
//
//   |base_dir|
//    |
//    +--10 (format_version)
//    |  |
//    |  +--1 (content_version)
//    |  |   \...
//    |  |
//    |  +--2 (content_version)
//    |      \...
//    |
//    +--11 (format_version)
//       |
//       +--2 (content_version)
//           \...
//
class IndexedRulesetLocator {
 public:
  // Returns a path to a directory under |base_dir| where files corresponding to
  // the given |version| should be stored.
  static base::FilePath GetSubdirectoryPathForVersion(
      const base::FilePath& base_dir,
      const IndexedRulesetVersion& version);

  static base::FilePath GetRulesetDataFilePath(
      const base::FilePath& version_directory);
  static base::FilePath GetLicenseFilePath(
      const base::FilePath& version_directory);
  static base::FilePath GetSentinelFilePath(
      const base::FilePath& version_directory);

  // Cleans up the |indexed_ruleset_base_dir| by deleting all obsoleted ruleset
  // versions, keeping only:
  //  -- the |most_recent_version|, if it is valid,
  //  -- versions of the current format that have a sentinel file present.
  // To be called on the |background_task_runner_|.
  static void DeleteObsoleteRulesets(
      const base::FilePath& indexed_ruleset_base_dir,
      const IndexedRulesetVersion& most_recent_version);
};

// Responsible for indexing subresource filtering rules that are downloaded
// through the component updater; for versioned storage of the indexed ruleset;
// and for supplying the most up-to-date version of the indexed ruleset to the
// RulesetServiceDelegate, provided in the constructor, that abstracts away
// distribution of the ruleset to renderers.
//
// Files corresponding to each version of the indexed ruleset are stored in a
// separate subdirectory inside |indexed_ruleset_base_dir| named after the
// version. The version information of the most recent successfully stored
// ruleset is written into |local_state|. The invariant is maintained that the
// version pointed to by preferences, if valid, will exist on disk at any point
// in time.
//
// Obsolete files deletion and rulesets indexing are posted to
// |background_task_runner|.
class RulesetService : public base::SupportsWeakPtr<RulesetService> {
 public:
  // Enumerates the possible outcomes of indexing a ruleset and writing it to
  // disk. Used in UMA histograms, so the order of enumerators should not be
  // changed.
  enum class IndexAndWriteRulesetResult {
    SUCCESS,
    FAILED_CREATING_SCRATCH_DIR,
    FAILED_WRITING_RULESET_DATA,
    FAILED_WRITING_LICENSE,
    FAILED_REPLACE_FILE,
    FAILED_DELETE_PREEXISTING,
    FAILED_OPENING_UNINDEXED_RULESET,
    FAILED_PARSING_UNINDEXED_RULESET,
    FAILED_CREATING_VERSION_DIR,
    FAILED_CREATING_SENTINEL_FILE,
    FAILED_DELETING_SENTINEL_FILE,
    ABORTED_BECAUSE_SENTINEL_FILE_PRESENT,

    // Insert new values before this line.
    MAX,
  };

  // Creates a new instance that will immediately publish the most recently
  // indexed version of the ruleset if one is available according to prefs.
  // See class comments for details of arguments.
  RulesetService(
      PrefService* local_state,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner,
      RulesetServiceDelegate* delegate,
      const base::FilePath& indexed_ruleset_base_dir);
  virtual ~RulesetService();

  // Indexes, stores, and publishes the given unindexed ruleset, unless its
  // |content_version| matches that of the most recently indexed version, in
  // which case it does nothing. The files comprising the unindexed ruleset
  // need to remain accessible even after the method returns.
  //
  // Computation-heavy steps and I/O are performed on a background thread.
  // Furthermore, to prevent start-up congestion, new rulesets provided via this
  // method will not be processed until after start-up.
  //
  // Virtual so that it can be mocked out in tests.
  virtual void IndexAndStoreAndPublishRulesetIfNeeded(
      const UnindexedRulesetInfo& unindexed_ruleset_info);

  void set_is_after_startup_for_testing() { is_after_startup_ = true; }

 private:
  friend class SubresourceFilteringRulesetServiceTest;
  FRIEND_TEST_ALL_PREFIXES(SubresourceFilterContentRulesetServiceTest,
                           PublishesRulesetInOnePostTask);
  FRIEND_TEST_ALL_PREFIXES(SubresourceFilteringRulesetServiceTest,
                           NewRuleset_WriteFailure);
  FRIEND_TEST_ALL_PREFIXES(SubresourceFilteringRulesetServiceDeathTest,
                           NewRuleset_IndexingCrash);

  using WriteRulesetCallback =
      base::Callback<void(const IndexedRulesetVersion&)>;

  // Reads the ruleset described in |unindexed_ruleset_info|, indexes it, and
  // calls WriteRuleset() to persist the indexed ruleset. Returns the resulting
  // indexed ruleset version, or an invalid version on error. To be called on
  // the |background_task_runner|.
  static IndexedRulesetVersion IndexAndWriteRuleset(
      const base::FilePath& indexed_ruleset_base_dir,
      const UnindexedRulesetInfo& unindexed_ruleset_info);

  // Reads the rules from the |unindexed_ruleset_file|, and indexes them using
  // |indexer|. Returns whether the entire ruleset could be parsed.
  static bool IndexRuleset(base::File unindexed_ruleset_file,
                           RulesetIndexer* indexer);

  // Writes all files comprising the given |indexed_version| of the ruleset
  // into the corresponding subdirectory in |indexed_ruleset_base_dir|.
  // More specifically, it writes:
  //  -- the |indexed_ruleset_data| of the given |indexed_ruleset_size|,
  //  -- a copy of the LICENSE file at |license_path|, if exists.
  // Returns true on success. To be called on the |background_task_runner|.
  // Attempts not to leave an incomplete copy in the target directory.
  //
  // Writing is factored out into this separate function so it can be
  // independently exercised in tests.
  static IndexAndWriteRulesetResult WriteRuleset(
      const base::FilePath& indexed_ruleset_version_dir,
      const base::FilePath& license_source_path,
      const uint8_t* indexed_ruleset_data,
      size_t indexed_ruleset_size);

  // Indirections for accessing these routines, so as to allow overriding and
  // injecting faults in tests.
  static decltype(&IndexRuleset) g_index_ruleset_func;
  static decltype(&base::ReplaceFile) g_replace_file_func;

  // Performs indexing of the queued unindexed ruleset (if any) after start-up.
  void InitializeAfterStartup();

  // Posts a task to the |background_task_runner| to index and persist the
  // given unindexed ruleset. Then, on success, updates the most recently
  // indexed version in preferences and invokes |success_callback| on the
  // calling thread. There is no callback on failure.
  void IndexAndStoreRuleset(const UnindexedRulesetInfo& unindexed_ruleset_info,
                            const WriteRulesetCallback& success_callback);

  void OnWrittenRuleset(const WriteRulesetCallback& result_callback,
                        const IndexedRulesetVersion& version);

  void OpenAndPublishRuleset(const IndexedRulesetVersion& version);
  void OnRulesetSet(base::File file);

  PrefService* const local_state_;

  // Obsolete files deletion and indexing should be done on this runner.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Must outlive |this| object.
  RulesetServiceDelegate* delegate_;

  UnindexedRulesetInfo queued_unindexed_ruleset_info_;
  bool is_after_startup_;

  const base::FilePath indexed_ruleset_base_dir_;

  DISALLOW_COPY_AND_ASSIGN(RulesetService);
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_BROWSER_RULESET_SERVICE_H_
