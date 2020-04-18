// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/threaded_object_proxy_base.h"

#include <memory>
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/workers/parent_execution_context_task_runners.h"
#include "third_party/blink/renderer/core/workers/threaded_messaging_proxy_base.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

void ThreadedObjectProxyBase::CountFeature(WebFeature feature) {
  PostCrossThreadTask(
      *GetParentExecutionContextTaskRunners()->Get(TaskType::kInternalDefault),
      FROM_HERE,
      CrossThreadBind(&ThreadedMessagingProxyBase::CountFeature,
                      MessagingProxyWeakPtr(), feature));
}

void ThreadedObjectProxyBase::CountDeprecation(WebFeature feature) {
  PostCrossThreadTask(
      *GetParentExecutionContextTaskRunners()->Get(TaskType::kInternalDefault),
      FROM_HERE,
      CrossThreadBind(&ThreadedMessagingProxyBase::CountDeprecation,
                      MessagingProxyWeakPtr(), feature));
}

void ThreadedObjectProxyBase::ReportConsoleMessage(MessageSource source,
                                                   MessageLevel level,
                                                   const String& message,
                                                   SourceLocation* location) {
  PostCrossThreadTask(
      *GetParentExecutionContextTaskRunners()->Get(TaskType::kInternalDefault),
      FROM_HERE,
      CrossThreadBind(&ThreadedMessagingProxyBase::ReportConsoleMessage,
                      MessagingProxyWeakPtr(), source, level, message,
                      WTF::Passed(location->Clone())));
}

void ThreadedObjectProxyBase::PostMessageToPageInspector(
    int session_id,
    const String& message) {
  // The TaskType of Inspector tasks need to be Unthrottled because they need to
  // run even on a suspended page.
  PostCrossThreadTask(
      *GetParentExecutionContextTaskRunners()->Get(
          TaskType::kInternalInspector),
      FROM_HERE,
      CrossThreadBind(&ThreadedMessagingProxyBase::PostMessageToPageInspector,
                      MessagingProxyWeakPtr(), session_id, message));
}

void ThreadedObjectProxyBase::DidCloseWorkerGlobalScope() {
  PostCrossThreadTask(
      *GetParentExecutionContextTaskRunners()->Get(TaskType::kInternalDefault),
      FROM_HERE,
      CrossThreadBind(&ThreadedMessagingProxyBase::TerminateGlobalScope,
                      MessagingProxyWeakPtr()));
}

void ThreadedObjectProxyBase::DidTerminateWorkerThread() {
  // This will terminate the MessagingProxy.
  PostCrossThreadTask(
      *GetParentExecutionContextTaskRunners()->Get(TaskType::kInternalDefault),
      FROM_HERE,
      CrossThreadBind(&ThreadedMessagingProxyBase::WorkerThreadTerminated,
                      MessagingProxyWeakPtr()));
}

ParentExecutionContextTaskRunners*
ThreadedObjectProxyBase::GetParentExecutionContextTaskRunners() {
  return parent_execution_context_task_runners_.Get();
}

ThreadedObjectProxyBase::ThreadedObjectProxyBase(
    ParentExecutionContextTaskRunners* parent_execution_context_task_runners)
    : parent_execution_context_task_runners_(
          parent_execution_context_task_runners) {}

}  // namespace blink
