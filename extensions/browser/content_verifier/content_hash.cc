// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_verifier/content_hash.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequence_checker.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/elapsed_timer.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/sha2.h"
#include "extensions/browser/content_hash_fetcher.h"
#include "extensions/browser/content_hash_tree.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/common/file_util.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace extensions {

namespace {

using SortedFilePathSet = std::set<base::FilePath>;

bool CreateDirAndWriteFile(const base::FilePath& destination,
                           const std::string& content) {
  base::AssertBlockingAllowed();
  DCHECK(GetExtensionFileTaskRunner()->RunsTasksInCurrentSequence());
  base::FilePath dir = destination.DirName();
  if (!base::CreateDirectory(dir))
    return false;

  int write_result =
      base::WriteFile(destination, content.data(), content.size());
  return write_result >= 0 &&
         base::checked_cast<size_t>(write_result) == content.size();
}

std::unique_ptr<VerifiedContents> GetVerifiedContents(
    const ContentHash::ExtensionKey& key,
    bool delete_invalid_file) {
  base::AssertBlockingAllowed();
  DCHECK(GetExtensionFileTaskRunner()->RunsTasksInCurrentSequence());
  auto verified_contents = std::make_unique<VerifiedContents>(key.verifier_key);
  base::FilePath verified_contents_path =
      file_util::GetVerifiedContentsPath(key.extension_root);
  if (!verified_contents->InitFrom(verified_contents_path)) {
    if (delete_invalid_file &&
        !base::DeleteFile(verified_contents_path, false)) {
      LOG(WARNING) << "Failed to delete " << verified_contents_path.value();
    }
    return nullptr;
  }
  return verified_contents;
}

}  // namespace

ContentHash::ExtensionKey::ExtensionKey(const ExtensionId& extension_id,
                                        const base::FilePath& extension_root,
                                        const base::Version& extension_version,
                                        ContentVerifierKey verifier_key)
    : extension_id(extension_id),
      extension_root(extension_root),
      extension_version(extension_version),
      verifier_key(verifier_key) {}

ContentHash::ExtensionKey::~ExtensionKey() = default;

ContentHash::ExtensionKey::ExtensionKey(
    const ContentHash::ExtensionKey& other) = default;

ContentHash::ExtensionKey& ContentHash::ExtensionKey::operator=(
    const ContentHash::ExtensionKey& other) = default;

ContentHash::FetchParams::FetchParams(
    net::URLRequestContextGetter* request_context,
    const GURL& fetch_url)
    : request_context(request_context), fetch_url(fetch_url) {}

ContentHash::FetchParams::FetchParams(const FetchParams& other) = default;
ContentHash::FetchParams& ContentHash::FetchParams::operator=(
    const FetchParams& other) = default;

// static
void ContentHash::Create(const ExtensionKey& key,
                         const FetchParams& fetch_params,
                         const IsCancelledCallback& is_cancelled,
                         CreatedCallback created_callback) {
  base::AssertBlockingAllowed();
  // Step 1/2: verified_contents.json:
  std::unique_ptr<VerifiedContents> verified_contents = GetVerifiedContents(
      key,
      // If verified_contents.json exists on disk but is invalid, we want to
      // delete it so we can fetch and write the file later if required.
      true /* delete_invalid_file */);

  if (!verified_contents) {
    // Fetch verified_contents.json and then respond.
    FetchVerifiedContents(key, fetch_params, is_cancelled,
                          std::move(created_callback));
    return;
  }

  // Step 2/2: computed_hashes.json:
  scoped_refptr<ContentHash> hash =
      new ContentHash(key, std::move(verified_contents), nullptr);
  const bool did_fetch_verified_contents = false;
  hash->BuildComputedHashes(did_fetch_verified_contents,
                            false /* force_build */, is_cancelled);
  std::move(created_callback).Run(hash, is_cancelled && is_cancelled.Run());
}

void ContentHash::ForceBuildComputedHashes(
    const IsCancelledCallback& is_cancelled,
    CreatedCallback created_callback) {
  BuildComputedHashes(false /* did_fetch_verified_contents */,
                      true /* force_build */, is_cancelled);
  std::move(created_callback).Run(this, is_cancelled && is_cancelled.Run());
}

const VerifiedContents& ContentHash::verified_contents() const {
  DCHECK(status_ >= Status::kHasVerifiedContents && verified_contents_);
  return *verified_contents_;
}

const ComputedHashes::Reader& ContentHash::computed_hashes() const {
  DCHECK(status_ == Status::kSucceeded && computed_hashes_);
  return *computed_hashes_;
}

ContentHash::ContentHash(
    const ExtensionKey& key,
    std::unique_ptr<VerifiedContents> verified_contents,
    std::unique_ptr<ComputedHashes::Reader> computed_hashes)
    : key_(key),
      verified_contents_(std::move(verified_contents)),
      computed_hashes_(std::move(computed_hashes)) {
  if (!verified_contents_)
    status_ = Status::kInvalid;
  else if (!computed_hashes_)
    status_ = Status::kHasVerifiedContents;
  else
    status_ = Status::kSucceeded;
}

ContentHash::~ContentHash() = default;

// static
void ContentHash::FetchVerifiedContents(
    const ContentHash::ExtensionKey& extension_key,
    const ContentHash::FetchParams& fetch_params,
    const ContentHash::IsCancelledCallback& is_cancelled,
    ContentHash::CreatedCallback created_callback) {
  // |fetcher| deletes itself when it's done.
  internals::ContentHashFetcher* fetcher =
      new internals::ContentHashFetcher(extension_key, fetch_params);
  fetcher->Start(base::BindOnce(&ContentHash::DidFetchVerifiedContents,
                                std::move(created_callback), is_cancelled));
}

// static
void ContentHash::DidFetchVerifiedContents(
    ContentHash::CreatedCallback created_callback,
    const ContentHash::IsCancelledCallback& is_cancelled,
    const ContentHash::ExtensionKey& key,
    const ContentHash::FetchParams& fetch_params,
    std::unique_ptr<std::string> fetched_contents) {
  base::AssertBlockingAllowed();
  if (!fetched_contents) {
    ContentHash::DispatchFetchFailure(key, std::move(created_callback),
                                      is_cancelled);
    return;
  }

  // Write file and continue reading hash.
  // Write:
  // Parse the response to make sure it is valid json (on staging sometimes it
  // can be a login redirect html, xml file, etc. if you aren't logged in with
  // the right cookies).  TODO(asargent) - It would be a nice enhancement to
  // move to parsing this in a sandboxed helper (https://crbug.com/372878).
  std::unique_ptr<base::Value> parsed =
      base::JSONReader::Read(*fetched_contents);
  if (!parsed) {
    ContentHash::DispatchFetchFailure(key, std::move(created_callback),
                                      is_cancelled);
    return;
  }

  VLOG(1) << "JSON parsed ok for " << key.extension_id;
  parsed.reset();  // no longer needed

  base::FilePath destination =
      file_util::GetVerifiedContentsPath(key.extension_root);
  if (!CreateDirAndWriteFile(destination, *fetched_contents)) {
    LOG(ERROR) << "Error writing computed_hashes.json at " << destination;
    ContentHash::DispatchFetchFailure(key, std::move(created_callback),
                                      is_cancelled);
    return;
  }

  // Continue reading hash.
  std::unique_ptr<VerifiedContents> verified_contents = GetVerifiedContents(
      key,
      // We've just fetched verified_contents.json, so treat it as a read-only
      // file from now on and do not delete the file even if it turns out to be
      // invalid.
      false /* delete_invalid_file */);

  if (!verified_contents) {
    ContentHash::DispatchFetchFailure(key, std::move(created_callback),
                                      is_cancelled);
    return;
  }

  scoped_refptr<ContentHash> hash =
      new ContentHash(key, std::move(verified_contents), nullptr);
  const bool did_fetch_verified_contents = true;
  hash->BuildComputedHashes(did_fetch_verified_contents,
                            false /* force_build */, is_cancelled);
  std::move(created_callback).Run(hash, is_cancelled && is_cancelled.Run());
}

// static
void ContentHash::DispatchFetchFailure(
    const ExtensionKey& key,
    CreatedCallback created_callback,
    const IsCancelledCallback& is_cancelled) {
  // NOTE: bare new because ContentHash constructor is private.
  scoped_refptr<ContentHash> content_hash =
      new ContentHash(key, nullptr, nullptr);
  std::move(created_callback)
      .Run(content_hash, is_cancelled && is_cancelled.Run());
}

bool ContentHash::CreateHashes(const base::FilePath& hashes_file,
                               const IsCancelledCallback& is_cancelled) {
  base::ElapsedTimer timer;
  did_attempt_creating_computed_hashes_ = true;
  // Make sure the directory exists.
  if (!base::CreateDirectoryAndGetError(hashes_file.DirName(), nullptr))
    return false;

  base::FileEnumerator enumerator(key_.extension_root, true, /* recursive */
                                  base::FileEnumerator::FILES);
  // First discover all the file paths and put them in a sorted set.
  SortedFilePathSet paths;
  for (;;) {
    if (is_cancelled && is_cancelled.Run())
      return false;

    base::FilePath full_path = enumerator.Next();
    if (full_path.empty())
      break;
    paths.insert(full_path);
  }

  // Now iterate over all the paths in sorted order and compute the block hashes
  // for each one.
  ComputedHashes::Writer writer;
  for (SortedFilePathSet::iterator i = paths.begin(); i != paths.end(); ++i) {
    if (is_cancelled && is_cancelled.Run())
      return false;

    const base::FilePath& full_path = *i;
    base::FilePath relative_unix_path;
    key_.extension_root.AppendRelativePath(full_path, &relative_unix_path);
    relative_unix_path = relative_unix_path.NormalizePathSeparatorsTo('/');

    if (!verified_contents_->HasTreeHashRoot(relative_unix_path))
      continue;

    std::string contents;
    if (!base::ReadFileToString(full_path, &contents)) {
      LOG(ERROR) << "Could not read " << full_path.MaybeAsASCII();
      continue;
    }

    // Iterate through taking the hash of each block of size (block_size_) of
    // the file.
    std::vector<std::string> hashes;
    ComputedHashes::ComputeHashesForContent(contents, block_size_, &hashes);
    std::string root =
        ComputeTreeHashRoot(hashes, block_size_ / crypto::kSHA256Length);
    if (!verified_contents_->TreeHashRootEquals(relative_unix_path, root)) {
      VLOG(1) << "content mismatch for " << relative_unix_path.AsUTF8Unsafe();
      hash_mismatch_unix_paths_.insert(relative_unix_path);
      continue;
    }

    writer.AddHashes(relative_unix_path, block_size_, hashes);
  }
  bool result = writer.WriteToFile(hashes_file);
  UMA_HISTOGRAM_TIMES("ExtensionContentHashFetcher.CreateHashesTime",
                      timer.Elapsed());

  if (result)
    status_ = Status::kSucceeded;

  return result;
}

void ContentHash::BuildComputedHashes(bool attempted_fetching_verified_contents,
                                      bool force_build,
                                      const IsCancelledCallback& is_cancelled) {
  base::FilePath computed_hashes_path =
      file_util::GetComputedHashesPath(key_.extension_root);

  // Create computed_hashes.json file if any of the following is true:
  // - We just fetched and wrote a verified_contents.json (i.e.
  //   |attempted_fetching_verified_contents| = true).
  // - computed_hashes.json is missing.
  // - existing computed_hashes.json cannot be initialized correctly (probably
  //   due to disk corruption).
  //  bool will_create = attempted_fetching_verified_contents ||
  //                     !base::PathExists(computed_hashes_path);

  // However, existing behavior is to create computed_hashes.json only when
  // ContentVerifyJob's request to computed_hashes.json fails.
  // TODO(lazyboy): Fix this and use |will_create| condition from the comment
  // above, see https://crbug.com/819832 for details.
  bool will_create = force_build || !base::PathExists(computed_hashes_path);
  if (!will_create) {
    // Note: Tolerate for existing implementation.
    // Try to read and initialize the file first. On failure, continue creating.
    auto computed_hashes = std::make_unique<ComputedHashes::Reader>();
    if (!computed_hashes->InitFromFile(computed_hashes_path)) {
      // TODO(lazyboy): Also create computed_hashes.json in this case. See the
      // comment above about |will_create|.
      // will_create = true;
    } else {
      // Read successful.
      status_ = Status::kSucceeded;
      computed_hashes_ = std::move(computed_hashes);
      return;
    }
  }

  if (will_create && !CreateHashes(computed_hashes_path, is_cancelled)) {
    // Failed creating computed_hashes.json.
    return;
  }

  // Read computed_hashes.json:
  if (!base::PathExists(computed_hashes_path))
    return;

  auto computed_hashes = std::make_unique<ComputedHashes::Reader>();
  if (!computed_hashes->InitFromFile(computed_hashes_path))
    return;

  // Read successful.
  status_ = Status::kSucceeded;
  computed_hashes_ = std::move(computed_hashes);
}

}  // namespace extensions
