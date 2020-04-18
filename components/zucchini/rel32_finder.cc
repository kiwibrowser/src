// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/rel32_finder.h"

#include <algorithm>

namespace zucchini {

/******** Abs32GapFinder ********/

Abs32GapFinder::Abs32GapFinder(ConstBufferView image,
                               ConstBufferView region,
                               const std::vector<offset_t>& abs32_locations,
                               size_t abs32_width)
    : base_(image.begin()),
      region_end_(region.end()),
      abs32_end_(abs32_locations.end()),
      abs32_width_(abs32_width) {
  DCHECK_GT(abs32_width, size_t(0));
  DCHECK_GE(region.begin(), image.begin());
  DCHECK_LE(region.end(), image.end());

  const offset_t begin_offset = region.begin() - image.begin();
  // Find the first |abs32_current_| with |*abs32_current_ >= begin_offset|.
  abs32_current_ = std::lower_bound(abs32_locations.begin(),
                                    abs32_locations.end(), begin_offset);

  // Find lower boundary, accounting for possibility that |abs32_current_[-1]|
  // may straddle across |region.begin()|.
  current_lo_ = region.begin();
  if (abs32_current_ > abs32_locations.begin()) {
    current_lo_ = std::max(current_lo_,
                           image.begin() + abs32_current_[-1] + abs32_width_);
  }
}

Abs32GapFinder::~Abs32GapFinder() = default;

base::Optional<ConstBufferView> Abs32GapFinder::GetNext() {
  // Iterate over |[abs32_current_, abs32_end_)| and emit segments.
  while (abs32_current_ != abs32_end_ &&
         base_ + *abs32_current_ < region_end_) {
    ConstBufferView::const_iterator hi = base_ + *abs32_current_;
    ConstBufferView gap = ConstBufferView::FromRange(current_lo_, hi);
    current_lo_ = hi + abs32_width_;
    ++abs32_current_;
    if (!gap.empty())
      return gap;
  }
  // Emit final segment.
  if (current_lo_ < region_end_) {
    ConstBufferView gap = ConstBufferView::FromRange(current_lo_, region_end_);
    current_lo_ = region_end_;
    return gap;
  }
  return base::nullopt;
}

/******** Rel32Finder ********/

Rel32Finder::Rel32Finder() = default;

Rel32Finder::Rel32Finder(ConstBufferView region)
    : region_(region), next_cursor_(region_.begin()) {}

Rel32Finder::~Rel32Finder() = default;

/******** Rel32FinderX86 ********/

ConstBufferView Rel32FinderX86::Scan(ConstBufferView region) {
  ConstBufferView::const_iterator cursor = region.begin();
  while (cursor < region.end()) {
    // Heuristic rel32 detection by looking for opcodes that use them.
    if (cursor + 5 <= region.end()) {
      if (cursor[0] == 0xE8 || cursor[0] == 0xE9) {  // JMP rel32; CALL rel32
        rel32_ = {cursor + 1, false};
        return ConstBufferView::FromRange(cursor, rel32_.location + 4);
      }
    }
    if (cursor + 6 <= region.end()) {
      if (cursor[0] == 0x0F && (cursor[1] & 0xF0) == 0x80) {  // Jcc long form
        rel32_ = {cursor + 2, false};
        return ConstBufferView::FromRange(cursor, rel32_.location + 4);
      }
    }
    ++cursor;
  }
  return {region.end(), 0};
}

/******** Rel32FinderX64 ********/

ConstBufferView Rel32FinderX64::Scan(ConstBufferView region) {
  ConstBufferView::const_iterator cursor = region.begin();
  while (cursor < region.end()) {
    // Heuristic rel32 detection by looking for opcodes that use them.
    if (cursor + 5 <= region.end()) {
      if (cursor[0] == 0xE8 || cursor[0] == 0xE9) {  // JMP rel32; CALL rel32
        rel32_ = {cursor + 1, false};
        return ConstBufferView::FromRange(cursor, rel32_.location + 4);
      }
    }
    if (cursor + 6 <= region.end()) {
      if (cursor[0] == 0x0F && (cursor[1] & 0xF0) == 0x80) {  // Jcc long form
        rel32_ = {cursor + 2, false};
        return ConstBufferView::FromRange(cursor, rel32_.location + 4);
      } else if ((cursor[0] == 0xFF &&
                  (cursor[1] == 0x15 || cursor[1] == 0x25)) ||
                 ((cursor[0] == 0x89 || cursor[0] == 0x8B ||
                   cursor[0] == 0x8D) &&
                  (cursor[1] & 0xC7) == 0x05)) {
        // 6-byte instructions:
        // [2-byte opcode] [disp32]:
        //  Opcode
        //   FF 15: CALL QWORD PTR [rip+disp32]
        //   FF 25: JMP  QWORD PTR [rip+disp32]
        //
        // [1-byte opcode] [ModR/M] [disp32]:
        //  Opcode
        //   89: MOV DWORD PTR [rip+disp32],reg
        //   8B: MOV reg,DWORD PTR [rip+disp32]
        //   8D: LEA reg,[rip+disp32]
        //  ModR/M : MMRRRMMM
        //   MM = 00 & MMM = 101 => rip+disp32
        //   RRR: selects reg operand from [eax|ecx|...|edi]
        rel32_ = {cursor + 2, true};
        return ConstBufferView::FromRange(cursor, rel32_.location + 4);
      }
    }
    ++cursor;
  }
  return {region.end(), 0};
}

}  // namespace zucchini
