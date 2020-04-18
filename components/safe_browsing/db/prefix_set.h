// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A read-only set implementation for |SBPrefix| items.  Prefixes are
// sorted and stored as 16-bit deltas from the previous prefix.  An
// index structure provides quick random access, and also handles
// cases where 16 bits cannot encode a delta.
//
// For example, the sequence {20, 25, 41, 65432, 150000, 160000} would
// be stored as:
//  A pair {20, 0} in |index_|.
//  5, 16, 65391 in |deltas_|.
//  A pair {150000, 3} in |index_|.
//  10000 in |deltas_|.
// |index_.size()| will be 2, |deltas_.size()| will be 4.
//
// This structure is intended for storage of sparse uniform sets of
// prefixes of a certain size.  As of this writing, my safe-browsing
// database contains:
//   653132 add prefixes
//   6446 are duplicates (from different chunks)
//   24301 w/in 2^8 of the prior prefix
//   622337 w/in 2^16 of the prior prefix
//   47 further than 2^16 from the prior prefix
// For this input, the memory usage is approximately 2 bytes per
// prefix, a bit over 1.2M.  The bloom filter used 25 bits per prefix,
// a bit over 1.9M on this data.
//
// Experimenting with random selections of the above data, storage
// size drops almost linearly as prefix count drops, until the index
// overhead starts to become a problem a bit under 200k prefixes.  The
// memory footprint gets worse than storing the raw prefix data around
// 75k prefixes.  Fortunately, the actual memory footprint also falls.
// If the prefix count increases the memory footprint should increase
// approximately linearly.  The worst-case would be 2^16 items all
// 2^16 apart, which would need 512k (versus 256k to store the raw
// data).
//
// The on-disk format looks like:
//         4 byte magic number
//         4 byte version number
//         4 byte |index_.size()|
//         4 byte |deltas_.size()|
//     n * 8 byte |&index_[0]..&index_[n]|
//     m * 2 byte |&deltas_[0]..&deltas_[m]|
//        16 byte digest

#ifndef COMPONENTS_SAFE_BROWSING_DB_PREFIX_SET_H_
#define COMPONENTS_SAFE_BROWSING_DB_PREFIX_SET_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/safe_browsing/db/util.h"

namespace base {
class FilePath;
}

namespace safe_browsing {

class PrefixSet {
 public:
  ~PrefixSet();

  // |true| if |hash| is in the hashes passed to the set's builder, or if
  // |hash.prefix| is one of the prefixes passed to the set's builder.
  bool Exists(const SBFullHash& hash) const;

  // Persist the set on disk.
  static std::unique_ptr<const PrefixSet> LoadFile(
      const base::FilePath& filter_name);
  bool WriteFile(const base::FilePath& filter_name) const;

 private:
  friend class PrefixSetBuilder;

  friend class PrefixSetTest;
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, AllBig);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, EdgeCases);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, Empty);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, FullHashBuild);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, IntMinMax);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, OneElement);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, ReadWrite);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, ReadWriteSigned);
  FRIEND_TEST_ALL_PREFIXES(PrefixSetTest, Version3);

  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, BasicStore);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, DeleteChunks);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, DetectsCorruption);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, Empty);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, PrefixMinMax);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, SubKnockout);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, Version7);
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingStoreFileTest, Version8);

  // Maximum number of consecutive deltas to encode before generating
  // a new index entry.  This helps keep the worst-case performance
  // for |Exists()| under control.
  static const size_t kMaxRun = 100;

  // Helpers to make |index_| easier to deal with.
  typedef std::pair<SBPrefix, uint32_t> IndexPair;
  typedef std::vector<IndexPair> IndexVector;
  static bool PrefixLess(const IndexPair& a, const IndexPair& b);

  // Helper to let |PrefixSetBuilder| add a run of data.  |index_prefix| is
  // added to |index_|, with the other elements added into |deltas_|.
  void AddRun(SBPrefix index_prefix,
              const uint16_t* run_begin,
              const uint16_t* run_end);

  // |true| if |prefix| is one of the prefixes passed to the set's builder.
  // Provided for testing purposes.
  bool PrefixExists(SBPrefix prefix) const;

  // Regenerate the vector of prefixes passed to the constructor into
  // |prefixes|.  Prefixes will be added in sorted order.  Useful for testing.
  void GetPrefixes(std::vector<SBPrefix>* prefixes) const;

  // Used by |PrefixSetBuilder|.
  PrefixSet();

  // Helper for |LoadFile()|.  Steals vector contents using |swap()|.
  PrefixSet(IndexVector* index,
            std::vector<uint16_t>* deltas,
            std::vector<SBFullHash>* full_hashes);

  // Top-level index of prefix to offset in |deltas_|.  Each pair
  // indicates a base prefix and where the deltas from that prefix
  // begin in |deltas_|.  The deltas for a pair end at the next pair's
  // index into |deltas_|.
  IndexVector index_;

  // Deltas which are added to the prefix in |index_| to generate
  // prefixes.  Deltas are only valid between consecutive items from
  // |index_|, or the end of |deltas_| for the last |index_| pair.
  std::vector<uint16_t> deltas_;

  // Full hashes ordered by SBFullHashLess.
  std::vector<SBFullHash> full_hashes_;

  DISALLOW_COPY_AND_ASSIGN(PrefixSet);
};

// Helper to incrementally build a PrefixSet from a stream of sorted prefixes.
class PrefixSetBuilder {
 public:
  PrefixSetBuilder();
  ~PrefixSetBuilder();

  // Helper for unit tests and format conversion.
  explicit PrefixSetBuilder(const std::vector<SBPrefix>& prefixes);

  // Add a prefix to the set.  Prefixes must arrive in ascending order.
  // Duplicate prefixes are dropped.
  void AddPrefix(SBPrefix prefix);

  // Flush any buffered prefixes, and return the final PrefixSet instance.
  // |hashes| are sorted and stored in |full_hashes_|.  Any call other than the
  // destructor is illegal after this call.
  std::unique_ptr<const PrefixSet> GetPrefixSet(
      const std::vector<SBFullHash>& hashes);

  // Helper for clients which only track prefixes.  Calls GetPrefixSet() with
  // empty hash vector.
  std::unique_ptr<const PrefixSet> GetPrefixSetNoHashes();

 private:
  // Encode a run of deltas for |AddRun()|.  The run is broken by a too-large
  // delta, or kMaxRun, whichever comes first.
  void EmitRun();

  // Buffers prefixes until enough are avaliable to emit a run.
  std::vector<SBPrefix> buffer_;

  // The PrefixSet being built.
  std::unique_ptr<PrefixSet> prefix_set_;
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_DB_PREFIX_SET_H_
