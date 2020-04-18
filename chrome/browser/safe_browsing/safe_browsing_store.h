// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_STORE_H_
#define CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_STORE_H_

#include <stdint.h>

#include <set>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/circular_deque.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "components/safe_browsing/db/util.h"

namespace base {
class FilePath;
}

namespace safe_browsing {

class PrefixSetBuilder;

// SafeBrowsingStore provides a storage abstraction for the
// safe-browsing data used to build the bloom filter.  The items
// stored are:
//   The set of add and sub chunks seen.
//   List of SBAddPrefix (chunk_id and SBPrefix).
//   List of SBSubPrefix (chunk_id and the target SBAddPrefix).
//   List of SBAddFullHash (SBAddPrefix, time received and an SBFullHash).
//   List of SBSubFullHash (chunk_id, target SBAddPrefix, and an SBFullHash).
//
// The store is geared towards updating the data, not runtime access
// to the data (that is handled by SafeBrowsingDatabase).  Updates are
// handled similar to a SQL transaction cycle, with the new data being
// returned from FinishUpdate() (the COMMIT).  Data is not persistent
// until FinishUpdate() returns successfully.
//
// FinishUpdate() also handles dropping items who's chunk has been
// deleted, and netting out the add/sub lists (when a sub matches an
// add, both are dropped).

// GetAddChunkId(), GetAddPrefix() and GetFullHash() are exposed so
// that these items can be generically compared with each other by
// SBAddPrefixLess() and SBAddPrefixHashLess().

struct SBAddPrefix {
  int32_t chunk_id;
  SBPrefix prefix;

  SBAddPrefix(int32_t id, SBPrefix p) : chunk_id(id), prefix(p) {}
  SBAddPrefix() : chunk_id(), prefix() {}

  int32_t GetAddChunkId() const { return chunk_id; }
  SBPrefix GetAddPrefix() const { return prefix; }
};

// TODO(shess): Measure the performance impact of switching this back to
// std::vector<> once the v8 file format dominates.  Also SBSubPrefixes.
using SBAddPrefixes = base::circular_deque<SBAddPrefix>;

struct SBSubPrefix {
  int32_t chunk_id;
  int32_t add_chunk_id;
  SBPrefix add_prefix;

  SBSubPrefix(int32_t id, int32_t add_id, SBPrefix prefix)
      : chunk_id(id), add_chunk_id(add_id), add_prefix(prefix) {}
  SBSubPrefix() : chunk_id(), add_chunk_id(), add_prefix() {}

  int32_t GetAddChunkId() const { return add_chunk_id; }
  SBPrefix GetAddPrefix() const { return add_prefix; }
};

using SBSubPrefixes = base::circular_deque<SBSubPrefix>;

struct SBAddFullHash {
  int32_t chunk_id;
  // Received field is not used anymore, but is kept for DB compatability.
  // TODO(shess): Deprecate and remove.
  int32_t deprecated_received;
  SBFullHash full_hash;

  SBAddFullHash(int32_t id, const SBFullHash& h)
      : chunk_id(id), deprecated_received(), full_hash(h) {}

  SBAddFullHash() : chunk_id(), deprecated_received(), full_hash() {}

  int32_t GetAddChunkId() const { return chunk_id; }
  SBPrefix GetAddPrefix() const { return full_hash.prefix; }
};

struct SBSubFullHash {
  int32_t chunk_id;
  int32_t add_chunk_id;
  SBFullHash full_hash;

  SBSubFullHash(int32_t id, int32_t add_id, const SBFullHash& h)
      : chunk_id(id), add_chunk_id(add_id), full_hash(h) {}
  SBSubFullHash() : chunk_id(), add_chunk_id(), full_hash() {}

  int32_t GetAddChunkId() const { return add_chunk_id; }
  SBPrefix GetAddPrefix() const { return full_hash.prefix; }
};

// Determine less-than based on prefix and add chunk.
template <class T, class U>
bool SBAddPrefixLess(const T& a, const U& b) {
  if (a.GetAddPrefix() != b.GetAddPrefix())
    return a.GetAddPrefix() < b.GetAddPrefix();

  return a.GetAddChunkId() < b.GetAddChunkId();
}

// Determine less-than based on prefix, add chunk, and full hash.
// Prefix can compare differently than hash due to byte ordering,
// so it must take precedence.
template <class T, class U>
bool SBAddPrefixHashLess(const T& a, const U& b) {
  if (SBAddPrefixLess(a, b))
    return true;

  if (SBAddPrefixLess(b, a))
    return false;

  return memcmp(a.full_hash.full_hash, b.full_hash.full_hash,
                sizeof(a.full_hash.full_hash)) < 0;
}

// Process the lists for subs which knock out adds.  For any item in
// |sub_prefixes| which has a match in |add_prefixes|, knock out the
// matched items from all vectors.  Additionally remove items from
// deleted chunks.
//
// The inputs must be sorted by SBAddPrefixLess or SBAddPrefixHashLess.
void SBProcessSubs(SBAddPrefixes* add_prefixes,
                   SBSubPrefixes* sub_prefixes,
                   std::vector<SBAddFullHash>* add_full_hashes,
                   std::vector<SBSubFullHash>* sub_full_hashes,
                   const base::hash_set<int32_t>& add_chunks_deleted,
                   const base::hash_set<int32_t>& sub_chunks_deleted);

// Abstract interface for storing data.
class SafeBrowsingStore {
 public:
  SafeBrowsingStore() {}
  virtual ~SafeBrowsingStore() {}

  // Sets up the information for later use, but does not necessarily
  // check whether the underlying file exists, or is valid.  If
  // |curruption_callback| is non-NULL it will be called if corruption
  // is detected, which could happen as part of any call other than
  // Delete().  The appropriate action is to use Delete() to clear the
  // store.
  virtual void Init(const base::FilePath& filename,
                    const base::Closure& corruption_callback) = 0;

  // Deletes the files which back the store, returning true if
  // successful.
  virtual bool Delete() = 0;

  // Get all Add prefixes out from the store.
  virtual bool GetAddPrefixes(SBAddPrefixes* add_prefixes) = 0;

  // Get all add full-length hashes.
  virtual bool GetAddFullHashes(
      std::vector<SBAddFullHash>* add_full_hashes) = 0;

  // Start an update.  None of the following methods should be called
  // unless this returns true.  If this returns true, the update
  // should be terminated by FinishUpdate() or CancelUpdate().
  virtual bool BeginUpdate() = 0;

  // Start a chunk of data.  None of the methods through FinishChunk()
  // should be called unless this returns true.
  // TODO(shess): Would it make sense for this to accept |chunk_id|?
  // Possibly not, because of possible confusion between sub_chunk_id
  // and add_chunk_id.
  virtual bool BeginChunk() = 0;

  virtual bool WriteAddPrefix(int32_t chunk_id, SBPrefix prefix) = 0;
  virtual bool WriteAddHash(int32_t chunk_id, const SBFullHash& full_hash) = 0;
  virtual bool WriteSubPrefix(int32_t chunk_id,
                              int32_t add_chunk_id,
                              SBPrefix prefix) = 0;
  virtual bool WriteSubHash(int32_t chunk_id,
                            int32_t add_chunk_id,
                            const SBFullHash& full_hash) = 0;

  // Collect the chunk data and preferrably store it on disk to
  // release memory.  Shoul not modify the data in-place.
  virtual bool FinishChunk() = 0;

  // Track the chunks which have been seen.
  virtual void SetAddChunk(int32_t chunk_id) = 0;
  virtual bool CheckAddChunk(int32_t chunk_id) = 0;
  virtual void GetAddChunks(std::vector<int32_t>* out) = 0;
  virtual void SetSubChunk(int32_t chunk_id) = 0;
  virtual bool CheckSubChunk(int32_t chunk_id) = 0;
  virtual void GetSubChunks(std::vector<int32_t>* out) = 0;

  // Delete the indicated chunk_id.  The chunk will continue to be
  // visible until the end of the transaction.
  virtual void DeleteAddChunk(int32_t chunk_id) = 0;
  virtual void DeleteSubChunk(int32_t chunk_id) = 0;

  // May be called during update to verify that the storage is valid.
  // Return true if the store seems valid.  If corruption is detected,
  // calls the corruption callback and return false.
  // NOTE(shess): When storage was SQLite, there was no guarantee that
  // a structurally sound database actually contained valid data,
  // whereas SafeBrowsingStoreFile checksums the data.  For now, this
  // distinction doesn't matter.
  virtual bool CheckValidity() = 0;

  // Pass the collected chunks through SBPRocessSubs() and commit to
  // permanent storage.  The resulting add prefixes and hashes will be
  // stored in |add_prefixes_result| and |add_full_hashes_result|.
  virtual bool FinishUpdate(
      PrefixSetBuilder* builder,
      std::vector<SBAddFullHash>* add_full_hashes_result) = 0;

  // Cancel the update in process and remove any temporary disk
  // storage, leaving the original data unmodified.
  virtual bool CancelUpdate() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingStore);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_STORE_H_
