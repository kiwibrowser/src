// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_TEXT_SEARCHER_ICU_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_TEXT_SEARCHER_ICU_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "third_party/blink/renderer/platform/wtf/text/unicode.h"

struct UStringSearch;

namespace blink {

struct CORE_EXPORT MatchResultICU {
  size_t start;
  size_t length;
};

class CORE_EXPORT TextSearcherICU {
 public:
  TextSearcherICU();
  ~TextSearcherICU();

  void SetPattern(const StringView& pattern, bool sensitive);
  void SetText(const UChar* text, size_t length);
  void SetOffset(size_t);
  bool NextMatchResult(MatchResultICU&);

 private:
  void SetPattern(const UChar* pattern, size_t length);
  void SetCaseSensitivity(bool case_sensitive);

  UStringSearch* searcher_ = nullptr;
  size_t text_length_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TextSearcherICU);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_TEXT_SEARCHER_ICU_H_
