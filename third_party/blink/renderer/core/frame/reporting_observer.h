// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORTING_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORTING_OBSERVER_H_

#include "third_party/blink/renderer/bindings/core/v8/v8_reporting_observer_callback.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ExecutionContext;
class Report;

class CORE_EXPORT ReportingObserver final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ReportingObserver* Create(ExecutionContext*,
                                   V8ReportingObserverCallback*);

  // Call the callback with reports.
  void ReportToCallback(const HeapVector<Member<Report>>& reports);

  void observe();
  void disconnect();

  void Trace(blink::Visitor*) override;

 private:
  explicit ReportingObserver(ExecutionContext*, V8ReportingObserverCallback*);

  Member<ExecutionContext> execution_context_;
  Member<V8ReportingObserverCallback> callback_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORTING_OBSERVER_H_
