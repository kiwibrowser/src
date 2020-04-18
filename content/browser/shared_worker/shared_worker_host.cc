// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_host.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/unguessable_token.h"
#include "content/browser/devtools/shared_worker_devtools_manager.h"
#include "content/browser/interface_provider_filtering.h"
#include "content/browser/renderer_interface_binders.h"
#include "content/browser/shared_worker/shared_worker_content_settings_proxy_impl.h"
#include "content/browser/shared_worker/shared_worker_instance.h"
#include "content/browser/shared_worker/shared_worker_service_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_client.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"
#include "third_party/blink/public/platform/web_feature.mojom.h"
#include "third_party/blink/public/web/worker_content_settings_proxy.mojom.h"

namespace content {
namespace {

void AllowFileSystemOnIOThreadResponse(base::OnceCallback<void(bool)> callback,
                                       bool result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(std::move(callback), result));
}

void AllowFileSystemOnIOThread(const GURL& url,
                               ResourceContext* resource_context,
                               std::vector<std::pair<int, int>> render_frames,
                               base::OnceCallback<void(bool)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GetContentClient()->browser()->AllowWorkerFileSystem(
      url, resource_context, render_frames,
      base::Bind(&AllowFileSystemOnIOThreadResponse, base::Passed(&callback)));
}

bool AllowIndexedDBOnIOThread(const GURL& url,
                              const base::string16& name,
                              ResourceContext* resource_context,
                              std::vector<std::pair<int, int>> render_frames) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return GetContentClient()->browser()->AllowWorkerIndexedDB(
      url, name, resource_context, render_frames);
}

}  // namespace

// RAII helper class for talking to SharedWorkerDevToolsManager.
class SharedWorkerHost::ScopedDevToolsHandle {
 public:
  ScopedDevToolsHandle(SharedWorkerHost* owner,
                       bool* out_pause_on_start,
                       base::UnguessableToken* out_devtools_worker_token)
      : owner_(owner) {
    SharedWorkerDevToolsManager::GetInstance()->WorkerCreated(
        owner, out_pause_on_start, out_devtools_worker_token);
  }

  ~ScopedDevToolsHandle() {
    SharedWorkerDevToolsManager::GetInstance()->WorkerDestroyed(owner_);
  }

  void WorkerReadyForInspection() {
    SharedWorkerDevToolsManager::GetInstance()->WorkerReadyForInspection(
        owner_);
  }

 private:
  SharedWorkerHost* owner_;
  DISALLOW_COPY_AND_ASSIGN(ScopedDevToolsHandle);
};

SharedWorkerHost::SharedWorkerHost(
    SharedWorkerServiceImpl* service,
    std::unique_ptr<SharedWorkerInstance> instance,
    int process_id)
    : binding_(this),
      service_(service),
      instance_(std::move(instance)),
      process_id_(process_id),
      next_connection_request_id_(1),
      creation_time_(base::TimeTicks::Now()),
      interface_provider_binding_(this),
      weak_factory_(this) {
  DCHECK(instance_);
  // Set up the worker interface request. This is needed first in either
  // AddClient() or Start(). AddClient() can sometimes be called before Start()
  // when two clients call new SharedWorker() at around the same time.
  worker_request_ = mojo::MakeRequest(&worker_);
}

SharedWorkerHost::~SharedWorkerHost() {
  UMA_HISTOGRAM_LONG_TIMES("SharedWorker.TimeToDeleted",
                           base::TimeTicks::Now() - creation_time_);
  switch (phase_) {
    case Phase::kInitial:
      // Tell clients that this worker failed to start. This is only needed in
      // kInitial. Once in kStarted, the worker in the renderer would alert this
      // host if script loading failed.
      for (const ClientInfo& info : clients_)
        info.client->OnScriptLoadFailed();
      break;
    case Phase::kStarted:
    case Phase::kClosed:
    case Phase::kTerminationSent:
    case Phase::kTerminationSentAndClosed:
      break;
  }
}

void SharedWorkerHost::Start(
    mojom::SharedWorkerFactoryPtr factory,
    mojom::ServiceWorkerProviderInfoForSharedWorkerPtr
        service_worker_provider_info,
    network::mojom::URLLoaderFactoryAssociatedPtrInfo script_loader_factory) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  AdvanceTo(Phase::kStarted);

  mojom::SharedWorkerInfoPtr info(mojom::SharedWorkerInfo::New(
      instance_->url(), instance_->name(), instance_->content_security_policy(),
      instance_->content_security_policy_type(),
      instance_->creation_address_space()));

  // Register with DevTools.
  bool pause_on_start;
  base::UnguessableToken devtools_worker_token;
  devtools_handle_ = std::make_unique<ScopedDevToolsHandle>(
      this, &pause_on_start, &devtools_worker_token);

  // Set up content settings interface.
  blink::mojom::WorkerContentSettingsProxyPtr content_settings;
  content_settings_ = std::make_unique<SharedWorkerContentSettingsProxyImpl>(
      instance_->url(), this, mojo::MakeRequest(&content_settings));

  // Set up host interface.
  mojom::SharedWorkerHostPtr host;
  binding_.Bind(mojo::MakeRequest(&host));

  // Set up interface provider interface.
  service_manager::mojom::InterfaceProviderPtr interface_provider;
  interface_provider_binding_.Bind(FilterRendererExposedInterfaces(
      mojom::kNavigation_SharedWorkerSpec, process_id_,
      mojo::MakeRequest(&interface_provider)));

  // Send the CreateSharedWorker message.
  factory_ = std::move(factory);
  factory_->CreateSharedWorker(
      std::move(info), pause_on_start, devtools_worker_token,
      std::move(content_settings), std::move(service_worker_provider_info),
      std::move(script_loader_factory), std::move(host),
      std::move(worker_request_), std::move(interface_provider));

  // Monitor the lifetime of the worker.
  worker_.set_connection_error_handler(base::BindOnce(
      &SharedWorkerHost::OnWorkerConnectionLost, weak_factory_.GetWeakPtr()));
}

void SharedWorkerHost::AllowFileSystem(
    const GURL& url,
    base::OnceCallback<void(bool)> callback) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AllowFileSystemOnIOThread, url,
                     RenderProcessHost::FromID(process_id_)
                         ->GetBrowserContext()
                         ->GetResourceContext(),
                     GetRenderFrameIDsForWorker(), std::move(callback)));
}

void SharedWorkerHost::AllowIndexedDB(const GURL& url,
                                      const base::string16& name,
                                      base::OnceCallback<void(bool)> callback) {
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AllowIndexedDBOnIOThread, url, name,
                     RenderProcessHost::FromID(process_id_)
                         ->GetBrowserContext()
                         ->GetResourceContext(),
                     GetRenderFrameIDsForWorker()),
      std::move(callback));
}

void SharedWorkerHost::TerminateWorker() {
  switch (phase_) {
    case Phase::kInitial:
      // The host is being asked to terminate the worker before it started.
      // Tell clients that this worker failed to start.
      for (const ClientInfo& info : clients_)
        info.client->OnScriptLoadFailed();
      // Tell the caller it terminated, so the caller doesn't wait forever.
      AdvanceTo(Phase::kTerminationSentAndClosed);
      OnWorkerConnectionLost();
      // |this| is destroyed here.
      return;
    case Phase::kStarted:
      AdvanceTo(Phase::kTerminationSent);
      break;
    case Phase::kClosed:
      AdvanceTo(Phase::kTerminationSentAndClosed);
      break;
    case Phase::kTerminationSent:
    case Phase::kTerminationSentAndClosed:
      // Termination was already sent. TerminateWorker can be called twice in
      // tests while cleaning up all the workers.
      return;
  }

  devtools_handle_.reset();
  worker_->Terminate();
  // Now, we wait to observe OnWorkerConnectionLost.
}

void SharedWorkerHost::AdvanceTo(Phase phase) {
  switch (phase_) {
    case Phase::kInitial:
      DCHECK(phase == Phase::kStarted ||
             phase == Phase::kTerminationSentAndClosed);
      break;
    case Phase::kStarted:
      DCHECK(phase == Phase::kClosed || phase == Phase::kTerminationSent);
      break;
    case Phase::kClosed:
      DCHECK(phase == Phase::kTerminationSentAndClosed);
      break;
    case Phase::kTerminationSent:
      DCHECK(phase == Phase::kTerminationSentAndClosed);
      break;
    case Phase::kTerminationSentAndClosed:
      NOTREACHED();
      break;
  }
  phase_ = phase;
}

SharedWorkerHost::ClientInfo::ClientInfo(mojom::SharedWorkerClientPtr client,
                                         int connection_request_id,
                                         int process_id,
                                         int frame_id)
    : client(std::move(client)),
      connection_request_id(connection_request_id),
      process_id(process_id),
      frame_id(frame_id) {}

SharedWorkerHost::ClientInfo::~ClientInfo() {}

void SharedWorkerHost::OnConnected(int connection_request_id) {
  if (!instance_)
    return;
  for (const ClientInfo& info : clients_) {
    if (info.connection_request_id != connection_request_id)
      continue;
    info.client->OnConnected(std::vector<blink::mojom::WebFeature>(
        used_features_.begin(), used_features_.end()));
    return;
  }
}

void SharedWorkerHost::OnContextClosed() {
  devtools_handle_.reset();

  // Mark as closed - this will stop any further messages from being sent to the
  // worker (messages can still be sent from the worker, for exception
  // reporting, etc).
  switch (phase_) {
    case Phase::kInitial:
      // Not possible: there is no Mojo connection on which OnContextClosed can
      // be called.
      NOTREACHED();
      break;
    case Phase::kStarted:
      AdvanceTo(Phase::kClosed);
      break;
    case Phase::kTerminationSent:
      AdvanceTo(Phase::kTerminationSentAndClosed);
      break;
    case Phase::kClosed:
    case Phase::kTerminationSentAndClosed:
      // Already closed, just ignore.
      break;
  }
}

void SharedWorkerHost::OnReadyForInspection() {
  if (devtools_handle_)
    devtools_handle_->WorkerReadyForInspection();
}

void SharedWorkerHost::OnScriptLoaded() {
  UMA_HISTOGRAM_TIMES("SharedWorker.TimeToScriptLoaded",
                      base::TimeTicks::Now() - creation_time_);
}

void SharedWorkerHost::OnScriptLoadFailed() {
  UMA_HISTOGRAM_TIMES("SharedWorker.TimeToScriptLoadFailed",
                      base::TimeTicks::Now() - creation_time_);
  for (const ClientInfo& info : clients_)
    info.client->OnScriptLoadFailed();
}

void SharedWorkerHost::OnFeatureUsed(blink::mojom::WebFeature feature) {
  // Avoid reporting a feature more than once, and enable any new clients to
  // observe features that were historically used.
  if (!used_features_.insert(feature).second)
    return;
  for (const ClientInfo& info : clients_)
    info.client->OnFeatureUsed(feature);
}

std::vector<std::pair<int, int>>
SharedWorkerHost::GetRenderFrameIDsForWorker() {
  std::vector<std::pair<int, int>> result;
  for (const ClientInfo& info : clients_)
    result.push_back(std::make_pair(info.process_id, info.frame_id));
  return result;
}

bool SharedWorkerHost::IsAvailable() const {
  switch (phase_) {
    case Phase::kInitial:
    case Phase::kStarted:
      return true;
    case Phase::kClosed:
    case Phase::kTerminationSent:
    case Phase::kTerminationSentAndClosed:
      return false;
  }
  NOTREACHED();
  return false;
}

base::WeakPtr<SharedWorkerHost> SharedWorkerHost::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SharedWorkerHost::AddClient(mojom::SharedWorkerClientPtr client,
                                 int process_id,
                                 int frame_id,
                                 const blink::MessagePortChannel& port) {
  // Pass the actual creation context type, so the client can understand if
  // there is a mismatch between security levels.
  client->OnCreated(instance_->creation_context_type());

  clients_.emplace_back(std::move(client), next_connection_request_id_++,
                        process_id, frame_id);
  ClientInfo& info = clients_.back();

  // Observe when the client goes away.
  info.client.set_connection_error_handler(base::BindOnce(
      &SharedWorkerHost::OnClientConnectionLost, weak_factory_.GetWeakPtr()));

  worker_->Connect(info.connection_request_id, port.ReleaseHandle());
}

void SharedWorkerHost::BindDevToolsAgent(
    blink::mojom::DevToolsAgentAssociatedRequest request) {
  worker_->BindDevToolsAgent(std::move(request));
}

void SharedWorkerHost::OnClientConnectionLost() {
  // We'll get a notification for each dropped connection.
  for (auto it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->client.encountered_error()) {
      clients_.erase(it);
      break;
    }
  }
  // If there are no clients left, then it's cleanup time.
  if (clients_.empty())
    TerminateWorker();
}

void SharedWorkerHost::OnWorkerConnectionLost() {
  // This will destroy |this| resulting in client's observing their mojo
  // connection being dropped.
  service_->DestroyHost(this);
}

void SharedWorkerHost::GetInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* process = RenderProcessHost::FromID(process_id_);
  if (!process)
    return;

  BindWorkerInterface(interface_name, std::move(interface_pipe), process,
                      url::Origin::Create(instance()->url()));
}

}  // namespace content
