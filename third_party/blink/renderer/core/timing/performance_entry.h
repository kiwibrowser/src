/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_ENTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_ENTRY_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptState;
class ScriptValue;
class V8ObjectBuilder;

using PerformanceEntryType = unsigned;
using PerformanceEntryTypeMask = unsigned;

class CORE_EXPORT PerformanceEntry : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~PerformanceEntry() override;

  enum EntryType : PerformanceEntryType {
    kInvalid = 0,
    kNavigation = 1 << 0,
    kComposite = 1 << 1,
    kMark = 1 << 2,
    kMeasure = 1 << 3,
    kRender = 1 << 4,
    kResource = 1 << 5,
    kLongTask = 1 << 6,
    kTaskAttribution = 1 << 7,
    kPaint = 1 << 8,
    kEvent = 1 << 9,
    kFirstInput = 1 << 10,
  };

  String name() const;
  String entryType() const;
  DOMHighResTimeStamp startTime() const;
  // PerformanceNavigationTiming will override this due to
  // the nature of reporting it early, which means not having a
  // finish time available at construction time.
  // Other classes must NOT override this.
  virtual DOMHighResTimeStamp duration() const;

  ScriptValue toJSONForBinding(ScriptState*) const;

  PerformanceEntryType EntryTypeEnum() const { return entry_type_enum_; }

  bool IsResource() const { return entry_type_enum_ == kResource; }
  bool IsRender() const { return entry_type_enum_ == kRender; }
  bool IsComposite() const { return entry_type_enum_ == kComposite; }
  bool IsMark() const { return entry_type_enum_ == kMark; }
  bool IsMeasure() const { return entry_type_enum_ == kMeasure; }

  static bool StartTimeCompareLessThan(PerformanceEntry* a,
                                       PerformanceEntry* b) {
    if (a->startTime() == b->startTime())
      return a->index_ < b->index_;
    return a->startTime() < b->startTime();
  }

  static PerformanceEntry::EntryType ToEntryTypeEnum(const String& entry_type);

 protected:
  PerformanceEntry(const String& name,
                   const String& entry_type,
                   double start_time,
                   double finish_time);
  virtual void BuildJSONValue(V8ObjectBuilder&) const;

  // Protected and not const because PerformanceEventTiming needs to modify it.
  double duration_;

 private:
  const String name_;
  const String entry_type_;
  const double start_time_;
  const PerformanceEntryType entry_type_enum_;
  const int index_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_ENTRY_H_
