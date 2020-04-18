// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_service_impl.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>

#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/browser/shared_worker/shared_worker_instance.h"
#include "content/browser/shared_worker/shared_worker_script_loader_factory.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/service_worker/service_worker_provider.mojom.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/common/shared_worker/shared_worker_client.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/bind_interface_helpers.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"
#include "url/origin.h"

namespace content {
namespace {

bool IsShuttingDown(RenderProcessHost* host) {
  return !host || host->FastShutdownStarted() ||
         host->IsKeepAliveRefCountDisabled();
}

void CreateScriptLoaderOnIO(
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter,
    std::unique_ptr<URLLoaderFactoryBundleInfo> factory_bundle_info,
    scoped_refptr<ServiceWorkerContextWrapper> context,
    int process_id,
    base::OnceCallback<void(mojom::ServiceWorkerProviderInfoForSharedWorkerPtr,
                            network::mojom::URLLoaderFactoryAssociatedPtrInfo)>
        callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Set up for service worker.
  auto provider_info = mojom::ServiceWorkerProviderInfoForSharedWorker::New();
  base::WeakPtr<ServiceWorkerProviderHost> host =
      context->PreCreateHostForSharedWorker(process_id, &provider_info);

  // Create the factory bundle for loading the script.
  scoped_refptr<URLLoaderFactoryBundle> factory_bundle =
      base::MakeRefCounted<URLLoaderFactoryBundle>(
          std::move(factory_bundle_info));

  // Add the network factory to the bundle. The factory from
  // CloneNetworkFactory() doesn't support reconnection to the network service
  // after a crash, but it's OK since it's used for a single shared worker
  // startup.
  network::mojom::URLLoaderFactoryPtr network_factory_ptr;
  loader_factory_getter->CloneNetworkFactory(
      mojo::MakeRequest(&network_factory_ptr));
  factory_bundle->SetDefaultFactory(std::move(network_factory_ptr));

  // Create the SharedWorkerScriptLoaderFactory.
  network::mojom::URLLoaderFactoryAssociatedPtrInfo script_loader_factory;
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<SharedWorkerScriptLoaderFactory>(
          context.get(), host->AsWeakPtr(), context->resource_context(),
          std::move(factory_bundle)),
      mojo::MakeRequest(&script_loader_factory));

  // TODO(falken): Also send the factory bundle to the renderer like
  // CommitNavigation does, to be used for subresource requests from the shared
  // worker (SharedWorkerScriptLoaderFactory is only used for the main resource
  // request). However, the restartable network factory should be used in this
  // case.

  // We continue in StartWorker.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), std::move(provider_info),
                     std::move(script_loader_factory)));
}

}  // namespace

SharedWorkerServiceImpl::SharedWorkerServiceImpl(
    scoped_refptr<ServiceWorkerContextWrapper> service_worker_context)
    : service_worker_context_(std::move(service_worker_context)),
      weak_factory_(this) {}

SharedWorkerServiceImpl::~SharedWorkerServiceImpl() {}

bool SharedWorkerServiceImpl::TerminateWorker(
    const GURL& url,
    const std::string& name,
    const url::Origin& constructor_origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (auto& host : worker_hosts_) {
    if (host->IsAvailable() &&
        host->instance()->Matches(url, name, constructor_origin)) {
      host->TerminateWorker();
      return true;
    }
  }
  return false;
}

void SharedWorkerServiceImpl::TerminateAllWorkersForTesting(
    base::OnceClosure callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!terminate_all_workers_callback_);
  if (worker_hosts_.empty()) {
    // Run callback asynchronously to avoid re-entering the caller.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(callback));
  } else {
    terminate_all_workers_callback_ = std::move(callback);
    for (auto& host : worker_hosts_)
      host->TerminateWorker();
    // Monitor for actual termination in DestroyHost.
  }
}

void SharedWorkerServiceImpl::ConnectToWorker(
    int process_id,
    int frame_id,
    mojom::SharedWorkerInfoPtr info,
    mojom::SharedWorkerClientPtr client,
    blink::mojom::SharedWorkerCreationContextType creation_context_type,
    const blink::MessagePortChannel& message_port) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  RenderFrameHostImpl* render_frame_host =
      RenderFrameHostImpl::FromID(process_id, frame_id);
  if (!render_frame_host) {
    // TODO(nhiroki): Support the case where the requester is a worker (i.e.,
    // nested worker) (https://crbug.com/31666).
    client->OnScriptLoadFailed();
    return;
  }

  RenderFrameHost* main_frame =
      render_frame_host->frame_tree_node()->frame_tree()->GetMainFrame();
  if (!GetContentClient()->browser()->AllowSharedWorker(
          info->url, main_frame->GetLastCommittedURL(), info->name,
          render_frame_host->GetLastCommittedOrigin(),
          WebContentsImpl::FromRenderFrameHostID(process_id, frame_id)
              ->GetBrowserContext(),
          process_id, frame_id)) {
    client->OnScriptLoadFailed();
    return;
  }

  auto instance = std::make_unique<SharedWorkerInstance>(
      info->url, info->name, render_frame_host->GetLastCommittedOrigin(),
      info->content_security_policy, info->content_security_policy_type,
      info->creation_address_space, creation_context_type);

  SharedWorkerHost* host = FindAvailableSharedWorkerHost(*instance);
  if (host) {
    // Non-secure contexts cannot connect to secure workers, and secure contexts
    // cannot connect to non-secure workers:
    if (host->instance()->creation_context_type() != creation_context_type) {
      client->OnScriptLoadFailed();
      return;
    }

    // The process may be shutting down, in which case we will try to create a
    // new shared worker instead.
    if (!IsShuttingDown(RenderProcessHost::FromID(host->process_id()))) {
      host->AddClient(std::move(client), process_id, frame_id, message_port);
      return;
    }
    // Cleanup the existing shared worker now, to avoid having two matching
    // instances. This host would likely be observing the destruction of the
    // child process shortly, but we can clean this up now to avoid some
    // complexity.
    DestroyHost(host);
  }

  CreateWorker(std::move(instance), std::move(client), process_id, frame_id,
               message_port);
}

void SharedWorkerServiceImpl::DestroyHost(SharedWorkerHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* process_host =
      RenderProcessHost::FromID(host->process_id());
  worker_hosts_.erase(worker_hosts_.find(host));

  // Complete the call to TerminateAllWorkersForTesting if no more workers.
  if (worker_hosts_.empty() && terminate_all_workers_callback_)
    std::move(terminate_all_workers_callback_).Run();

  if (!IsShuttingDown(process_host))
    process_host->DecrementKeepAliveRefCount(
        RenderProcessHost::KeepAliveClientType::kSharedWorker);
}

void SharedWorkerServiceImpl::CreateWorker(
    std::unique_ptr<SharedWorkerInstance> instance,
    mojom::SharedWorkerClientPtr client,
    int process_id,
    int frame_id,
    const blink::MessagePortChannel& message_port) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Create the host. We need to do this even before starting the worker,
  // because we are about to bounce to the IO thread. If another ConnectToWorker
  // request arrives in the meantime, it finds and reuses the host instead of
  // creating a new host and therefore new SharedWorker thread.
  auto host =
      std::make_unique<SharedWorkerHost>(this, std::move(instance), process_id);
  auto weak_host = host->AsWeakPtr();
  worker_hosts_.insert(std::move(host));

  // Bounce to the IO thread to setup service worker support in case the request
  // for the worker script will need to be intercepted by service workers.
  if (ServiceWorkerUtils::IsServicificationEnabled()) {
    // Set up the factory bundle for non-NetworkService URLs, e.g.,
    // chrome-extension:// URLs.
    ContentBrowserClient::NonNetworkURLLoaderFactoryMap factories;
    GetContentClient()
        ->browser()
        ->RegisterNonNetworkSubresourceURLLoaderFactories(
            process_id, MSG_ROUTING_NONE, &factories);

    // TODO(falken): Add FileURLLoaderFactory if the requesting frame is a file
    // resource.

    auto factory_bundle = std::make_unique<URLLoaderFactoryBundleInfo>();
    for (auto& pair : factories) {
      const std::string& scheme = pair.first;
      std::unique_ptr<network::mojom::URLLoaderFactory> factory =
          std::move(pair.second);

      network::mojom::URLLoaderFactoryPtr factory_ptr;
      mojo::MakeStrongBinding(std::move(factory),
                              mojo::MakeRequest(&factory_ptr));
      factory_bundle->factories_info().emplace(scheme,
                                               factory_ptr.PassInterface());
    }

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &CreateScriptLoaderOnIO,
            service_worker_context_->storage_partition()
                ->url_loader_factory_getter(),
            std::move(factory_bundle), service_worker_context_, process_id,
            base::BindOnce(&SharedWorkerServiceImpl::StartWorker,
                           weak_factory_.GetWeakPtr(), std::move(instance),
                           weak_host, std::move(client), process_id, frame_id,
                           message_port)));
    return;
  }

  StartWorker(std::move(instance), weak_host, std::move(client), process_id,
              frame_id, message_port, nullptr, {});
}

void SharedWorkerServiceImpl::StartWorker(
    std::unique_ptr<SharedWorkerInstance> instance,
    base::WeakPtr<SharedWorkerHost> host,
    mojom::SharedWorkerClientPtr client,
    int process_id,
    int frame_id,
    const blink::MessagePortChannel& message_port,
    mojom::ServiceWorkerProviderInfoForSharedWorkerPtr
        service_worker_provider_info,
    network::mojom::URLLoaderFactoryAssociatedPtrInfo
        script_loader_factory_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // The host may already be gone if something forcibly terminated the worker
  // before it could start (e.g., in tests or a UI action). Just fail.
  if (!host)
    return;

  RenderProcessHost* process_host = RenderProcessHost::FromID(process_id);
  // If the target process is shutting down, then just drop this request and
  // tell the host to destruct. This also means clients that were still waiting
  // for the shared worker to start will fail.
  if (!process_host || IsShuttingDown(process_host)) {
    host->TerminateWorker();
    return;
  }

  // Keep the renderer process alive that will be hosting the shared worker.
  process_host->IncrementKeepAliveRefCount(
      RenderProcessHost::KeepAliveClientType::kSharedWorker);

  // Get the factory used to instantiate the new shared worker instance in
  // the target process.
  mojom::SharedWorkerFactoryPtr factory;
  BindInterface(process_host, &factory);

  host->Start(std::move(factory), std::move(service_worker_provider_info),
              std::move(script_loader_factory_info));
  host->AddClient(std::move(client), process_id, frame_id, message_port);
}

SharedWorkerHost* SharedWorkerServiceImpl::FindAvailableSharedWorkerHost(
    const SharedWorkerInstance& instance) {
  for (auto& host : worker_hosts_) {
    if (host->IsAvailable() && host->instance()->Matches(instance))
      return host.get();
  }
  return nullptr;
}

}  // namespace content
