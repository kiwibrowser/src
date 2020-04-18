/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#include "third_party/blink/renderer/core/frame/dom_window_timers.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_gc_for_context_dispose.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/dom_timer.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/weborigin/security_violation_reporting_policy.h"

namespace blink {

namespace DOMWindowTimers {

static bool IsAllowed(ScriptState* script_state,
                      ExecutionContext* execution_context,
                      bool is_eval,
                      const String& source) {
  if (execution_context->IsDocument()) {
    Document* document = static_cast<Document*>(execution_context);
    if (!document->GetFrame())
      return false;
    if (is_eval && !document->GetContentSecurityPolicy()->AllowEval(
                       script_state, SecurityViolationReportingPolicy::kReport,
                       ContentSecurityPolicy::kWillNotThrowException, source)) {
      return false;
    }
    return true;
  }
  if (execution_context->IsWorkerGlobalScope()) {
    WorkerGlobalScope* worker_global_scope =
        static_cast<WorkerGlobalScope*>(execution_context);
    if (!worker_global_scope->ScriptController())
      return false;
    ContentSecurityPolicy* policy =
        worker_global_scope->GetContentSecurityPolicy();
    if (is_eval && policy &&
        !policy->AllowEval(
            script_state, SecurityViolationReportingPolicy::kReport,
            ContentSecurityPolicy::kWillNotThrowException, source)) {
      return false;
    }
    return true;
  }
  NOTREACHED();
  return false;
}

int setTimeout(ScriptState* script_state,
               EventTarget& event_target,
               const ScriptValue& handler,
               int timeout,
               const Vector<ScriptValue>& arguments) {
  ExecutionContext* execution_context = event_target.GetExecutionContext();
  if (!IsAllowed(script_state, execution_context, false, g_empty_string))
    return 0;
  if (timeout >= 0 && execution_context->IsDocument()) {
    // FIXME: Crude hack that attempts to pass idle time to V8. This should
    // be done using the scheduler instead.
    V8GCForContextDispose::Instance().NotifyIdle();
  }
  ScheduledAction* action = ScheduledAction::Create(
      script_state, execution_context, handler, arguments);
  return DOMTimer::Install(execution_context, action,
                           TimeDelta::FromMilliseconds(timeout), true);
}

int setTimeout(ScriptState* script_state,
               EventTarget& event_target,
               const String& handler,
               int timeout,
               const Vector<ScriptValue>&) {
  ExecutionContext* execution_context = event_target.GetExecutionContext();
  if (!IsAllowed(script_state, execution_context, true, handler))
    return 0;
  // Don't allow setting timeouts to run empty functions.  Was historically a
  // perfomance issue.
  if (handler.IsEmpty())
    return 0;
  if (timeout >= 0 && execution_context->IsDocument()) {
    // FIXME: Crude hack that attempts to pass idle time to V8. This should
    // be done using the scheduler instead.
    V8GCForContextDispose::Instance().NotifyIdle();
  }
  ScheduledAction* action =
      ScheduledAction::Create(script_state, execution_context, handler);
  return DOMTimer::Install(execution_context, action,
                           TimeDelta::FromMilliseconds(timeout), true);
}

int setInterval(ScriptState* script_state,
                EventTarget& event_target,
                const ScriptValue& handler,
                int timeout,
                const Vector<ScriptValue>& arguments) {
  ExecutionContext* execution_context = event_target.GetExecutionContext();
  if (!IsAllowed(script_state, execution_context, false, g_empty_string))
    return 0;
  ScheduledAction* action = ScheduledAction::Create(
      script_state, execution_context, handler, arguments);
  return DOMTimer::Install(execution_context, action,
                           TimeDelta::FromMilliseconds(timeout), false);
}

int setInterval(ScriptState* script_state,
                EventTarget& event_target,
                const String& handler,
                int timeout,
                const Vector<ScriptValue>&) {
  ExecutionContext* execution_context = event_target.GetExecutionContext();
  if (!IsAllowed(script_state, execution_context, true, handler))
    return 0;
  // Don't allow setting timeouts to run empty functions.  Was historically a
  // perfomance issue.
  if (handler.IsEmpty())
    return 0;
  ScheduledAction* action =
      ScheduledAction::Create(script_state, execution_context, handler);
  return DOMTimer::Install(execution_context, action,
                           TimeDelta::FromMilliseconds(timeout), false);
}

void clearTimeout(EventTarget& event_target, int timeout_id) {
  if (ExecutionContext* context = event_target.GetExecutionContext())
    DOMTimer::RemoveByID(context, timeout_id);
}

void clearInterval(EventTarget& event_target, int timeout_id) {
  if (ExecutionContext* context = event_target.GetExecutionContext())
    DOMTimer::RemoveByID(context, timeout_id);
}

}  // namespace DOMWindowTimers

}  // namespace blink
