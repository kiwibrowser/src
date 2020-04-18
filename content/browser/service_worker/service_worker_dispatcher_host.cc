// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_dispatcher_host.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/bad_message.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_handle.h"
#include "content/browser/service_worker/service_worker_navigation_handle_core.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/origin_util.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_error_type.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_provider_type.mojom.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_error.h"
#include "url/gurl.h"

using blink::MessagePortChannel;
using blink::WebServiceWorkerError;

namespace content {

namespace {

const uint32_t kServiceWorkerFilteredMessageClasses[] = {
    ServiceWorkerMsgStart,
};

}  // namespace

ServiceWorkerDispatcherHost::ServiceWorkerDispatcherHost(int render_process_id)
    : BrowserMessageFilter(kServiceWorkerFilteredMessageClasses,
                           arraysize(kServiceWorkerFilteredMessageClasses)),
      BrowserAssociatedInterface<mojom::ServiceWorkerDispatcherHost>(this,
                                                                     this),
      render_process_id_(render_process_id),
      weak_ptr_factory_(this) {}

ServiceWorkerDispatcherHost::~ServiceWorkerDispatcherHost() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (GetContext() && phase_ == Phase::kAddedToContext)
    GetContext()->RemoveDispatcherHost(render_process_id_);
}

void ServiceWorkerDispatcherHost::Init(
    ServiceWorkerContextWrapper* context_wrapper) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerDispatcherHost::Init, this,
                       base::RetainedRef(context_wrapper)));
    return;
  }

  // Just speculating that maybe we were destructed before Init() was called on
  // the IO thread in order to try to fix https://crbug.com/750267.
  if (phase_ != Phase::kInitial)
    return;

  context_wrapper_ = context_wrapper;
  if (!GetContext())
    return;
  GetContext()->AddDispatcherHost(render_process_id_, this);
  phase_ = Phase::kAddedToContext;
}

void ServiceWorkerDispatcherHost::OnFilterRemoved() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Don't wait until the destructor to teardown since a new dispatcher host
  // for this process might be created before then.
  if (GetContext() && phase_ == Phase::kAddedToContext) {
    GetContext()->RemoveDispatcherHost(render_process_id_);
    weak_ptr_factory_.InvalidateWeakPtrs();
  }
  phase_ = Phase::kRemovedFromContext;
  context_wrapper_ = nullptr;
}

void ServiceWorkerDispatcherHost::OnDestruct() const {
  // Destruct on the IO thread since |context_wrapper_| should only be accessed
  // on the IO thread.
  BrowserThread::DeleteOnIOThread::Destruct(this);
}

bool ServiceWorkerDispatcherHost::OnMessageReceived(
    const IPC::Message& message) {
  return false;
}

base::WeakPtr<ServiceWorkerDispatcherHost>
ServiceWorkerDispatcherHost::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ServiceWorkerDispatcherHost::OnProviderCreated(
    ServiceWorkerProviderHostInfo info) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerDispatcherHost::OnProviderCreated");
  if (!GetContext())
    return;
  if (GetContext()->GetProviderHost(render_process_id_, info.provider_id)) {
    bad_message::ReceivedBadMessage(
        this, bad_message::SWDH_PROVIDER_CREATED_DUPLICATE_ID);
    return;
  }

  // Provider hosts for navigations are precreated on the browser process with a
  // browser-assigned id. The renderer process calls OnProviderCreated once it
  // creates the provider.
  if (ServiceWorkerUtils::IsBrowserAssignedProviderId(info.provider_id)) {
    if (info.type != blink::mojom::ServiceWorkerProviderType::kForWindow) {
      bad_message::ReceivedBadMessage(
          this, bad_message::SWDH_PROVIDER_CREATED_ILLEGAL_TYPE_NOT_WINDOW);
      return;
    }

    // Retrieve the provider host pre-created for the navigation.
    std::unique_ptr<ServiceWorkerProviderHost> provider_host =
        GetContext()->ReleaseProviderHost(ChildProcessHost::kInvalidUniqueID,
                                          info.provider_id);
    // If no host is found, create one.
    // TODO(crbug.com/789111#c14): This is probably not right, see bug.
    if (!provider_host) {
      GetContext()->AddProviderHost(ServiceWorkerProviderHost::Create(
          render_process_id_, std::move(info), GetContext()->AsWeakPtr(),
          AsWeakPtr()));
      return;
    }

    // Otherwise, complete initialization of the pre-created host.
    provider_host->CompleteNavigationInitialized(render_process_id_,
                                                 std::move(info), AsWeakPtr());
    GetContext()->AddProviderHost(std::move(provider_host));
    return;
  }

  // Provider hosts for service workers don't call OnProviderCreated. They are
  // precreated and ServiceWorkerProviderHost::CompleteStartWorkerPreparation is
  // called during the startup sequence once a process is allocated.
  if (info.type == blink::mojom::ServiceWorkerProviderType::kForServiceWorker) {
    bad_message::ReceivedBadMessage(
        this, bad_message::SWDH_PROVIDER_CREATED_ILLEGAL_TYPE_SERVICE_WORKER);
    return;
  }

  GetContext()->AddProviderHost(ServiceWorkerProviderHost::Create(
      render_process_id_, std::move(info), GetContext()->AsWeakPtr(),
      AsWeakPtr()));
}

ServiceWorkerContextCore* ServiceWorkerDispatcherHost::GetContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!context_wrapper_.get())
    return nullptr;
  return context_wrapper_->context();
}

}  // namespace content
