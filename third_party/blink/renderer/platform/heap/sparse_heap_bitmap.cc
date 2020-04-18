// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/sparse_heap_bitmap.h"

#include "third_party/blink/renderer/platform/heap/heap.h"

namespace blink {

// Return the subtree/bitmap that covers the
// [address, address + size) range.  Null if there is none.
SparseHeapBitmap* SparseHeapBitmap::HasRange(Address address, size_t size) {
  DCHECK(!(reinterpret_cast<uintptr_t>(address) & kPointerAlignmentMask));
  SparseHeapBitmap* bitmap = this;
  while (bitmap) {
    // Interval starts after, |m_right| handles.
    if (address > bitmap->end()) {
      bitmap = bitmap->right_.get();
      continue;
    }
    // Interval starts within, |bitmap| is included in the resulting range.
    if (address >= bitmap->Base())
      break;

    Address right = address + size - 1;
    // Interval starts before, but intersects with |bitmap|'s range.
    if (right >= bitmap->Base())
      break;

    // Interval is before entirely, for |m_left| to handle.
    bitmap = bitmap->left_.get();
  }
  return bitmap;
}

bool SparseHeapBitmap::IsSet(Address address) {
  DCHECK(!(reinterpret_cast<uintptr_t>(address) & kPointerAlignmentMask));
  SparseHeapBitmap* bitmap = this;
  while (bitmap) {
    if (address > bitmap->end()) {
      bitmap = bitmap->right_.get();
      continue;
    }
    if (address >= bitmap->Base()) {
      if (bitmap->bitmap_) {
        return bitmap->bitmap_->test((address - bitmap->Base()) >>
                                     kPointerAlignmentInBits);
      }
      DCHECK(address == bitmap->Base());
      DCHECK_EQ(bitmap->size(), 1u);
      return true;
    }
    bitmap = bitmap->left_.get();
  }
  return false;
}

void SparseHeapBitmap::Add(Address address) {
  DCHECK(!(reinterpret_cast<uintptr_t>(address) & kPointerAlignmentMask));
  // |address| is beyond the maximum that this SparseHeapBitmap node can
  // encompass.
  if (address >= MaxEnd()) {
    if (!right_) {
      right_ = SparseHeapBitmap::Create(address);
      return;
    }
    right_->Add(address);
    return;
  }
  // Same on the other side.
  if (address < MinStart()) {
    if (!left_) {
      left_ = SparseHeapBitmap::Create(address);
      return;
    }
    left_->Add(address);
    return;
  }
  if (address == Base())
    return;
  // |address| can be encompassed by |this| by expanding its size.
  if (address > Base()) {
    if (!bitmap_)
      CreateBitmap();
    bitmap_->set((address - Base()) >> kPointerAlignmentInBits);
    return;
  }
  // Use |address| as the new base for this interval.
  Address old_base = SwapBase(address);
  CreateBitmap();
  bitmap_->set((old_base - address) >> kPointerAlignmentInBits);
}

void SparseHeapBitmap::CreateBitmap() {
  DCHECK(!bitmap_ && size() == 1);
  bitmap_ = std::make_unique<std::bitset<kBitmapChunkSize>>();
  size_ = kBitmapChunkRange;
  bitmap_->set(0);
}

size_t SparseHeapBitmap::IntervalCount() const {
  size_t count = 1;
  if (left_)
    count += left_->IntervalCount();
  if (right_)
    count += right_->IntervalCount();
  return count;
}

}  // namespace blink
