// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SYMBOLS_ITERATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SYMBOLS_ITERATOR_H_

#include <memory>
#include "third_party/blink/renderer/platform/fonts/font_fallback_priority.h"
#include "third_party/blink/renderer/platform/fonts/font_orientation.h"
#include "third_party/blink/renderer/platform/fonts/script_run_iterator.h"
#include "third_party/blink/renderer/platform/fonts/utf16_text_iterator.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class PLATFORM_EXPORT SymbolsIterator {
  USING_FAST_MALLOC(SymbolsIterator);
  WTF_MAKE_NONCOPYABLE(SymbolsIterator);

 public:
  SymbolsIterator(const UChar* buffer, unsigned buffer_size);

  bool Consume(unsigned* symbols_limit, FontFallbackPriority*);

 private:
  FontFallbackPriority FontFallbackPriorityForCharacter(UChar32);

  std::unique_ptr<UTF16TextIterator> utf16_iterator_;
  unsigned buffer_size_;
  UChar32 next_char_;
  bool at_end_;

  FontFallbackPriority current_font_fallback_priority_;
  FontFallbackPriority previous_font_fallback_priority_;
};

}  // namespace blink

#endif
