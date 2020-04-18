/*
 * Copyright (C) 2004, 2006, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_SEARCH_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_SEARCH_BUFFER_H_

#include <memory>

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/editing/finder/find_options.h"
#include "third_party/blink/renderer/core/editing/forward.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/unicode.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class TextSearcherICU;

// Buffer that knows how to compare with a search target.
// Keeps enough of the previous text to be able to search in the future, but no
// more. Non-breaking spaces are always equal to normal spaces. Case folding is
// also done if the CaseInsensitive option is specified. Matches are further
// filtered if the AtWordStarts option is specified, although some matches
// inside a word are permitted if TreatMedialCapitalAsWordStart is specified as
// well.
class SearchBuffer {
  STACK_ALLOCATED();

 public:
  SearchBuffer(const String& target, FindOptions);
  ~SearchBuffer();

  // Returns number of characters appended; guaranteed to be in the range
  // [1, length].
  template <typename CharType>
  void Append(const CharType*, size_t length);
  size_t NumberOfCharactersJustAppended() const {
    return number_of_characters_just_appended_;
  }

  bool NeedsMoreContext() const;
  void PrependContext(const UChar*, size_t length);
  void ReachedBreak();

  // Result is the size in characters of what was found.
  // And <startOffset> is the number of characters back to the start of what
  // was found.
  size_t Search(size_t& start_offset);
  bool AtBreak() const;

 private:
  bool IsBadMatch(const UChar*, size_t length) const;
  bool IsWordStartMatch(size_t start, size_t length) const;

  Vector<UChar> target_;
  FindOptions options_;

  Vector<UChar> buffer_;
  size_t overlap_;
  size_t prefix_length_;
  size_t number_of_characters_just_appended_;
  bool at_break_;
  bool needs_more_context_;

  bool target_requires_kana_workaround_;
  Vector<UChar> normalized_target_;
  mutable Vector<UChar> normalized_match_;

  std::unique_ptr<TextSearcherICU> text_searcher_;

  DISALLOW_COPY_AND_ASSIGN(SearchBuffer);
};

CORE_EXPORT EphemeralRange FindPlainText(const EphemeralRange& input_range,
                                         const String&,
                                         FindOptions);
CORE_EXPORT EphemeralRangeInFlatTree
FindPlainText(const EphemeralRangeInFlatTree& input_range,
              const String&,
              FindOptions);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_SEARCH_BUFFER_H_
