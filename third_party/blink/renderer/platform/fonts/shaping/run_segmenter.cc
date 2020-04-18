// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/shaping/run_segmenter.h"

#include <memory>

#include "third_party/blink/renderer/platform/fonts/script_run_iterator.h"
#include "third_party/blink/renderer/platform/fonts/small_caps_iterator.h"
#include "third_party/blink/renderer/platform/fonts/symbols_iterator.h"
#include "third_party/blink/renderer/platform/fonts/utf16_text_iterator.h"
#include "third_party/blink/renderer/platform/text/character.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

RunSegmenter::RunSegmenter(const UChar* buffer,
                           unsigned buffer_size,
                           FontOrientation run_orientation)
    : buffer_size_(buffer_size),
      candidate_range_(NullRange()),
      script_run_iterator_(
          std::make_unique<ScriptRunIterator>(buffer, buffer_size)),
      orientation_iterator_(
          run_orientation == FontOrientation::kVerticalMixed
              ? std::make_unique<OrientationIterator>(buffer,
                                                      buffer_size,
                                                      run_orientation)
              : nullptr),
      symbols_iterator_(std::make_unique<SymbolsIterator>(buffer, buffer_size)),
      last_split_(0),
      script_run_iterator_position_(0),
      orientation_iterator_position_(
          run_orientation == FontOrientation::kVerticalMixed ? 0
                                                             : buffer_size_),
      symbols_iterator_position_(0),
      at_end_(!buffer_size_) {}

void RunSegmenter::ConsumeScriptIteratorPastLastSplit() {
  DCHECK(script_run_iterator_);
  if (script_run_iterator_position_ <= last_split_ &&
      script_run_iterator_position_ < buffer_size_) {
    while (script_run_iterator_->Consume(script_run_iterator_position_,
                                         candidate_range_.script)) {
      if (script_run_iterator_position_ > last_split_)
        return;
    }
  }
}

void RunSegmenter::ConsumeOrientationIteratorPastLastSplit() {
  if (orientation_iterator_ && orientation_iterator_position_ <= last_split_ &&
      orientation_iterator_position_ < buffer_size_) {
    while (
        orientation_iterator_->Consume(&orientation_iterator_position_,
                                       &candidate_range_.render_orientation)) {
      if (orientation_iterator_position_ > last_split_)
        return;
    }
  }
}

void RunSegmenter::ConsumeSymbolsIteratorPastLastSplit() {
  DCHECK(symbols_iterator_);
  if (symbols_iterator_position_ <= last_split_ &&
      symbols_iterator_position_ < buffer_size_) {
    while (
        symbols_iterator_->Consume(&symbols_iterator_position_,
                                   &candidate_range_.font_fallback_priority)) {
      if (symbols_iterator_position_ > last_split_)
        return;
    }
  }
}

bool RunSegmenter::Supports(FontOrientation orientation) const {
  bool needs_orientation_iterator =
      orientation == FontOrientation::kVerticalMixed;
  return (needs_orientation_iterator == !!orientation_iterator_.get());
}

bool RunSegmenter::HasProgressedPast(unsigned start) const {
  return candidate_range_.start > start;
}

bool RunSegmenter::ConsumePast(unsigned start, RunSegmenterRange* next_range) {
  DCHECK(!HasProgressedPast(start));
  while (true) {
    // Check the candidate range first, before checking |at_end_| because
    // |Consume()| sets |at_end_| even when the current range is valid.
    if (candidate_range_.start <= start && start < candidate_range_.end) {
      *next_range = candidate_range_;
      return true;
    }

    if (!Consume())
      return false;
  }
}

// Consume the input until the next range. Returns false if no more ranges are
// available.
bool RunSegmenter::Consume() {
  if (at_end_)
    return false;

  ConsumeScriptIteratorPastLastSplit();
  ConsumeOrientationIteratorPastLastSplit();
  ConsumeSymbolsIteratorPastLastSplit();

  if (script_run_iterator_position_ <= orientation_iterator_position_ &&
      script_run_iterator_position_ <= symbols_iterator_position_) {
    last_split_ = script_run_iterator_position_;
  }

  if (orientation_iterator_position_ <= script_run_iterator_position_ &&
      orientation_iterator_position_ <= symbols_iterator_position_) {
    last_split_ = orientation_iterator_position_;
  }

  if (symbols_iterator_position_ <= script_run_iterator_position_ &&
      symbols_iterator_position_ <= orientation_iterator_position_) {
    last_split_ = symbols_iterator_position_;
  }

  candidate_range_.start = candidate_range_.end;
  candidate_range_.end = last_split_;

  at_end_ = last_split_ == buffer_size_;
  return true;
}

}  // namespace blink
