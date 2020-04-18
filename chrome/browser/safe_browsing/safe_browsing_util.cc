// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_util.h"

#include <utility>

#include "base/strings/string_util.h"
#include "chrome/browser/safe_browsing/chunk.pb.h"
#include "components/google/core/browser/google_util.h"

namespace safe_browsing {

// SBChunkData -----------------------------------------------------------------

// TODO(shess): Right now this contains a std::unique_ptr<ChunkData> so that the
// proto buffer isn't copied all over the place, then these are contained in a
// ScopedVector for purposes of passing things around between tasks.  This seems
// convoluted.  Maybe it would make sense to have an overall container class
// returning references to a nested per-chunk class?

SBChunkData::SBChunkData() {
}

SBChunkData::SBChunkData(std::unique_ptr<ChunkData> data)
    : chunk_data_(std::move(data)) {
  DCHECK(chunk_data_.get());
}

SBChunkData::~SBChunkData() {
}

bool SBChunkData::ParseFrom(const unsigned char* data, size_t length) {
  std::unique_ptr<ChunkData> chunk(new ChunkData());
  if (!chunk->ParseFromArray(data, length))
    return false;

  if (chunk->chunk_type() != ChunkData::ADD &&
      chunk->chunk_type() != ChunkData::SUB) {
    return false;
  }

  size_t hash_size = 0;
  if (chunk->prefix_type() == ChunkData::PREFIX_4B) {
    hash_size = sizeof(SBPrefix);
  } else if (chunk->prefix_type() == ChunkData::FULL_32B) {
    hash_size = sizeof(SBFullHash);
  } else {
    return false;
  }

  const size_t hash_count = chunk->hashes().size() / hash_size;
  if (hash_count * hash_size != chunk->hashes().size())
    return false;

  if (chunk->chunk_type() == ChunkData::SUB &&
      static_cast<size_t>(chunk->add_numbers_size()) != hash_count) {
    return false;
  }

  chunk_data_.swap(chunk);
  return true;
}

int SBChunkData::ChunkNumber() const {
  return chunk_data_->chunk_number();
}

bool SBChunkData::IsAdd() const {
  return chunk_data_->chunk_type() == ChunkData::ADD;
}

bool SBChunkData::IsSub() const {
  return chunk_data_->chunk_type() == ChunkData::SUB;
}

int SBChunkData::AddChunkNumberAt(size_t i) const {
  DCHECK(IsSub());
  DCHECK((IsPrefix() && i < PrefixCount()) ||
         (IsFullHash() && i < FullHashCount()));
  return chunk_data_->add_numbers(i);
}

bool SBChunkData::IsPrefix() const {
  return chunk_data_->prefix_type() == ChunkData::PREFIX_4B;
}

size_t SBChunkData::PrefixCount() const {
  DCHECK(IsPrefix());
  return chunk_data_->hashes().size() / sizeof(SBPrefix);
}

SBPrefix SBChunkData::PrefixAt(size_t i) const {
  DCHECK(IsPrefix());
  DCHECK_LT(i, PrefixCount());

  SBPrefix prefix;
  memcpy(&prefix, chunk_data_->hashes().data() + i * sizeof(SBPrefix),
         sizeof(SBPrefix));
  return prefix;
}

bool SBChunkData::IsFullHash() const {
  return chunk_data_->prefix_type() == ChunkData::FULL_32B;
}

size_t SBChunkData::FullHashCount() const {
  DCHECK(IsFullHash());
  return chunk_data_->hashes().size() / sizeof(SBFullHash);
}

SBFullHash SBChunkData::FullHashAt(size_t i) const {
  DCHECK(IsFullHash());
  DCHECK_LT(i, FullHashCount());

  SBFullHash full_hash;
  memcpy(&full_hash, chunk_data_->hashes().data() + i * sizeof(SBFullHash),
         sizeof(SBFullHash));
  return full_hash;
}

// SBListChunkRanges -----------------------------------------------------------

SBListChunkRanges::SBListChunkRanges(const std::string& n)
    : name(n) {
}

// SBChunkDelete ---------------------------------------------------------------

SBChunkDelete::SBChunkDelete() : is_sub_del(false) {}

SBChunkDelete::SBChunkDelete(const SBChunkDelete& other) = default;

SBChunkDelete::~SBChunkDelete() {}

}  // namespace safe_browsing
