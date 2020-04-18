// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/abort_signal.h"

#include <utility>

#include "base/callback.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

AbortSignal::AbortSignal(ExecutionContext* execution_context)
    : execution_context_(execution_context) {}
AbortSignal::~AbortSignal() = default;

const AtomicString& AbortSignal::InterfaceName() const {
  return EventTargetNames::AbortSignal;
}

ExecutionContext* AbortSignal::GetExecutionContext() const {
  return execution_context_.Get();
}

void AbortSignal::AddAlgorithm(base::OnceClosure algorithm) {
  if (aborted_flag_)
    return;
  abort_algorithms_.push_back(std::move(algorithm));
}

void AbortSignal::SignalAbort() {
  if (aborted_flag_)
    return;
  aborted_flag_ = true;
  for (base::OnceClosure& closure : abort_algorithms_) {
    std::move(closure).Run();
  }
  abort_algorithms_.clear();
  DispatchEvent(Event::Create(EventTypeNames::abort));
}

void AbortSignal::Follow(AbortSignal* parentSignal) {
  if (aborted_flag_)
    return;
  if (parentSignal->aborted_flag_)
    SignalAbort();

  // Unlike the usual practice for AddAlgorithm(), we don't use a weak pointer
  // here. To see why, consider the following object graph:
  //
  // controller --owns--> signal1 -?-> signal2 -?-> signal3 <--owns-- request
  //
  // It's easy to create chained signals like this using the Request
  // constructor. If the -?-> pointers were weak, then |signal2| could be
  // collected even though |controller| and |request| were still
  // referenced. This would prevent controller.abort() from working. So the
  // pointers need to be strong. This won't artificially extend the lifetime of
  // |request|, because the pointer to it in the closure held by |signal3| is
  // still weak.
  parentSignal->AddAlgorithm(
      WTF::Bind(&AbortSignal::SignalAbort, WrapPersistent(this)));
}

void AbortSignal::Trace(Visitor* visitor) {
  visitor->Trace(execution_context_);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
