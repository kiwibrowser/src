// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of ChunkRange class.

#include <stddef.h>

#include <algorithm>

#include "chrome/browser/safe_browsing/chunk_range.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace safe_browsing {

ChunkRange::ChunkRange(int start) : start_(start), stop_(start) {
}

ChunkRange::ChunkRange(int start, int stop) : start_(start), stop_(stop) {
}

ChunkRange::ChunkRange(const ChunkRange& rhs)
    : start_(rhs.start()), stop_(rhs.stop()) {
}

// Helper functions -----------------------------------------------------------

void ChunksToRangeString(const std::vector<int>& chunks, std::string* result) {
  // The following code requires the range to be sorted.
  std::vector<int> sorted_chunks(chunks);
  std::sort(sorted_chunks.begin(), sorted_chunks.end());

  DCHECK(result);
  result->clear();
  std::vector<int>::const_iterator iter = sorted_chunks.begin();
  while (iter != sorted_chunks.end()) {
    const int range_begin = *iter;
    int range_end = *iter;

    // Extend the range forward across duplicates and increments.
    for (; iter != sorted_chunks.end() && *iter <= range_end + 1; ++iter) {
      range_end = *iter;
    }

    if (!result->empty())
      result->append(",");
    result->append(base::IntToString(range_begin));
    if (range_end > range_begin) {
      result->append("-");
      result->append(base::IntToString(range_end));
    }
  }
}

void RangesToChunks(const std::vector<ChunkRange>& ranges,
                    std::vector<int>* chunks) {
  DCHECK(chunks);
  for (size_t i = 0; i < ranges.size(); ++i) {
    const ChunkRange& range = ranges[i];
    for (int chunk = range.start(); chunk <= range.stop(); ++chunk) {
      chunks->push_back(chunk);
    }
  }
}

bool StringToRanges(const std::string& input,
                    std::vector<ChunkRange>* ranges) {
  DCHECK(ranges);

  // Crack the string into chunk parts, then crack each part looking for a
  // range.
  for (const base::StringPiece& chunk : base::SplitStringPiece(
           input, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    std::vector<std::string> chunk_ranges = base::SplitString(
        chunk, "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    int start = atoi(chunk_ranges[0].c_str());
    int stop = start;
    if (chunk_ranges.size() == 2)
      stop = atoi(chunk_ranges[1].c_str());
    if (start == 0 || stop == 0) {
      // atoi error, since chunk numbers are guaranteed to never be 0.
      ranges->clear();
      return false;
    }
    ranges->push_back(ChunkRange(start, stop));
  }

  return true;
}

// Binary search over a series of ChunkRanges.
bool IsChunkInRange(int chunk_number, const std::vector<ChunkRange>& ranges) {
  if (ranges.empty())
    return false;

  int low = 0;
  int high = ranges.size() - 1;

  while (low <= high) {
    // http://googleresearch.blogspot.com/2006/06/extra-extra-read-all-about-it-nearly.html
    int mid = ((unsigned int)low + (unsigned int)high) >> 1;
    const ChunkRange& chunk = ranges[mid];
    if ((chunk.stop() >= chunk_number) && (chunk.start() <= chunk_number))
      return true;  // chunk_number is in range.

    // Adjust our mid point.
    if (chunk.stop() < chunk_number)
      low = mid + 1;
    else
      high = mid - 1;
  }

  return false;
}

}  // namespace safe_browsing
