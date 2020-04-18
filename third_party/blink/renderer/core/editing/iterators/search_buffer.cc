/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2005 Alexey Proskuryakov.
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

#include "third_party/blink/renderer/core/editing/iterators/search_buffer.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/iterators/character_iterator.h"
#include "third_party/blink/renderer/core/editing/iterators/simplified_backwards_text_iterator.h"
#include "third_party/blink/renderer/core/editing/iterators/text_searcher_icu.h"
#include "third_party/blink/renderer/platform/text/character.h"
#include "third_party/blink/renderer/platform/text/text_boundaries.h"
#include "third_party/blink/renderer/platform/text/unicode_utilities.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"

namespace blink {

namespace {

const size_t kMinimumSearchBufferSize = 8192;

UChar32 GetCodePointAt(const UChar* str, size_t index, size_t length) {
  UChar32 c;
  U16_GET(str, 0, index, length, c);
  return c;
}

}  // namespace

inline SearchBuffer::SearchBuffer(const String& target, FindOptions options)
    : options_(options),
      prefix_length_(0),
      number_of_characters_just_appended_(0),
      at_break_(true),
      needs_more_context_(options & kAtWordStarts),
      target_requires_kana_workaround_(ContainsKanaLetters(target)) {
  DCHECK(!target.IsEmpty()) << target;
  target.AppendTo(target_);

  // FIXME: We'd like to tailor the searcher to fold quote marks for us instead
  // of doing it in a separate replacement pass here, but ICU doesn't offer a
  // way to add tailoring on top of the locale-specific tailoring as of this
  // writing.
  FoldQuoteMarksAndSoftHyphens(target_.data(), target_.size());

  size_t target_length = target_.size();
  buffer_.ReserveInitialCapacity(
      std::max(target_length * 8, kMinimumSearchBufferSize));
  overlap_ = buffer_.capacity() / 4;

  if ((options_ & kAtWordStarts) && target_length) {
    const UChar32 target_first_character =
        GetCodePointAt(target_.data(), 0, target_length);
    // Characters in the separator category never really occur at the beginning
    // of a word, so if the target begins with such a character, we just ignore
    // the AtWordStart option.
    if (IsSeparator(target_first_character)) {
      options_ &= ~kAtWordStarts;
      needs_more_context_ = false;
    }
  }

  text_searcher_ = std::make_unique<TextSearcherICU>();
  text_searcher_->SetPattern(StringView(target_.data(), target_.size()),
                             !(options_ & kCaseInsensitive));

  // The kana workaround requires a normalized copy of the target string.
  if (target_requires_kana_workaround_)
    NormalizeCharactersIntoNFCForm(target_.data(), target_.size(),
                                   normalized_target_);
}

inline SearchBuffer::~SearchBuffer() = default;

template <typename CharType>
inline void SearchBuffer::Append(const CharType* characters, size_t length) {
  DCHECK(length);

  if (at_break_) {
    buffer_.Shrink(0);
    prefix_length_ = 0;
    at_break_ = false;
  } else if (buffer_.size() == buffer_.capacity()) {
    memcpy(buffer_.data(), buffer_.data() + buffer_.size() - overlap_,
           overlap_ * sizeof(UChar));
    prefix_length_ -= std::min(prefix_length_, buffer_.size() - overlap_);
    buffer_.Shrink(overlap_);
  }

  size_t old_length = buffer_.size();
  size_t usable_length = std::min(buffer_.capacity() - old_length, length);
  DCHECK(usable_length);
  buffer_.resize(old_length + usable_length);
  UChar* destination = buffer_.data() + old_length;
  StringImpl::CopyChars(destination, characters, usable_length);
  FoldQuoteMarksAndSoftHyphens(destination, usable_length);
  number_of_characters_just_appended_ = usable_length;
}

inline bool SearchBuffer::NeedsMoreContext() const {
  return needs_more_context_;
}

inline void SearchBuffer::PrependContext(const UChar* characters,
                                         size_t length) {
  DCHECK(needs_more_context_);
  DCHECK_EQ(prefix_length_, buffer_.size());

  if (!length)
    return;

  at_break_ = false;

  size_t word_boundary_context_start = length;
  if (word_boundary_context_start) {
    U16_BACK_1(characters, 0, word_boundary_context_start);
    word_boundary_context_start =
        StartOfLastWordBoundaryContext(characters, word_boundary_context_start);
  }

  size_t usable_length = std::min(buffer_.capacity() - prefix_length_,
                                  length - word_boundary_context_start);
  buffer_.push_front(characters + length - usable_length, usable_length);
  prefix_length_ += usable_length;

  if (word_boundary_context_start || prefix_length_ == buffer_.capacity())
    needs_more_context_ = false;
}

inline bool SearchBuffer::AtBreak() const {
  return at_break_;
}

inline void SearchBuffer::ReachedBreak() {
  at_break_ = true;
}

inline bool SearchBuffer::IsBadMatch(const UChar* match,
                                     size_t match_length) const {
  // This function implements the kana workaround. If usearch treats
  // it as a match, but we do not want to, then it's a "bad match".
  if (!target_requires_kana_workaround_)
    return false;

  // Normalize into a match buffer. We reuse a single buffer rather than
  // creating a new one each time.
  NormalizeCharactersIntoNFCForm(match, match_length, normalized_match_);

  return !CheckOnlyKanaLettersInStrings(
      normalized_target_.begin(), normalized_target_.size(),
      normalized_match_.begin(), normalized_match_.size());
}

inline bool SearchBuffer::IsWordStartMatch(size_t start, size_t length) const {
  DCHECK(options_ & kAtWordStarts);

  if (!start)
    return true;

  int size = buffer_.size();
  int offset = start;
  UChar32 first_character = GetCodePointAt(buffer_.data(), offset, size);

  if (options_ & kTreatMedialCapitalAsWordStart) {
    UChar32 previous_character;
    U16_PREV(buffer_.data(), 0, offset, previous_character);

    if (IsSeparator(first_character)) {
      // The start of a separator run is a word start (".org" in "webkit.org").
      if (!IsSeparator(previous_character))
        return true;
    } else if (IsASCIIUpper(first_character)) {
      // The start of an uppercase run is a word start ("Kit" in "WebKit").
      if (!IsASCIIUpper(previous_character))
        return true;
      // The last character of an uppercase run followed by a non-separator,
      // non-digit is a word start ("Request" in "XMLHTTPRequest").
      offset = start;
      U16_FWD_1(buffer_.data(), offset, size);
      UChar32 next_character = 0;
      if (offset < size)
        next_character = GetCodePointAt(buffer_.data(), offset, size);
      if (!IsASCIIUpper(next_character) && !IsASCIIDigit(next_character) &&
          !IsSeparator(next_character))
        return true;
    } else if (IsASCIIDigit(first_character)) {
      // The start of a digit run is a word start ("2" in "WebKit2").
      if (!IsASCIIDigit(previous_character))
        return true;
    } else if (IsSeparator(previous_character) ||
               IsASCIIDigit(previous_character)) {
      // The start of a non-separator, non-uppercase, non-digit run is a word
      // start, except after an uppercase. ("org" in "webkit.org", but not "ore"
      // in "WebCore").
      return true;
    }
  }

  // Chinese and Japanese lack word boundary marks, and there is no clear
  // agreement on what constitutes a word, so treat the position before any CJK
  // character as a word start.
  if (Character::IsCJKIdeographOrSymbol(first_character))
    return true;

  size_t word_break_search_start = start + length;
  while (word_break_search_start > start) {
    word_break_search_start = FindNextWordBackward(
        buffer_.data(), buffer_.size(), word_break_search_start);
  }
  if (word_break_search_start != start)
    return false;
  if (options_ & kWholeWord)
    return static_cast<int>(start + length) ==
           FindWordEndBoundary(buffer_.data(), buffer_.size(),
                               word_break_search_start);
  return true;
}

inline size_t SearchBuffer::Search(size_t& start) {
  size_t size = buffer_.size();
  if (at_break_) {
    if (!size)
      return 0;
  } else {
    if (size != buffer_.capacity())
      return 0;
  }

  text_searcher_->SetText(buffer_.data(), size);
  text_searcher_->SetOffset(prefix_length_);

  MatchResultICU match;

nextMatch:
  if (!text_searcher_->NextMatchResult(match))
    return 0;

  // Matches that start in the overlap area are only tentative.
  // The same match may appear later, matching more characters,
  // possibly including a combining character that's not yet in the buffer.
  if (!at_break_ && match.start >= size - overlap_) {
    size_t overlap = overlap_;
    if (options_ & kAtWordStarts) {
      // Ensure that there is sufficient context before matchStart the next time
      // around for determining if it is at a word boundary.
      int word_boundary_context_start = match.start;
      U16_BACK_1(buffer_.data(), 0, word_boundary_context_start);
      word_boundary_context_start = StartOfLastWordBoundaryContext(
          buffer_.data(), word_boundary_context_start);
      overlap = std::min(size - 1,
                         std::max(overlap, size - word_boundary_context_start));
    }
    memcpy(buffer_.data(), buffer_.data() + size - overlap,
           overlap * sizeof(UChar));
    prefix_length_ -= std::min(prefix_length_, size - overlap);
    buffer_.Shrink(overlap);
    return 0;
  }

  SECURITY_DCHECK(match.start + match.length <= size);

  // If this match is "bad", move on to the next match.
  if (IsBadMatch(buffer_.data() + match.start, match.length) ||
      ((options_ & kAtWordStarts) &&
       !IsWordStartMatch(match.start, match.length))) {
    goto nextMatch;
  }

  size_t new_size = size - (match.start + 1);
  memmove(buffer_.data(), buffer_.data() + match.start + 1,
          new_size * sizeof(UChar));
  prefix_length_ -= std::min<size_t>(prefix_length_, match.start + 1);
  buffer_.Shrink(new_size);

  start = size - match.start;
  return match.length;
}

// Check if there's any unpaird surrogate code point.
// Non-character code points are not checked.
static bool IsValidUTF16(const String& s) {
  if (s.Is8Bit())
    return true;
  const UChar* ustr = s.Characters16();
  size_t length = s.length();
  size_t position = 0;
  while (position < length) {
    UChar32 character;
    U16_NEXT(ustr, position, length, character);
    if (U_IS_SURROGATE(character))
      return false;
  }
  return true;
}

template <typename Strategy>
static size_t FindPlainTextInternal(CharacterIteratorAlgorithm<Strategy>& it,
                                    const String& target,
                                    FindOptions options,
                                    size_t& match_start) {
  match_start = 0;
  size_t match_length = 0;

  if (!IsValidUTF16(target))
    return 0;

  SearchBuffer buffer(target, options);

  if (buffer.NeedsMoreContext()) {
    for (SimplifiedBackwardsTextIteratorAlgorithm<Strategy> backwards_iterator(
             EphemeralRangeTemplate<Strategy>(
                 PositionTemplate<Strategy>::FirstPositionInNode(
                     it.OwnerDocument()),
                 PositionTemplate<Strategy>(it.CurrentContainer(),
                                            it.StartOffset())));
         !backwards_iterator.AtEnd(); backwards_iterator.Advance()) {
      BackwardsTextBuffer characters;
      backwards_iterator.CopyTextTo(&characters);
      buffer.PrependContext(characters.Data(), characters.Size());
      if (!buffer.NeedsMoreContext())
        break;
    }
  }

  while (!it.AtEnd()) {
    ForwardsTextBuffer characters;
    it.CopyTextTo(&characters);
    buffer.Append(characters.Data(), characters.Size());
    it.Advance(buffer.NumberOfCharactersJustAppended());
  tryAgain:
    size_t match_start_offset;
    if (size_t new_match_length = buffer.Search(match_start_offset)) {
      // Note that we found a match, and where we found it.
      size_t last_character_in_buffer_offset = it.CharacterOffset();
      DCHECK_GE(last_character_in_buffer_offset, match_start_offset);
      match_start = last_character_in_buffer_offset - match_start_offset;
      match_length = new_match_length;
      // If searching forward, stop on the first match.
      // If searching backward, don't stop, so we end up with the last match.
      if (!(options & kBackwards))
        break;
      goto tryAgain;
    }
    if (it.AtBreak() && !buffer.AtBreak()) {
      buffer.ReachedBreak();
      goto tryAgain;
    }
  }

  return match_length;
}

template <typename Strategy>
static EphemeralRangeTemplate<Strategy> FindPlainTextAlgorithm(
    const EphemeralRangeTemplate<Strategy>& input_range,
    const String& target,
    FindOptions options) {
  // CharacterIterator requires layoutObjects to be up to date.
  if (!input_range.StartPosition().IsConnected())
    return EphemeralRangeTemplate<Strategy>();
  DCHECK_EQ(input_range.StartPosition().GetDocument(),
            input_range.EndPosition().GetDocument());

  const TextIteratorBehavior& iterator_flags_for_find_plain_text =
      TextIteratorBehavior::Builder()
          .SetEntersTextControls(true)
          .SetEntersOpenShadowRoots(true)
          .SetDoesNotBreakAtReplacedElement(true)
          .SetCollapseTrailingSpace(true)
          .Build();

  // FIXME: Reduce the code duplication with above (but how?).
  size_t match_start;
  size_t match_length;
  {
    const TextIteratorBehavior& behavior =
        TextIteratorBehavior::Builder(iterator_flags_for_find_plain_text)
            .SetForWindowFind(options & kFindAPICall)
            .Build();
    CharacterIteratorAlgorithm<Strategy> find_iterator(input_range, behavior);
    match_length =
        FindPlainTextInternal(find_iterator, target, options, match_start);
    if (!match_length)
      return EphemeralRangeTemplate<Strategy>(options & kBackwards
                                                  ? input_range.StartPosition()
                                                  : input_range.EndPosition());
  }

  CharacterIteratorAlgorithm<Strategy> compute_range_iterator(
      input_range, iterator_flags_for_find_plain_text);
  return compute_range_iterator.CalculateCharacterSubrange(match_start,
                                                           match_length);
}

EphemeralRange FindPlainText(const EphemeralRange& input_range,
                             const String& target,
                             FindOptions options) {
  return FindPlainTextAlgorithm<EditingStrategy>(input_range, target, options);
}

EphemeralRangeInFlatTree FindPlainText(
    const EphemeralRangeInFlatTree& input_range,
    const String& target,
    FindOptions options) {
  return FindPlainTextAlgorithm<EditingInFlatTreeStrategy>(input_range, target,
                                                           options);
}

}  // namespace blink
