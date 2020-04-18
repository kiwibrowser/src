// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZUCCHINI_REL32_FINDER_H_
#define COMPONENTS_ZUCCHINI_REL32_FINDER_H_

#include <stddef.h>

#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/optional.h"
#include "components/zucchini/buffer_view.h"
#include "components/zucchini/image_utils.h"

namespace zucchini {

// See README.md for definitions on abs32 and rel32 references. We assume the
// following:
// - Abs32 locations have fixed lengths, and never overlap.
// - Rel32 locations can be reasonably identified by heuristically disassembling
//   machine code.
// - Rel32 locations never overlap with each other, and never with abs32
//   locations.

// Abs32GapFinder is a class that iterates over all contiguous gaps in |region|
// that lie outside of |abs32_locations| elements, each spanning |abs_width|
// bytes. For example, given
//   region = [base_ + 8, base_ + 25),
//   abs32_locations = {2, 6, 15, 20, 27},
//   abs32_width_ = 4,
// we obtain the following:
//             111111111122222222223   -> offsets
//   0123456789012345678901234567890
//   ........*****************......   -> region = *
//     ^   ^        ^    ^      ^      -> abs32 locations
//     aaaaaaaa     aaaa aaaa   aaaa   -> abs32 locations with width
//   ........--*****----*----*......   -> region excluding abs32 -> 3 gaps
// The resulting gaps (must be non-empty) are:
//   [10, 15), [19, 20), [24, 25).
// These gaps can then be passed to Rel32Finder (below) to find rel32 references
// that are guaranteed to not overlap with any abs32 references.
class Abs32GapFinder {
 public:
  // |abs32_locations| is a sorted list of non-overlapping abs32 reference
  // locations in |image|, each spanning |abs32_width| bytes. Gaps are searched
  // in |region|, which must be part of |image|.
  Abs32GapFinder(ConstBufferView image,
                 ConstBufferView region,
                 const std::vector<offset_t>& abs32_locations,
                 size_t abs32_width);
  ~Abs32GapFinder();

  // Returns the next available gap, or nullopt if exhausted.
  base::Optional<ConstBufferView> GetNext();

 private:
  const ConstBufferView::const_iterator base_;
  const ConstBufferView::const_iterator region_end_;
  ConstBufferView::const_iterator current_lo_;
  std::vector<offset_t>::const_iterator abs32_current_;
  std::vector<offset_t>::const_iterator abs32_end_;
  size_t abs32_width_;

  DISALLOW_COPY_AND_ASSIGN(Abs32GapFinder);
};

// A class to parse executable bytes of an image to find rel32 locations.
// Architecture-specific parse details are delegated to inherited classes.
// This is typically used along with Abs32GapFinder to find search regions.
// The caller may filter rel32 locations, based on rel32 targets.
class Rel32Finder {
 public:
  Rel32Finder();
  // |region| is the region being scanned for rel32 references.
  explicit Rel32Finder(ConstBufferView region);
  virtual ~Rel32Finder();

  // Reset object to start scanning for rel32 references in |region|.
  void Reset(ConstBufferView region) {
    next_cursor_ = region.begin();
    region_ = region;
  }

  // Accept the last reference found. Next call to FindNext() will scan starting
  // beyond that reference, instead of the current search position.
  void Accept() { region_.seek(next_cursor_); }

  // Accessors for unittest.
  ConstBufferView::const_iterator next_cursor() const { return next_cursor_; }
  ConstBufferView region() const { return region_; }

 protected:
  // Scans for the next rel32 reference. If a reference is found, advances the
  // search position beyond it and returns true. Otherwise, moves the search
  // position to the end of the region and returns false.
  bool FindNext() {
    ConstBufferView result = Scan(region_);
    region_.seek(result.begin());
    next_cursor_ = result.end();
    if (region_.empty())
      return false;
    region_.remove_prefix(1);
    DCHECK_GE(next_cursor_, region_.begin());
    DCHECK_LE(next_cursor_, region_.end());
    return true;
  }

  // Architecture-specific rel32 reference detection, which scans executable
  // bytes given by |region|. For each rel32 reference found, the implementation
  // should cache the necessary data to be retrieved via accessors and return a
  // region starting at the current search position, and ending beyond the
  // reference that was just found, or an empty region starting at the end of
  // the search region if no more reference is found. By default, the next time
  // FindNext() is called, |region| will start at the current search position,
  // unless Accept() was called, in which case |region| will start beyond the
  // last reference.
  virtual ConstBufferView Scan(ConstBufferView region) = 0;

 private:
  ConstBufferView region_;
  ConstBufferView::const_iterator next_cursor_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Rel32Finder);
};

// Parsing for X86 or X64: we perform naive scan for opcodes that have rel32 as
// an argument, and disregard instruction alignment.
class Rel32FinderIntel : public Rel32Finder {
 public:
  // Struct to store GetNext() results.
  struct Result {
    ConstBufferView::const_iterator location;

    // Some references must have their target in the same section as location,
    // which we use this to heuristically reject rel32 reference candidates.
    // When true, this constraint is relaxed.
    bool can_point_outside_section;
  };

  using Rel32Finder::Rel32Finder;

  // Returns the next available Result, or nullopt if exhausted.
  base::Optional<Result> GetNext() {
    if (FindNext())
      return rel32_;
    return base::nullopt;
  }

 protected:
  // Cached results.
  Result rel32_;

  // Rel32Finder:
  ConstBufferView Scan(ConstBufferView region) override = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Rel32FinderIntel);
};

// X86 instructions.
class Rel32FinderX86 : public Rel32FinderIntel {
 public:
  using Rel32FinderIntel::Rel32FinderIntel;

 private:
  // Rel32Finder:
  ConstBufferView Scan(ConstBufferView region) override;

  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX86);
};

// X64 instructions.
class Rel32FinderX64 : public Rel32FinderIntel {
 public:
  using Rel32FinderIntel::Rel32FinderIntel;

 private:
  // Rel32Finder:
  ConstBufferView Scan(ConstBufferView region) override;

  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX64);
};

}  // namespace zucchini

#endif  // COMPONENTS_ZUCCHINI_REL32_FINDER_H_
