// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPVT_WORDRANGE_H_
#define CORE_FPDFDOC_CPVT_WORDRANGE_H_

#include <algorithm>
#include <utility>

#include "core/fpdfdoc/cpvt_wordplace.h"
#include "core/fxcrt/fx_system.h"

struct CPVT_WordRange {
  CPVT_WordRange() {}

  CPVT_WordRange(const CPVT_WordPlace& begin, const CPVT_WordPlace& end)
      : BeginPos(begin), EndPos(end) {
    Normalize();
  }

  void Reset() {
    BeginPos.Reset();
    EndPos.Reset();
  }

  void Set(const CPVT_WordPlace& begin, const CPVT_WordPlace& end) {
    BeginPos = begin;
    EndPos = end;
    Normalize();
  }

  void SetBeginPos(const CPVT_WordPlace& begin) {
    BeginPos = begin;
    Normalize();
  }

  void SetEndPos(const CPVT_WordPlace& end) {
    EndPos = end;
    Normalize();
  }

  CPVT_WordRange Intersect(const CPVT_WordRange& that) const {
    if (that.EndPos < BeginPos || that.BeginPos > EndPos)
      return CPVT_WordRange();

    return CPVT_WordRange(std::max(BeginPos, that.BeginPos),
                          std::min(EndPos, that.EndPos));
  }

  inline bool IsEmpty() const { return BeginPos == EndPos; }
  inline bool operator==(const CPVT_WordRange& wr) const {
    return wr.BeginPos == BeginPos && wr.EndPos == EndPos;
  }
  inline bool operator!=(const CPVT_WordRange& wr) const {
    return !(*this == wr);
  }

  void Normalize() {
    if (BeginPos > EndPos)
      std::swap(BeginPos, EndPos);
  }

  CPVT_WordPlace BeginPos;
  CPVT_WordPlace EndPos;
};

#endif  // CORE_FPDFDOC_CPVT_WORDRANGE_H_
