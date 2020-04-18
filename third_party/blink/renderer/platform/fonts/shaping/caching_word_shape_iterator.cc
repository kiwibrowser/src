// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/shaping/caching_word_shape_iterator.h"

#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"

namespace blink {

scoped_refptr<const ShapeResult> CachingWordShapeIterator::ShapeWordWithoutSpacing(
    const TextRun& word_run,
    const Font* font) {
  ShapeCacheEntry* cache_entry = shape_cache_->Add(word_run, ShapeCacheEntry());
  if (cache_entry && cache_entry->shape_result_)
    return cache_entry->shape_result_;

  unsigned word_length = 0;
  std::unique_ptr<UChar[]> word_text = word_run.NormalizedUTF16(&word_length);

  HarfBuzzShaper shaper(word_text.get(), word_length);
  scoped_refptr<const ShapeResult> shape_result =
      shaper.Shape(font, word_run.Direction());
  if (!shape_result)
    return nullptr;

  if (cache_entry)
    cache_entry->shape_result_ = shape_result;

  return shape_result;
}

}  // namespace blink
