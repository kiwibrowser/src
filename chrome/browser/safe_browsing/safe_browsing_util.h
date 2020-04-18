// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utilities for the SafeBrowsing code.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_UTIL_H_
#define CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_UTIL_H_

#include <stddef.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "chrome/browser/safe_browsing/chunk_range.h"
#include "components/safe_browsing/db/util.h"

namespace safe_browsing {

class ChunkData;

// Container for holding a chunk URL and the list it belongs to.
struct ChunkUrl {
  std::string url;
  std::string list_name;
};

// Data for an individual chunk sent from the server.
class SBChunkData {
 public:
  SBChunkData();
  ~SBChunkData();

  // Create with manufactured data, for testing only.
  // TODO(shess): Right now the test code calling this is in an anonymous
  // namespace.  Figure out how to shift this into private:.
  explicit SBChunkData(std::unique_ptr<ChunkData> data);

  // Read serialized ChunkData, returning true if the parse suceeded.
  bool ParseFrom(const unsigned char* data, size_t length);

  // Access the chunk data.  |AddChunkNumberAt()| can only be called if
  // |IsSub()| returns true.  |Prefix*()| and |FullHash*()| can only be called
  // if the corrosponding |Is*()| returned true.
  int ChunkNumber() const;
  bool IsAdd() const;
  bool IsSub() const;
  int AddChunkNumberAt(size_t i) const;
  bool IsPrefix() const;
  size_t PrefixCount() const;
  SBPrefix PrefixAt(size_t i) const;
  bool IsFullHash() const;
  size_t FullHashCount() const;
  SBFullHash FullHashAt(size_t i) const;

 private:
  // Protocol buffer sent from server.
  std::unique_ptr<ChunkData> chunk_data_;

  DISALLOW_COPY_AND_ASSIGN(SBChunkData);
};

// Contains information about a list in the database.
struct SBListChunkRanges {
  explicit SBListChunkRanges(const std::string& n);

  std::string name;  // The list name.
  std::string adds;  // The ranges for add chunks.
  std::string subs;  // The ranges for sub chunks.
};

// Container for deleting chunks from the database.
struct SBChunkDelete {
  SBChunkDelete();
  SBChunkDelete(const SBChunkDelete& other);
  ~SBChunkDelete();

  std::string list_name;
  bool is_sub_del;
  std::vector<ChunkRange> chunk_del;
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_UTIL_H_
