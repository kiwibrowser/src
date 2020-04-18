// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_timing.h"

#include "third_party/blink/renderer/core/paint/paint_timing.h"

namespace blink {

// static
const char CSSTiming::kSupplementName[] = "CSSTiming";

CSSTiming& CSSTiming::From(Document& document) {
  CSSTiming* timing = Supplement<Document>::From<CSSTiming>(document);
  if (!timing) {
    timing = new CSSTiming(document);
    ProvideTo(document, timing);
  }
  return *timing;
}

void CSSTiming::RecordAuthorStyleSheetParseTime(double seconds) {
  if (paint_timing_->FirstContentfulPaint().is_null())
    parse_time_before_fcp_ += seconds;
}

void CSSTiming::RecordUpdateDuration(double seconds) {
  if (paint_timing_->FirstContentfulPaint().is_null())
    update_time_before_fcp_ += seconds;
}

void CSSTiming::Trace(blink::Visitor* visitor) {
  visitor->Trace(paint_timing_);
  Supplement<Document>::Trace(visitor);
}

CSSTiming::CSSTiming(Document& document)
    : Supplement<Document>(document),
      paint_timing_(PaintTiming::From(document)) {}

}  // namespace blink
