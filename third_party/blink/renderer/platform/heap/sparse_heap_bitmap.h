// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_SPARSE_HEAP_BITMAP_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_SPARSE_HEAP_BITMAP_H_

#include <bitset>
#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/heap/blink_gc.h"
#include "third_party/blink/renderer/platform/heap/heap_page.h"
#include "third_party/blink/renderer/platform/wtf/alignment.h"

namespace blink {

// A sparse bitmap of heap addresses where the (very few) addresses that are
// set are likely to be in small clusters. The abstraction is tailored to
// support heap compaction, assuming the following:
//
//   - Addresses will be bitmap-marked from lower to higher addresses.
//   - Bitmap lookups are performed for each object that is compacted
//     and moved to some new location, supplying the (base, size)
//     pair of the object's heap allocation.
//   - If the sparse bitmap has any marked addresses in that range, it
//     returns a sub-bitmap that can be quickly iterated over to check which
//     addresses within the range are actually set.
//   - The bitmap is needed to support something that is very rarely done
//     by the current Blink codebase, which is to have nested collection
//     part objects. Consequently, it is safe to assume sparseness.
//
// Support the above by having a sparse bitmap organized as a binary
// tree with nodes covering fixed size ranges via a simple bitmap/set.
// That is, each SparseHeapBitmap node will contain a bitmap/set for
// some fixed size range, along with pointers to SparseHeapBitmaps
// for addresses on each side its range.
//
// This bitmap tree isn't kept balanced across the Address additions
// made.
//
class PLATFORM_EXPORT SparseHeapBitmap {
 public:
  static std::unique_ptr<SparseHeapBitmap> Create(Address base) {
    return base::WrapUnique(new SparseHeapBitmap(base));
  }

  ~SparseHeapBitmap() = default;

  // Return the sparse bitmap subtree that at least covers the
  // [address, address + size) range, or nullptr if none.
  //
  // The returned SparseHeapBitmap can be used to quickly lookup what
  // addresses in that range are set or not; see |isSet()|. Its
  // |isSet()| behavior outside that range is not defined.
  SparseHeapBitmap* HasRange(Address, size_t);

  // True iff |address| is set for this SparseHeapBitmap tree.
  bool IsSet(Address);

  // Mark |address| as present/set.
  void Add(Address);

  // The assumed minimum alignment of the pointers being added. Cannot
  // exceed |log2(allocationGranularity)|; having it be equal to
  // the platform pointer alignment is what's wanted.
  static const int kPointerAlignmentInBits = WTF_ALIGN_OF(void*) == 8 ? 3 : 2;
  static const size_t kPointerAlignmentMask =
      (0x1u << kPointerAlignmentInBits) - 1;

  // Represent ranges in 0x100 bitset chunks; bit I is set iff Address
  // |m_base + I * (0x1 << s_pointerAlignmentInBits)| has been added to the
  // |SparseHeapBitmap|.
  static const size_t kBitmapChunkSize = 0x100;

  // A SparseHeapBitmap either contains a single Address or a bitmap
  // recording the mapping for [m_base, m_base + s_bitmapChunkRange)
  static const size_t kBitmapChunkRange = kBitmapChunkSize
                                          << kPointerAlignmentInBits;

  // Return the number of nodes; for debug stats.
  size_t IntervalCount() const;

 private:
  explicit SparseHeapBitmap(Address base) : base_(base), size_(1) {
    DCHECK(!(reinterpret_cast<uintptr_t>(base_) & kPointerAlignmentMask));
    static_assert(kPointerAlignmentMask <= kAllocationMask,
                  "address shift exceeds heap pointer alignment");
    // For now, only recognize 8 and 4.
    static_assert(WTF_ALIGN_OF(void*) == 8 || WTF_ALIGN_OF(void*) == 4,
                  "unsupported pointer alignment");
  }

  Address Base() const { return base_; }
  size_t size() const { return size_; }
  Address end() const { return Base() + (size_ - 1); }

  Address MaxEnd() const { return Base() + kBitmapChunkRange; }

  Address MinStart() const {
    // If this bitmap node represents the sparse [m_base, s_bitmapChunkRange)
    // range, do not allow it to be "left extended" as that would entail
    // having to shift down the contents of the std::bitset somehow.
    //
    // This shouldn't be a real problem as any clusters of set addresses
    // will be marked while iterating from lower to higher addresses, hence
    // "left extension" are unlikely to be common.
    if (bitmap_)
      return Base();
    return (base_ > reinterpret_cast<Address>(kBitmapChunkRange))
               ? (Base() - kBitmapChunkRange + 1)
               : nullptr;
  }

  Address SwapBase(Address address) {
    DCHECK(!(reinterpret_cast<uintptr_t>(address) & kPointerAlignmentMask));
    Address old_base = base_;
    base_ = address;
    return old_base;
  }

  void CreateBitmap();

  Address base_;
  // Either 1 or |s_bitmapChunkRange|.
  size_t size_;

  // If non-null, contains a bitmap for addresses within [m_base, m_size)
  std::unique_ptr<std::bitset<kBitmapChunkSize>> bitmap_;

  std::unique_ptr<SparseHeapBitmap> left_;
  std::unique_ptr<SparseHeapBitmap> right_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_SPARSE_HEAP_BITMAP_H_
