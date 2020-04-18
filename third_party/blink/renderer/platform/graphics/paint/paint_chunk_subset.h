// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CHUNK_SUBSET_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CHUNK_SUBSET_H_

#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

struct PaintChunk;

// Provides access to a subset of a Vector<PaintChunk>.
class PaintChunkSubset {
 public:
  PaintChunkSubset(const Vector<PaintChunk>& chunks,
                   const Vector<size_t>& subset_indices)
      : chunks_(chunks), subset_indices_(&subset_indices) {}

  // For convenience, this allows using a Vector<PaintChunk> in place of
  // PaintChunkSubset to include all paint chunks.
  PaintChunkSubset(const Vector<PaintChunk>& chunks)
      : chunks_(chunks), subset_indices_(nullptr) {}

  class Iterator {
   public:
    const PaintChunk& operator*() const { return subset_[offset_]; }
    bool operator!=(const Iterator& other) const {
      DCHECK_EQ(&subset_, &other.subset_);
      return offset_ != other.offset_;
    }
    const Iterator& operator++() {
      ++offset_;
      return *this;
    }

   private:
    friend class PaintChunkSubset;
    Iterator(const PaintChunkSubset& subset, size_t offset)
        : subset_(subset), offset_(offset) {}

    const PaintChunkSubset& subset_;
    size_t offset_;
  };

  Iterator begin() const { return Iterator(*this, 0); }

  Iterator end() const { return Iterator(*this, size()); }

  size_t size() const {
    return subset_indices_ ? subset_indices_->size() : chunks_.size();
  }

  const PaintChunk& operator[](int i) const {
    return chunks_[subset_indices_ ? (*subset_indices_)[i] : i];
  }

 private:
  const Vector<PaintChunk>& chunks_;
  const Vector<size_t>* subset_indices_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CHUNK_SUBSET_H_
