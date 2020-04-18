// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_H_
#define EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_H_

#include <set>

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/version.h"
#include "extensions/browser/computed_hashes.h"
#include "extensions/browser/content_verifier/content_verifier_key.h"
#include "extensions/browser/verified_contents.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_id.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace extensions {

// Represents content verification hashes for an extension.
//
// Instances can be created using Create() factory method on sequences with
// blocking IO access. If hash retrieval succeeds then ContentHash::succeeded()
// will return true and
// a. ContentHash::verified_contents() will return structured representation of
//    verified_contents.json
// b. ContentHash::computed_hashes() will return structured representation of
//    computed_hashes.json.
//
// If verified_contents.json was missing on disk (e.g. because of disk
// corruption or such), this class will fetch the file from network. After
// fetching the class will parse/validate this data as needed, including
// calculating expected hashes for each block of each file within an extension.
// (These unsigned leaf node block level hashes will always be checked at time
// of use use to make sure they match the signed treehash root hash).
//
// computed_hashes.json is computed over the files in an extension's directory.
// If computed_hashes.json was required to be written to disk and
// it was successful, ContentHash::hash_mismatch_unix_paths() will return all
// FilePaths from the extension directory that had content verification
// mismatch.
//
// Clients of this class can cancel the disk write operation of
// computed_hashes.json while it is ongoing. This is because it can potentially
// take long time. This cancellation can be performed through |is_cancelled|.
class ContentHash : public base::RefCountedThreadSafe<ContentHash> {
 public:
  // Key to identify an extension.
  struct ExtensionKey {
    ExtensionId extension_id;
    base::FilePath extension_root;
    base::Version extension_version;
    // The key used to validate verified_contents.json.
    ContentVerifierKey verifier_key;

    ExtensionKey(const ExtensionId& extension_id,
                 const base::FilePath& extension_root,
                 const base::Version& extension_version,
                 ContentVerifierKey verifier_key);
    ~ExtensionKey();

    ExtensionKey(const ExtensionKey& other);
    ExtensionKey& operator=(const ExtensionKey& other);
  };

  // Parameters to fetch verified_contents.json.
  struct FetchParams {
    net::URLRequestContextGetter* request_context;
    GURL fetch_url;

    FetchParams(net::URLRequestContextGetter* request_context,
                const GURL& fetch_url);

    FetchParams(const FetchParams& other);
    FetchParams& operator=(const FetchParams& other);
  };

  using IsCancelledCallback = base::RepeatingCallback<bool(void)>;

  // Factory:
  // Returns ContentHash through |created_callback|, the returned values are:
  //   - |hash| The content hash. This will never be nullptr, but
  //     verified_contents or computed_hashes may be empty if something fails.
  //   - |was_cancelled| Indicates whether or not the request was cancelled
  //     through |is_cancelled|, while it was being processed.
  using CreatedCallback =
      base::OnceCallback<void(const scoped_refptr<ContentHash>& hash,
                              bool was_cancelled)>;
  static void Create(const ExtensionKey& key,
                     const FetchParams& fetch_params,
                     const IsCancelledCallback& is_cancelled,
                     CreatedCallback created_callback);

  // Forces creation of computed_hashes.json. Must be called with after
  // |verified_contents| has been successfully set.
  // TODO(lazyboy): Remove this once https://crbug.com/819832 is fixed.
  void ForceBuildComputedHashes(const IsCancelledCallback& is_cancelled,
                                CreatedCallback created_callback);

  const VerifiedContents& verified_contents() const;
  const ComputedHashes::Reader& computed_hashes() const;

  bool has_verified_contents() const {
    return status_ >= Status::kHasVerifiedContents;
  }
  bool succeeded() const { return status_ >= Status::kSucceeded; }

  // If ContentHash creation writes computed_hashes.json, then this returns the
  // FilePaths whose content hash didn't match expected hashes.
  const std::set<base::FilePath>& hash_mismatch_unix_paths() const {
    return hash_mismatch_unix_paths_;
  }
  const ExtensionKey extension_key() const { return key_; }

  // Returns whether or not computed_hashes.json re-creation might be required
  // for |this| to succeed.
  // TODO(lazyboy): Remove this once https://crbug.com/819832 is fixed.
  bool might_require_computed_hashes_force_creation() const {
    return !succeeded() && has_verified_contents() &&
           !did_attempt_creating_computed_hashes_;
  }

 private:
  friend class base::RefCountedThreadSafe<ContentHash>;

  enum class Status {
    // Retrieving hashes failed.
    kInvalid,
    // Retrieved valid verified_contents.json, but there was no
    // computed_hashes.json.
    kHasVerifiedContents,
    // Both verified_contents.json and computed_hashes.json were read
    // correctly.
    kSucceeded,
  };

  ContentHash(const ExtensionKey& key,
              std::unique_ptr<VerifiedContents> verified_contents,
              std::unique_ptr<ComputedHashes::Reader> computed_hashes);
  ~ContentHash();

  static void FetchVerifiedContents(const ExtensionKey& extension_key,
                                    const FetchParams& fetch_params,
                                    const IsCancelledCallback& is_cancelled,
                                    CreatedCallback created_callback);
  static void DidFetchVerifiedContents(
      CreatedCallback created_callback,
      const IsCancelledCallback& is_cancelled,
      const ExtensionKey& key,
      const FetchParams& fetch_params,
      std::unique_ptr<std::string> fetched_contents);

  static void DispatchFetchFailure(const ExtensionKey& key,
                                   CreatedCallback created_callback,
                                   const IsCancelledCallback& is_cancelled);

  // Computes hashes for all files in |key_.extension_root|, and uses
  // a ComputedHashes::Writer to write that information into |hashes_file|.
  // Returns true on success.
  // The verified contents file from the webstore only contains the treehash
  // root hash, but for performance we want to cache the individual block level
  // hashes. This function will create that cache with block-level hashes for
  // each file in the extension if needed (the treehash root hash for each of
  // these should equal what is in the verified contents file from the
  // webstore).
  bool CreateHashes(const base::FilePath& hashes_file,
                    const IsCancelledCallback& is_cancelled);

  // Builds computed_hashes. Possibly after creating computed_hashes.json file
  // if necessary.
  void BuildComputedHashes(bool attempted_fetching_verified_contents,
                           bool force_build,
                           const IsCancelledCallback& is_cancelled);

  ExtensionKey key_;

  Status status_ = Status::kInvalid;

  bool did_attempt_creating_computed_hashes_ = false;

  // TODO(lazyboy): Avoid dynamic allocations here, |this| already supports
  // move.
  std::unique_ptr<VerifiedContents> verified_contents_;
  std::unique_ptr<ComputedHashes::Reader> computed_hashes_;

  // Paths that were found to have a mismatching hash.
  std::set<base::FilePath> hash_mismatch_unix_paths_;

  // The block size to use for hashing.
  // TODO(asargent) - use the value from verified_contents.json for each
  // file, instead of using a constant.
  int block_size_ = extension_misc::kContentVerificationDefaultBlockSize;

  DISALLOW_COPY_AND_ASSIGN(ContentHash);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_H_
