// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_TIMING_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class PaintTiming;

class CSSTiming : public GarbageCollectedFinalized<CSSTiming>,
                  public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(CSSTiming);

 public:
  static const char kSupplementName[];

  virtual ~CSSTiming() = default;

  void RecordAuthorStyleSheetParseTime(double seconds);
  void RecordUpdateDuration(double seconds);

  double AuthorStyleSheetParseDurationBeforeFCP() const {
    return parse_time_before_fcp_;
  }

  double UpdateDurationBeforeFCP() const { return update_time_before_fcp_; }

  static CSSTiming& From(Document&);
  void Trace(blink::Visitor*) override;

 private:
  explicit CSSTiming(Document&);

  double parse_time_before_fcp_ = 0;
  double update_time_before_fcp_ = 0;

  Member<PaintTiming> paint_timing_;
  DISALLOW_COPY_AND_ASSIGN(CSSTiming);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_TIMING_H_
