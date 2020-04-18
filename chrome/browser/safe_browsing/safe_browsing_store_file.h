// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_STORE_FILE_H_
#define CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_STORE_FILE_H_

#include <stdint.h>

#include <set>
#include <vector>

#include "chrome/browser/safe_browsing/safe_browsing_store.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"

namespace safe_browsing {

// Implement SafeBrowsingStore in terms of a flat file.  The file
// format is pretty literal:
//
// int32_t magic;             // magic number "validating" file
// int32_t version;           // format version
//
// // Counts for the various data which follows the header.
// uint32_t add_chunk_count;  // Chunks seen, including empties.
// uint32_t sub_chunk_count;  // Ditto.
// uint32_t shard_stride;     // SBPrefix space covered per shard.
//                          // 0==entire space in one shard.
// // Sorted by chunk_id.
// array[add_chunk_count] {
//   int32_t chunk_id;
// }
// // Sorted by chunk_id.
// array[sub_chunk_count] {
//   int32_t chunk_id;
// }
// MD5Digest header_checksum;  // Checksum over preceeding data.
//
// // Sorted by prefix, then add chunk_id, then hash, both within shards and
// // overall.
// array[from 0 to wraparound to 0 by shard_stride] {
//   uint32_t add_prefix_count;
//   uint32_t sub_prefix_count;
//   uint32_t add_hash_count;
//   uint32_t sub_hash_count;
//   array[add_prefix_count] {
//     int32_t chunk_id;
//     uint32_t prefix;
//   }
//   array[sub_prefix_count] {
//     int32_t chunk_id;
//     int32_t add_chunk_id;
//     uint32_t add_prefix;
//   }
//   array[add_hash_count] {
//     int32_t chunk_id;
//     int32_t received_time;     // From base::Time::ToTimeT().
//     char[32] full_hash;
//   }
//   array[sub_hash_count] {
//     int32_t chunk_id;
//     int32_t add_chunk_id;
//     char[32] add_full_hash;
//   }
// }
// MD5Digest checksum;      // Checksum over entire file.
//
// The checksums are used to allow writing the file without doing an expensive
// fsync().  Since the data can be re-fetched, failing the checksum is not
// catastrophic.  Histograms indicate that file corruption here is pretty
// uncommon.
//
// The |header_checksum| is present to guarantee valid header and chunk data for
// updates.  Only that part of the file needs to be read to post the update.
//
// |shard_stride| breaks the file into approximately-equal portions, allowing
// updates to stream from one file to another with modest memory usage.  It is
// dynamic to adjust to different file sizes without adding excessive overhead.
//
// During the course of an update, uncommitted data is stored in a
// temporary file (which is later re-used to commit).  This is an
// array of chunks, with the count kept in memory until the end of the
// transaction.  The format of this file is like the main file, with
// the list of chunks seen omitted, as that data is tracked in-memory:
//
// array[] {
//   uint32_t add_prefix_count;
//   uint32_t sub_prefix_count;
//   uint32_t add_hash_count;
//   uint32_t sub_hash_count;
//   array[add_prefix_count] {
//     int32_t chunk_id;
//     uint32_t prefix;
//   }
//   array[sub_prefix_count] {
//     int32_t chunk_id;
//     int32_t add_chunk_id;
//     uint32_t add_prefix;
//   }
//   array[add_hash_count] {
//     int32_t chunk_id;
//     int32_t received_time;     // From base::Time::ToTimeT().
//     char[32] full_hash;
//   }
//   array[sub_hash_count] {
//     int32_t chunk_id;
//     int32_t add_chunk_id;
//     char[32] add_full_hash;
//   }
// }
//
// The overall transaction works like this:
// - Open the original file to get the chunks-seen data.
// - Open a temp file for storing new chunk info.
// - Write new chunks to the temp file.
// - When the transaction is finished:
//   - Read the update data from the temp file into memory.
//   - Overwrite the temp file with new header data.
//   - Until done:
//     - Read shards of the original file's data into memory.
//     - Merge from the update data.
//     - Write shards to the temp file.
//   - Delete original file.
//   - Rename temp file to original filename.

class SafeBrowsingStoreFile : public SafeBrowsingStore {
 public:
  explicit SafeBrowsingStoreFile(
      const scoped_refptr<const base::SequencedTaskRunner>& task_runner);
  ~SafeBrowsingStoreFile() override;

  void Init(const base::FilePath& filename,
            const base::Closure& corruption_callback) override;

  // Delete any on-disk files, including the permanent storage.
  bool Delete() override;

  // Get all add hash prefixes and full-length hashes, respectively, from
  // the store.
  bool GetAddPrefixes(SBAddPrefixes* add_prefixes) override;
  bool GetAddFullHashes(std::vector<SBAddFullHash>* add_full_hashes) override;

  bool BeginChunk() override;

  bool WriteAddPrefix(int32_t chunk_id, SBPrefix prefix) override;
  bool WriteAddHash(int32_t chunk_id, const SBFullHash& full_hash) override;
  bool WriteSubPrefix(int32_t chunk_id,
                      int32_t add_chunk_id,
                      SBPrefix prefix) override;
  bool WriteSubHash(int32_t chunk_id,
                    int32_t add_chunk_id,
                    const SBFullHash& full_hash) override;
  bool FinishChunk() override;

  bool BeginUpdate() override;
  bool FinishUpdate(
      PrefixSetBuilder* builder,
      std::vector<SBAddFullHash>* add_full_hashes_result) override;
  bool CancelUpdate() override;

  void SetAddChunk(int32_t chunk_id) override;
  bool CheckAddChunk(int32_t chunk_id) override;
  void GetAddChunks(std::vector<int32_t>* out) override;
  void SetSubChunk(int32_t chunk_id) override;
  bool CheckSubChunk(int32_t chunk_id) override;
  void GetSubChunks(std::vector<int32_t>* out) override;

  void DeleteAddChunk(int32_t chunk_id) override;
  void DeleteSubChunk(int32_t chunk_id) override;

  // Verify |file_|'s checksum, calling the corruption callback if it
  // does not check out.  Empty input is considered valid.
  bool CheckValidity() override;

  // Returns the name of the temporary file used to buffer data for
  // |filename|.  Exported for unit tests.
  static const base::FilePath TemporaryFileForFilename(
      const base::FilePath& filename) {
    return base::FilePath(filename.value() + FILE_PATH_LITERAL("_new"));
  }

  // Delete any on-disk files, including the permanent storage.
  static bool DeleteStore(const base::FilePath& basename);

 private:
  // Checks whether the current thread is part of the sequenced task runner
  // this object was initialized with.
  bool CalledOnValidThread();

  // Does the actual update for FinishUpdate(), so that FinishUpdate() can clean
  // up correctly in case of error.
  virtual bool DoUpdate(PrefixSetBuilder* builder,
                        std::vector<SBAddFullHash>* add_full_hashes_result);

  // Some very lucky users have an original-format file still in their
  // profile.  Check for it and delete, recording a histogram for the
  // result (no histogram for not-found).  Logically this
  // would make more sense at the SafeBrowsingDatabase level, but
  // practically speaking that code doesn't touch files directly.
  static void CheckForOriginalAndDelete(const base::FilePath& filename);

  // Close all files and clear all buffers.
  bool Close();

  // Calls |corruption_callback_| if non-NULL, always returns false as
  // a convenience to the caller.
  bool OnCorruptDatabase();

  // Helper for creating a corruption callback for |old_store_|.
  // TODO(shess): Remove after migration.
  void HandleCorruptDatabase();

  // Clear temporary buffers used to accumulate chunk data.
  bool ClearChunkBuffers() {
    // NOTE: .clear() doesn't release memory.
    // TODO(shess): Figure out if this is overkill.  Some amount of
    // pre-reserved space is probably reasonable between each chunk
    // collected.
    SBAddPrefixes().swap(add_prefixes_);
    SBSubPrefixes().swap(sub_prefixes_);
    std::vector<SBAddFullHash>().swap(add_hashes_);
    std::vector<SBSubFullHash>().swap(sub_hashes_);
    return true;
  }

  // Clear all buffers used during update.
  void ClearUpdateBuffers() {
    ClearChunkBuffers();
    chunks_written_ = 0;
    std::set<int32_t>().swap(add_chunks_cache_);
    std::set<int32_t>().swap(sub_chunks_cache_);
    base::hash_set<int32_t>().swap(add_del_cache_);
    base::hash_set<int32_t>().swap(sub_del_cache_);
  }

  // The sequenced task runner for this object, used to verify that its state
  // is only ever accessed from the runner.
  scoped_refptr<const base::SequencedTaskRunner> task_runner_;

  // Buffers for collecting data between BeginChunk() and
  // FinishChunk().
  SBAddPrefixes add_prefixes_;
  SBSubPrefixes sub_prefixes_;
  std::vector<SBAddFullHash> add_hashes_;
  std::vector<SBSubFullHash> sub_hashes_;

  // Count of chunks collected in |new_file_|.
  int chunks_written_;

  // Name of the main database file.
  base::FilePath filename_;

  // Handles to the main and scratch files.  |empty_| is true if the
  // main file didn't exist when the update was started.
  base::ScopedFILE file_;
  base::ScopedFILE new_file_;
  bool empty_;

  // Cache of chunks which have been seen.  Loaded from the database
  // on BeginUpdate() so that it can be queried during the
  // transaction.
  std::set<int32_t> add_chunks_cache_;
  std::set<int32_t> sub_chunks_cache_;

  // Cache the set of deleted chunks during a transaction, applied on
  // FinishUpdate().
  // TODO(shess): If the set is small enough, hash_set<> might be
  // slower than plain set<>.
  base::hash_set<int32_t> add_del_cache_;
  base::hash_set<int32_t> sub_del_cache_;

  base::Closure corruption_callback_;

  // Tracks whether corruption has already been seen in the current
  // update, so that only one instance is recorded in the stats.
  // TODO(shess): Remove with format-migration support.
  bool corruption_seen_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingStoreFile);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_STORE_FILE_H_
