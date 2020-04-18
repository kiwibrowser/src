// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/symbols_iterator.h"

#include <unicode/uchar.h>
#include <unicode/uniset.h>
#include <memory>

namespace blink {

SymbolsIterator::SymbolsIterator(const UChar* buffer, unsigned buffer_size)
    : utf16_iterator_(std::make_unique<UTF16TextIterator>(buffer, buffer_size)),
      buffer_size_(buffer_size),
      next_char_(0),
      at_end_(buffer_size == 0),
      current_font_fallback_priority_(FontFallbackPriority::kInvalid) {}

FontFallbackPriority SymbolsIterator::FontFallbackPriorityForCharacter(
    UChar32 codepoint) {
  // Those should only be Emoji presentation as combinations of two.
  if (Character::IsEmojiKeycapBase(codepoint) ||
      Character::IsRegionalIndicator(codepoint))
    return FontFallbackPriority::kText;

  if (codepoint == kCombiningEnclosingKeycapCharacter)
    return FontFallbackPriority::kEmojiEmoji;

  if (Character::IsEmojiEmojiDefault(codepoint) ||
      Character::IsEmojiModifierBase(codepoint) ||
      Character::IsModifier(codepoint))
    return FontFallbackPriority::kEmojiEmoji;

  if (Character::IsEmojiTextDefault(codepoint))
    return FontFallbackPriority::kEmojiText;

  // Here we could segment into Symbols and Math categories as well, similar
  // to what the Windows font fallback does. Map the math Unicode and Symbols
  // blocks to Text for now since we don't have a good cross-platform way to
  // select suitable math fonts.
  return FontFallbackPriority::kText;
}

bool SymbolsIterator::Consume(unsigned* symbols_limit,
                              FontFallbackPriority* font_fallback_priority) {
  if (at_end_)
    return false;

  while (utf16_iterator_->Consume(next_char_)) {
    previous_font_fallback_priority_ = current_font_fallback_priority_;
    unsigned iterator_offset = utf16_iterator_->Offset();
    utf16_iterator_->Advance();

    // Except at the beginning, ZWJ just carries over the emoji or neutral
    // text type, VS15 & VS16 we just carry over as well, since we already
    // resolved those through lookahead. Also, don't downgrade to text
    // presentation for emoji that are part of a ZWJ sequence, example
    // U+1F441 U+200D U+1F5E8, eye (text presentation) + ZWJ + left speech
    // bubble, see below.
    if ((!(next_char_ == kZeroWidthJoinerCharacter &&
           previous_font_fallback_priority_ ==
               FontFallbackPriority::kEmojiEmoji) &&
         next_char_ != kVariationSelector15Character &&
         next_char_ != kVariationSelector16Character &&
         next_char_ != kCombiningEnclosingCircleBackslashCharacter &&
         !Character::IsRegionalIndicator(next_char_) &&
         !((next_char_ == kLeftSpeechBubbleCharacter ||
            next_char_ == kRainbowCharacter ||
            next_char_ == kMaleSignCharacter ||
            next_char_ == kFemaleSignCharacter ||
            next_char_ == kStaffOfAesculapiusCharacter) &&
           previous_font_fallback_priority_ ==
               FontFallbackPriority::kEmojiEmoji) &&
         !Character::IsEmojiFlagSequenceTag(next_char_)) ||
        current_font_fallback_priority_ == FontFallbackPriority::kInvalid) {
      current_font_fallback_priority_ =
          FontFallbackPriorityForCharacter(next_char_);
    }

    UChar32 peek_char = 0;
    if (utf16_iterator_->Consume(peek_char) && peek_char != 0) {
      // Variation Selectors
      if (current_font_fallback_priority_ ==
              FontFallbackPriority::kEmojiEmoji &&
          peek_char == kVariationSelector15Character) {
        current_font_fallback_priority_ = FontFallbackPriority::kEmojiText;
      }

      if ((current_font_fallback_priority_ ==
               FontFallbackPriority::kEmojiText ||
           Character::IsEmojiKeycapBase(next_char_)) &&
          peek_char == kVariationSelector16Character) {
        current_font_fallback_priority_ = FontFallbackPriority::kEmojiEmoji;
      }

      // Combining characters Keycap...
      if (Character::IsEmojiKeycapBase(next_char_) &&
          peek_char == kCombiningEnclosingKeycapCharacter) {
        current_font_fallback_priority_ = FontFallbackPriority::kEmojiEmoji;
      };

      // Regional indicators
      if (Character::IsRegionalIndicator(next_char_) &&
          Character::IsRegionalIndicator(peek_char)) {
        current_font_fallback_priority_ = FontFallbackPriority::kEmojiEmoji;
      }

      // Upgrade text presentation emoji to emoji presentation when followed by
      // ZWJ, Example U+1F441 U+200D U+1F5E8, eye + ZWJ + left speech bubble.
      if ((next_char_ == kEyeCharacter ||
           next_char_ == kWavingWhiteFlagCharacter) &&
          peek_char == kZeroWidthJoinerCharacter) {
        current_font_fallback_priority_ = FontFallbackPriority::kEmojiEmoji;
      }
    }

    if (previous_font_fallback_priority_ != current_font_fallback_priority_ &&
        (previous_font_fallback_priority_ != FontFallbackPriority::kInvalid)) {
      *symbols_limit = iterator_offset;
      *font_fallback_priority = previous_font_fallback_priority_;
      return true;
    }
  }
  *symbols_limit = buffer_size_;
  *font_fallback_priority = current_font_fallback_priority_;
  at_end_ = true;
  return true;
}

}  // namespace blink
