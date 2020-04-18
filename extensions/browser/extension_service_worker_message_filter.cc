// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_service_worker_message_filter.h"

#include "content/public/browser/service_worker_context.h"
#include "extensions/browser/bad_message.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/common/extension_messages.h"

namespace extensions {

ExtensionServiceWorkerMessageFilter::ExtensionServiceWorkerMessageFilter(
    int render_process_id,
    content::BrowserContext* context,
    content::ServiceWorkerContext* service_worker_context)
    : content::BrowserMessageFilter(ExtensionWorkerMsgStart),
      render_process_id_(render_process_id),
      service_worker_context_(service_worker_context),
      dispatcher_(new ExtensionFunctionDispatcher(context)) {}

ExtensionServiceWorkerMessageFilter::~ExtensionServiceWorkerMessageFilter() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

void ExtensionServiceWorkerMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    content::BrowserThread::ID* thread) {
  if (message.type() == ExtensionHostMsg_RequestWorker::ID) {
    *thread = content::BrowserThread::UI;
  }
}

bool ExtensionServiceWorkerMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ExtensionServiceWorkerMessageFilter, message)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_RequestWorker, OnRequestWorker)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_IncrementServiceWorkerActivity,
                        OnIncrementServiceWorkerActivity)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_DecrementServiceWorkerActivity,
                        OnDecrementServiceWorkerActivity)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ExtensionServiceWorkerMessageFilter::OnRequestWorker(
    const ExtensionHostMsg_Request_Params& params) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  dispatcher_->Dispatch(params, nullptr, render_process_id_);
}

void ExtensionServiceWorkerMessageFilter::OnIncrementServiceWorkerActivity(
    int64_t service_worker_version_id,
    const std::string& request_uuid) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // The worker might have already stopped before we got here, so the increment
  // below might fail legitimately. Therefore, we do not send bad_message to the
  // worker even if it fails.
  service_worker_context_->StartingExternalRequest(service_worker_version_id,
                                                   request_uuid);
}

void ExtensionServiceWorkerMessageFilter::OnDecrementServiceWorkerActivity(
    int64_t service_worker_version_id,
    const std::string& request_uuid) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  bool status = service_worker_context_->FinishedExternalRequest(
      service_worker_version_id, request_uuid);
  if (!status) {
    bad_message::ReceivedBadMessage(
        this, bad_message::ESWMF_INVALID_DECREMENT_ACTIVITY);
  }
}

}  // namespace extensions
