// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/embedded_worker_instance.h"

#include <utility>

#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/bad_message.h"
#include "content/browser/devtools/service_worker_devtools_manager.h"
#include "content/browser/service_worker/embedded_worker_registry.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/service_worker_content_settings_proxy_impl.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/content_switches_internal.h"
#include "content/common/renderer.mojom.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/common/url_loader_factory_bundle.mojom.h"
#include "content/common/url_schemes.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "url/gurl.h"

namespace content {

namespace {

// When a service worker version's failure count exceeds
// |kMaxSameProcessFailureCount|, the embedded worker is forced to start in a
// new process.
const int kMaxSameProcessFailureCount = 2;

const char kServiceWorkerTerminationCanceledMesage[] =
    "Service Worker termination by a timeout timer was canceled because "
    "DevTools is attached.";

void NotifyWorkerReadyForInspectionOnUI(
    int worker_process_id,
    int worker_route_id,
    blink::mojom::DevToolsAgentAssociatedPtrInfo devtools_agent_ptr_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ServiceWorkerDevToolsManager::GetInstance()->WorkerReadyForInspection(
      worker_process_id, worker_route_id, std::move(devtools_agent_ptr_info));
}

void NotifyWorkerDestroyedOnUI(int worker_process_id, int worker_route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ServiceWorkerDevToolsManager::GetInstance()->WorkerDestroyed(
      worker_process_id, worker_route_id);
}

void NotifyWorkerVersionInstalledOnUI(int worker_process_id,
                                      int worker_route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ServiceWorkerDevToolsManager::GetInstance()->WorkerVersionInstalled(
      worker_process_id, worker_route_id);
}

void NotifyWorkerVersionDoomedOnUI(int worker_process_id, int worker_route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ServiceWorkerDevToolsManager::GetInstance()->WorkerVersionDoomed(
      worker_process_id, worker_route_id);
}

using SetupProcessCallback = base::OnceCallback<void(
    ServiceWorkerStatusCode,
    mojom::EmbeddedWorkerStartParamsPtr,
    std::unique_ptr<ServiceWorkerProcessManager::AllocatedProcessInfo>,
    std::unique_ptr<EmbeddedWorkerInstance::DevToolsProxy>,
    std::unique_ptr<URLLoaderFactoryBundleInfo>)>;

// Allocates a renderer process for starting a worker and does setup like
// registering with DevTools. Called on the UI thread. Calls |callback| on the
// IO thread. |context| and |weak_context| are only for passing to DevTools and
// must not be dereferenced here on the UI thread.
void SetupOnUIThread(base::WeakPtr<ServiceWorkerProcessManager> process_manager,
                     bool can_use_existing_process,
                     mojom::EmbeddedWorkerStartParamsPtr params,
                     mojom::EmbeddedWorkerInstanceClientRequest request,
                     ServiceWorkerContextCore* context,
                     base::WeakPtr<ServiceWorkerContextCore> weak_context,
                     SetupProcessCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto process_info =
      std::make_unique<ServiceWorkerProcessManager::AllocatedProcessInfo>();
  std::unique_ptr<EmbeddedWorkerInstance::DevToolsProxy> devtools_proxy;
  std::unique_ptr<URLLoaderFactoryBundleInfo> factory_bundle;

  if (!process_manager) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT,
                       std::move(params), std::move(process_info),
                       std::move(devtools_proxy), std::move(factory_bundle)));
    return;
  }

  // Get a process.
  ServiceWorkerStatusCode status = process_manager->AllocateWorkerProcess(
      params->embedded_worker_id, params->scope, params->script_url,
      can_use_existing_process, process_info.get());
  if (status != SERVICE_WORKER_OK) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(std::move(callback), status, std::move(params),
                       std::move(process_info), std::move(devtools_proxy),
                       std::move(factory_bundle)));
    return;
  }
  const int process_id = process_info->process_id;
  RenderProcessHost* rph = RenderProcessHost::FromID(process_id);
  // TODO(falken): This CHECK should no longer fail, so turn to a DCHECK it if
  // crash reports agree. Consider also checking for rph->HasConnection().
  CHECK(rph);

  // Bind |request|, which is attached to |EmbeddedWorkerInstance::client_|, to
  // the process. If the process dies, |client_|'s connection error callback
  // will be called on the IO thread.
  if (request.is_pending()) {
    rph->GetRendererInterface()->SetUpEmbeddedWorkerChannelForServiceWorker(
        std::move(request));
  }

  // S13nServiceWorker:
  // Create the loader factories for non-http(s) URLs, for example
  // chrome-extension:// URLs. For performance, only do this step when the main
  // script URL is non-http(s). We assume an http(s) service worker cannot
  // importScripts a non-http(s) URL.
  if (ServiceWorkerUtils::IsServicificationEnabled() &&
      !params->script_url.SchemeIsHTTPOrHTTPS()) {
    ContentBrowserClient::NonNetworkURLLoaderFactoryMap factories;
    GetContentClient()
        ->browser()
        ->RegisterNonNetworkSubresourceURLLoaderFactories(
            rph->GetID(), MSG_ROUTING_NONE, &factories);

    factory_bundle = std::make_unique<URLLoaderFactoryBundleInfo>();
    for (auto& pair : factories) {
      const std::string& scheme = pair.first;
      std::unique_ptr<network::mojom::URLLoaderFactory> factory =
          std::move(pair.second);

      // To be safe, ignore schemes that aren't allowed to register service
      // workers. We assume that importScripts should fail on such schemes.
      if (!base::ContainsValue(GetServiceWorkerSchemes(), scheme))
        continue;
      network::mojom::URLLoaderFactoryPtr factory_ptr;
      mojo::MakeStrongBinding(std::move(factory),
                              mojo::MakeRequest(&factory_ptr));
      factory_bundle->factories_info().emplace(scheme,
                                               factory_ptr.PassInterface());
    }
  }

  // Register to DevTools and update params accordingly.
  // TODO(dgozman): we can now remove this routing id and use something else
  // as id when talking to ServiceWorkerDevToolsManager.
  const int routing_id = rph->GetNextRoutingID();
  ServiceWorkerDevToolsManager::GetInstance()->WorkerCreated(
      process_id, routing_id, context, weak_context,
      params->service_worker_version_id, params->script_url, params->scope,
      params->is_installed, &params->devtools_worker_token,
      &params->wait_for_debugger);
  params->worker_devtools_agent_route_id = routing_id;
  // Create DevToolsProxy here to ensure that the WorkerCreated() call is
  // balanced by DevToolsProxy's destructor calling WorkerDestroyed().
  devtools_proxy = std::make_unique<EmbeddedWorkerInstance::DevToolsProxy>(
      process_id, routing_id);

  // Set EmbeddedWorkerSettings for content settings only readable from the UI
  // thread.
  // TODO(bengr): Support changes to the data saver setting while the worker is
  // running.
  params->data_saver_enabled =
      GetContentClient()->browser()->IsDataSaverEnabled(
          process_manager->browser_context());

  // Continue to OnSetupCompleted on the IO thread.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(std::move(callback), status, std::move(params),
                     std::move(process_info), std::move(devtools_proxy),
                     std::move(factory_bundle)));
}

bool HasSentStartWorker(EmbeddedWorkerInstance::StartingPhase phase) {
  switch (phase) {
    case EmbeddedWorkerInstance::NOT_STARTING:
    case EmbeddedWorkerInstance::ALLOCATING_PROCESS:
      return false;
    case EmbeddedWorkerInstance::SENT_START_WORKER:
    case EmbeddedWorkerInstance::SCRIPT_DOWNLOADING:
    case EmbeddedWorkerInstance::SCRIPT_READ_STARTED:
    case EmbeddedWorkerInstance::SCRIPT_READ_FINISHED:
    case EmbeddedWorkerInstance::SCRIPT_STREAMING:
    case EmbeddedWorkerInstance::SCRIPT_LOADED:
    case EmbeddedWorkerInstance::SCRIPT_EVALUATED:
    case EmbeddedWorkerInstance::THREAD_STARTED:
      return true;
    case EmbeddedWorkerInstance::STARTING_PHASE_MAX_VALUE:
      NOTREACHED();
  }
  return false;
}

}  // namespace

// Created on UI thread and moved to IO thread. Proxies notifications to
// DevToolsManager that lives on UI thread. Owned by EmbeddedWorkerInstance.
class EmbeddedWorkerInstance::DevToolsProxy {
 public:
  DevToolsProxy(int process_id, int agent_route_id)
      : process_id_(process_id),
        agent_route_id_(agent_route_id) {}

  ~DevToolsProxy() {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(NotifyWorkerDestroyedOnUI,
                                           process_id_, agent_route_id_));
  }

  void NotifyWorkerReadyForInspection(
      blink::mojom::DevToolsAgentAssociatedPtrInfo devtools_agent_ptr_info) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(NotifyWorkerReadyForInspectionOnUI, process_id_,
                       agent_route_id_, std::move(devtools_agent_ptr_info)));
  }

  void NotifyWorkerVersionInstalled() {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(NotifyWorkerVersionInstalledOnUI,
                                           process_id_, agent_route_id_));
  }

  void NotifyWorkerVersionDoomed() {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(NotifyWorkerVersionDoomedOnUI,
                                           process_id_, agent_route_id_));
  }

  bool ShouldNotifyWorkerStopIgnored() const {
    return !worker_stop_ignored_notified_;
  }

  void WorkerStopIgnoredNotified() { worker_stop_ignored_notified_ = true; }

  int agent_route_id() const { return agent_route_id_; }

 private:
  const int process_id_;
  const int agent_route_id_;
  bool worker_stop_ignored_notified_ = false;

  DISALLOW_COPY_AND_ASSIGN(DevToolsProxy);
};

// A handle for a renderer process managed by ServiceWorkerProcessManager on the
// UI thread. Lives on the IO thread.
class EmbeddedWorkerInstance::WorkerProcessHandle {
 public:
  WorkerProcessHandle(
      const base::WeakPtr<ServiceWorkerProcessManager>& process_manager,
      int embedded_worker_id,
      int process_id)
      : process_manager_(process_manager),
        embedded_worker_id_(embedded_worker_id),
        process_id_(process_id) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DCHECK_NE(ChildProcessHost::kInvalidUniqueID, process_id_);
  }

  ~WorkerProcessHandle() {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&ServiceWorkerProcessManager::ReleaseWorkerProcess,
                       process_manager_, embedded_worker_id_));
  }

  int process_id() const { return process_id_; }

 private:
  // Can be dereferenced on the UI thread only.
  base::WeakPtr<ServiceWorkerProcessManager> process_manager_;

  const int embedded_worker_id_;
  const int process_id_;

  DISALLOW_COPY_AND_ASSIGN(WorkerProcessHandle);
};

// A task to allocate a worker process and to send a start worker message. This
// is created on EmbeddedWorkerInstance::Start(), owned by the instance and
// destroyed on EmbeddedWorkerInstance::OnScriptEvaluated().
// We can abort starting worker by destroying this task anytime during the
// sequence.
// Lives on the IO thread.
class EmbeddedWorkerInstance::StartTask {
 public:
  enum class ProcessAllocationState { NOT_ALLOCATED, ALLOCATING, ALLOCATED };

  StartTask(EmbeddedWorkerInstance* instance,
            const GURL& script_url,
            mojom::EmbeddedWorkerInstanceClientRequest request)
      : instance_(instance),
        request_(std::move(request)),
        state_(ProcessAllocationState::NOT_ALLOCATED),
        is_installed_(false),
        started_during_browser_startup_(false),
        weak_factory_(this) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("ServiceWorker",
                                      "EmbeddedWorkerInstance::Start", this,
                                      "Script", script_url.spec());
  }

  ~StartTask() {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    TRACE_EVENT_NESTABLE_ASYNC_END0("ServiceWorker",
                                    "EmbeddedWorkerInstance::Start", this);

    if (!instance_->context_)
      return;

    switch (state_) {
      case ProcessAllocationState::NOT_ALLOCATED:
        // Not necessary to release a process.
        break;
      case ProcessAllocationState::ALLOCATING:
        // Abort half-baked process allocation on the UI thread.
        BrowserThread::PostTask(
            BrowserThread::UI, FROM_HERE,
            base::BindOnce(&ServiceWorkerProcessManager::ReleaseWorkerProcess,
                           instance_->context_->process_manager()->AsWeakPtr(),
                           instance_->embedded_worker_id()));
        break;
      case ProcessAllocationState::ALLOCATED:
        // Otherwise, the process will be released by EmbeddedWorkerInstance.
        break;
    }

    // Don't have to abort |start_callback_| here. The caller of
    // EmbeddedWorkerInstance::Start(), that is, ServiceWorkerVersion does not
    // expect it when the start worker sequence is canceled by Stop() because
    // the callback, ServiceWorkerVersion::OnStartSentAndScriptEvaluated(),
    // could drain valid start requests queued in the version. After the worker
    // is stopped, the version attempts to restart the worker if there are
    // requests in the queue. See ServiceWorkerVersion::OnStoppedInternal() for
    // details.
    // TODO(nhiroki): Reconsider this bizarre layering.
  }

  void set_start_worker_sent_time(base::TimeTicks time) {
    start_worker_sent_time_ = time;
  }
  base::TimeTicks start_worker_sent_time() const {
    return start_worker_sent_time_;
  }

  void Start(mojom::EmbeddedWorkerStartParamsPtr params,
             StatusCallback callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DCHECK(instance_->context_);
    state_ = ProcessAllocationState::ALLOCATING;
    start_callback_ = std::move(callback);
    is_installed_ = params->is_installed;

    if (!GetContentClient()->browser()->IsBrowserStartupComplete())
      started_during_browser_startup_ = true;

    bool can_use_existing_process =
        instance_->context_->GetVersionFailureCount(
            params->service_worker_version_id) < kMaxSameProcessFailureCount;
    DCHECK_EQ(params->embedded_worker_id, instance_->embedded_worker_id_);
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("ServiceWorker", "ALLOCATING_PROCESS",
                                      this);
    base::WeakPtr<ServiceWorkerContextCore> context = instance_->context_;
    base::WeakPtr<ServiceWorkerProcessManager> process_manager =
        context->process_manager()->AsWeakPtr();

    // Hop to the UI thread for process allocation and setup. We will continue
    // on the IO thread in StartTask::OnSetupCompleted().
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &SetupOnUIThread, process_manager, can_use_existing_process,
            std::move(params), std::move(request_), context.get(), context,
            base::BindOnce(&StartTask::OnSetupCompleted,
                           weak_factory_.GetWeakPtr(), process_manager)));
  }

  static void RunStartCallback(StartTask* task,
                               ServiceWorkerStatusCode status) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    TRACE_EVENT_NESTABLE_ASYNC_END1("ServiceWorker", "INITIALIZING_ON_RENDERER",
                                    task, "Status",
                                    ServiceWorkerStatusToString(status));
    StatusCallback callback = std::move(task->start_callback_);
    task->start_callback_.Reset();
    std::move(callback).Run(status);
    // |task| may be destroyed.
  }

  bool is_installed() const { return is_installed_; }

 private:
  void OnSetupCompleted(
      base::WeakPtr<ServiceWorkerProcessManager> process_manager,
      ServiceWorkerStatusCode status,
      mojom::EmbeddedWorkerStartParamsPtr params,
      std::unique_ptr<ServiceWorkerProcessManager::AllocatedProcessInfo>
          process_info,
      std::unique_ptr<EmbeddedWorkerInstance::DevToolsProxy> devtools_proxy,
      std::unique_ptr<URLLoaderFactoryBundleInfo> factory_bundle) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    std::unique_ptr<WorkerProcessHandle> process_handle;
    if (status == SERVICE_WORKER_OK) {
      // If we allocated a process, WorkerProcessHandle has to be created before
      // returning to ensure the process is eventually released.
      process_handle = std::make_unique<WorkerProcessHandle>(
          process_manager, instance_->embedded_worker_id(),
          process_info->process_id);

      if (!instance_->context_)
        status = SERVICE_WORKER_ERROR_ABORT;
    }

    if (status != SERVICE_WORKER_OK) {
      TRACE_EVENT_NESTABLE_ASYNC_END1("ServiceWorker", "ALLOCATING_PROCESS",
                                      this, "Error",
                                      ServiceWorkerStatusToString(status));
      StatusCallback callback = std::move(start_callback_);
      start_callback_.Reset();
      instance_->OnStartFailed(std::move(callback), status);
      // |this| may be destroyed.
      return;
    }

    ServiceWorkerMetrics::StartSituation start_situation =
        process_info->start_situation;
    TRACE_EVENT_NESTABLE_ASYNC_END1(
        "ServiceWorker", "ALLOCATING_PROCESS", this, "StartSituation",
        ServiceWorkerMetrics::StartSituationToString(start_situation));
    if (is_installed_) {
      ServiceWorkerMetrics::RecordProcessCreated(
          start_situation == ServiceWorkerMetrics::StartSituation::NEW_PROCESS);
    }

    if (started_during_browser_startup_)
      start_situation = ServiceWorkerMetrics::StartSituation::DURING_STARTUP;

    // Notify the instance that a process is allocated.
    state_ = ProcessAllocationState::ALLOCATED;
    instance_->OnProcessAllocated(std::move(process_handle), start_situation);

    // Notify the instance that it is registered to the DevTools manager.
    instance_->OnRegisteredToDevToolsManager(std::move(devtools_proxy),
                                             params->wait_for_debugger);

    // S13nServiceWorker: Build the URLLoaderFactory for loading new scripts.
    scoped_refptr<network::SharedURLLoaderFactory> factory_for_new_scripts;
    if (ServiceWorkerUtils::IsServicificationEnabled()) {
      if (factory_bundle) {
        network::mojom::URLLoaderFactoryPtr network_factory_ptr;
        // The factory from CloneNetworkFactory() doesn't support reconnection
        // to the network service after a crash, but it's probably OK since it's
        // used for a single service worker startup until installation finishes
        // (with the exception of https://crbug.com/719052).
        instance_->context_->loader_factory_getter()->CloneNetworkFactory(
            mojo::MakeRequest(&network_factory_ptr));
        scoped_refptr<URLLoaderFactoryBundle> factory =
            base::MakeRefCounted<URLLoaderFactoryBundle>(
                std::move(factory_bundle));
        factory->SetDefaultFactory(std::move(network_factory_ptr));
        factory_for_new_scripts = std::move(factory);
      } else {
        factory_for_new_scripts =
            instance_->context_->loader_factory_getter()->GetNetworkFactory();
      }
    }

    instance_->SendStartWorker(std::move(params),
                               std::move(factory_for_new_scripts));

    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("ServiceWorker",
                                      "INITIALIZING_ON_RENDERER", this);
    // |this|'s work is done here, but |instance_| still uses its state until
    // startup is complete.
  }

  // |instance_| must outlive |this|.
  EmbeddedWorkerInstance* instance_;

  // Ownership is transferred by a PostTask() call after process allocation.
  mojom::EmbeddedWorkerInstanceClientRequest request_;

  StatusCallback start_callback_;
  ProcessAllocationState state_;

  // Used for UMA.
  bool is_installed_;
  bool started_during_browser_startup_;
  base::TimeTicks start_worker_sent_time_;

  base::WeakPtrFactory<StartTask> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StartTask);
};

EmbeddedWorkerInstance::~EmbeddedWorkerInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(status_ == EmbeddedWorkerStatus::STOPPING ||
         status_ == EmbeddedWorkerStatus::STOPPED)
      << static_cast<int>(status_);
  devtools_proxy_.reset();
  if (registry_->GetWorker(embedded_worker_id_))
    registry_->RemoveWorker(process_id(), embedded_worker_id_);
  process_handle_.reset();
}

void EmbeddedWorkerInstance::Start(mojom::EmbeddedWorkerStartParamsPtr params,
                                   ProviderInfoGetter provider_info_getter,
                                   StatusCallback callback) {
  restart_count_++;
  if (!context_) {
    std::move(callback).Run(SERVICE_WORKER_ERROR_ABORT);
    // |this| may be destroyed by the callback.
    return;
  }
  DCHECK_EQ(EmbeddedWorkerStatus::STOPPED, status_);

  DCHECK(!params->pause_after_download || !params->is_installed);
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerVersionId,
            params->service_worker_version_id);

  step_time_ = base::TimeTicks::Now();
  status_ = EmbeddedWorkerStatus::STARTING;
  starting_phase_ = ALLOCATING_PROCESS;
  network_accessed_for_script_ = false;
  provider_info_getter_ = std::move(provider_info_getter);

  for (auto& observer : listener_list_)
    observer.OnStarting();

  params->embedded_worker_id = embedded_worker_id_;
  params->worker_devtools_agent_route_id = MSG_ROUTING_NONE;
  params->wait_for_debugger = false;
  params->v8_cache_options = GetV8CacheOptions();

  mojom::EmbeddedWorkerInstanceClientRequest request =
      mojo::MakeRequest(&client_);
  client_.set_connection_error_handler(
      base::BindOnce(&EmbeddedWorkerInstance::Detach, base::Unretained(this)));
  inflight_start_task_.reset(
      new StartTask(this, params->script_url, std::move(request)));
  inflight_start_task_->Start(std::move(params), std::move(callback));
}

void EmbeddedWorkerInstance::Stop() {
  DCHECK(status_ == EmbeddedWorkerStatus::STARTING ||
         status_ == EmbeddedWorkerStatus::RUNNING)
      << static_cast<int>(status_);

  // Abort an inflight start task.
  inflight_start_task_.reset();

  // Don't send the StopWorker message if the StartWorker message hasn't
  // been sent.
  if (status_ == EmbeddedWorkerStatus::STARTING &&
      !HasSentStartWorker(starting_phase())) {
    ReleaseProcess();
    for (auto& observer : listener_list_)
      observer.OnStopped(EmbeddedWorkerStatus::STARTING /* old_status */);
    return;
  }

  client_->StopWorker();
  status_ = EmbeddedWorkerStatus::STOPPING;
  for (auto& observer : listener_list_)
    observer.OnStopping();
}

void EmbeddedWorkerInstance::StopIfNotAttachedToDevTools() {
  if (devtools_attached_) {
    if (devtools_proxy_) {
      // Check ShouldNotifyWorkerStopIgnored not to show the same message
      // multiple times in DevTools.
      if (devtools_proxy_->ShouldNotifyWorkerStopIgnored()) {
        AddMessageToConsole(blink::WebConsoleMessage::kLevelVerbose,
                            kServiceWorkerTerminationCanceledMesage);
        devtools_proxy_->WorkerStopIgnoredNotified();
      }
    }
    return;
  }
  Stop();
}

void EmbeddedWorkerInstance::ResumeAfterDownload() {
  if (process_id() == ChildProcessHost::kInvalidUniqueID ||
      status_ != EmbeddedWorkerStatus::STARTING) {
    return;
  }
  DCHECK(client_.is_bound());
  client_->ResumeAfterDownload();
}

EmbeddedWorkerInstance::EmbeddedWorkerInstance(
    base::WeakPtr<ServiceWorkerContextCore> context,
    ServiceWorkerVersion* owner_version,
    int embedded_worker_id)
    : context_(context),
      registry_(context->embedded_worker_registry()),
      owner_version_(owner_version),
      embedded_worker_id_(embedded_worker_id),
      status_(EmbeddedWorkerStatus::STOPPED),
      starting_phase_(NOT_STARTING),
      restart_count_(0),
      thread_id_(kInvalidEmbeddedWorkerThreadId),
      instance_host_binding_(this),
      devtools_attached_(false),
      network_accessed_for_script_(false),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void EmbeddedWorkerInstance::OnProcessAllocated(
    std::unique_ptr<WorkerProcessHandle> handle,
    ServiceWorkerMetrics::StartSituation start_situation) {
  DCHECK_EQ(EmbeddedWorkerStatus::STARTING, status_);
  DCHECK(!process_handle_);

  process_handle_ = std::move(handle);
  start_situation_ = start_situation;
  for (auto& observer : listener_list_)
    observer.OnProcessAllocated();
}

void EmbeddedWorkerInstance::OnRegisteredToDevToolsManager(
    std::unique_ptr<DevToolsProxy> devtools_proxy,
    bool wait_for_debugger) {
  if (devtools_proxy) {
    DCHECK(!devtools_proxy_);
    devtools_proxy_ = std::move(devtools_proxy);
  }
  if (wait_for_debugger) {
    // We don't measure the start time when wait_for_debugger flag is set. So
    // we set the NULL time here.
    step_time_ = base::TimeTicks();
  }
  for (auto& observer : listener_list_)
    observer.OnRegisteredToDevToolsManager();
}

void EmbeddedWorkerInstance::SendStartWorker(
    mojom::EmbeddedWorkerStartParamsPtr params,
    scoped_refptr<network::SharedURLLoaderFactory> factory) {
  DCHECK(context_);
  DCHECK(params->dispatcher_request.is_pending());
  DCHECK(params->controller_request.is_pending());
  DCHECK(!instance_host_binding_.is_bound());
  instance_host_binding_.Bind(mojo::MakeRequest(&params->instance_host));

  blink::mojom::WorkerContentSettingsProxyPtr content_settings_proxy_ptr_info;
  content_settings_ = std::make_unique<ServiceWorkerContentSettingsProxyImpl>(
      params->script_url, context_,
      mojo::MakeRequest(&params->content_settings_proxy));

  const bool is_script_streaming = !params->installed_scripts_info.is_null();
  inflight_start_task_->set_start_worker_sent_time(base::TimeTicks::Now());
  params->provider_info =
      std::move(provider_info_getter_).Run(process_id(), std::move(factory));
  client_->StartWorker(std::move(params));
  registry_->BindWorkerToProcess(process_id(), embedded_worker_id());

  if (!step_time_.is_null()) {
    base::TimeDelta duration = UpdateStepTime();
    if (inflight_start_task_->is_installed()) {
      ServiceWorkerMetrics::RecordTimeToSendStartWorker(duration,
                                                        start_situation_);
    }
  }

  starting_phase_ = is_script_streaming ? SCRIPT_STREAMING : SENT_START_WORKER;
  for (auto& observer : listener_list_)
    observer.OnStartWorkerMessageSent();
}

void EmbeddedWorkerInstance::RequestTermination() {
  if (!ServiceWorkerUtils::IsServicificationEnabled()) {
    mojo::ReportBadMessage(
        "Invalid termination request: RequestTermination() was called but "
        "S13nServiceWorker is not enabled");
    return;
  }

  if (status() != EmbeddedWorkerStatus::RUNNING &&
      status() != EmbeddedWorkerStatus::STOPPING) {
    mojo::ReportBadMessage(
        "Invalid termination request: Termination should be requested during "
        "running or stopping");
    return;
  }

  if (status() == EmbeddedWorkerStatus::STOPPING)
    return;
  owner_version_->StopWorkerIfIdle(true /* requested_from_renderer */);
}

void EmbeddedWorkerInstance::CountFeature(blink::mojom::WebFeature feature) {
  owner_version_->CountFeature(feature);
}

void EmbeddedWorkerInstance::OnReadyForInspection() {
  if (devtools_proxy_) {
    blink::mojom::DevToolsAgentAssociatedPtrInfo devtools_agent_ptr_info;
    client_->BindDevToolsAgent(mojo::MakeRequest(&devtools_agent_ptr_info));
    devtools_proxy_->NotifyWorkerReadyForInspection(
        std::move(devtools_agent_ptr_info));
  }
}

void EmbeddedWorkerInstance::OnScriptReadStarted() {
  starting_phase_ = SCRIPT_READ_STARTED;
}

void EmbeddedWorkerInstance::OnScriptReadFinished() {
  starting_phase_ = SCRIPT_READ_FINISHED;
}

void EmbeddedWorkerInstance::OnScriptLoaded() {
  using LoadSource = ServiceWorkerMetrics::LoadSource;

  if (!inflight_start_task_)
    return;
  LoadSource source;
  if (network_accessed_for_script_) {
    DCHECK(!inflight_start_task_->is_installed());
    source = LoadSource::NETWORK;
  } else if (inflight_start_task_->is_installed()) {
    source = LoadSource::SERVICE_WORKER_STORAGE;
  } else {
    source = LoadSource::HTTP_CACHE;
  }

  // Don't record the time when script streaming is enabled because
  // OnScriptLoaded is called at the different timing.
  if (starting_phase_ != SCRIPT_STREAMING && !step_time_.is_null()) {
    base::TimeDelta duration = UpdateStepTime();
    ServiceWorkerMetrics::RecordTimeToLoad(duration, source, start_situation_);
  }

  // Renderer side has started to launch the worker thread.
  starting_phase_ = SCRIPT_LOADED;
  for (auto& observer : listener_list_)
    observer.OnScriptLoaded();
  // |this| may be destroyed by the callback.
}

void EmbeddedWorkerInstance::OnURLJobCreatedForMainScript() {
  if (!inflight_start_task_)
    return;

  if (!step_time_.is_null()) {
    base::TimeDelta duration = UpdateStepTime();
    if (inflight_start_task_->is_installed())
      ServiceWorkerMetrics::RecordTimeToURLJob(duration, start_situation_);
  }
}

void EmbeddedWorkerInstance::OnWorkerVersionInstalled() {
  if (devtools_proxy_)
    devtools_proxy_->NotifyWorkerVersionInstalled();
}

void EmbeddedWorkerInstance::OnWorkerVersionDoomed() {
  if (devtools_proxy_)
    devtools_proxy_->NotifyWorkerVersionDoomed();
}

void EmbeddedWorkerInstance::OnThreadStarted(int thread_id) {
  if (!context_ || !inflight_start_task_)
    return;

  starting_phase_ = THREAD_STARTED;
  if (!step_time_.is_null()) {
    base::TimeDelta duration = UpdateStepTime();
    if (inflight_start_task_->is_installed())
      ServiceWorkerMetrics::RecordTimeToStartThread(duration, start_situation_);
  }

  thread_id_ = thread_id;
  for (auto& observer : listener_list_)
    observer.OnThreadStarted();
}

void EmbeddedWorkerInstance::OnScriptLoadFailed() {
  if (!inflight_start_task_)
    return;

  // starting_phase_ may be SCRIPT_READ_FINISHED in case of reading from cache.
  for (auto& observer : listener_list_)
    observer.OnScriptLoadFailed();
}

void EmbeddedWorkerInstance::OnScriptEvaluated(bool success) {
  if (!inflight_start_task_)
    return;

  DCHECK_EQ(EmbeddedWorkerStatus::STARTING, status_);

  // Renderer side has completed evaluating the loaded worker script.
  starting_phase_ = SCRIPT_EVALUATED;
  if (!step_time_.is_null()) {
    base::TimeDelta duration = UpdateStepTime();
    if (success && inflight_start_task_->is_installed())
      ServiceWorkerMetrics::RecordTimeToEvaluateScript(duration,
                                                       start_situation_);
  }

  base::WeakPtr<EmbeddedWorkerInstance> weak_this = weak_factory_.GetWeakPtr();
  StartTask::RunStartCallback(
      inflight_start_task_.get(),
      success ? SERVICE_WORKER_OK
              : SERVICE_WORKER_ERROR_SCRIPT_EVALUATE_FAILED);
  // |this| may be destroyed by the callback.
}

void EmbeddedWorkerInstance::OnStarted(
    mojom::EmbeddedWorkerStartTimingPtr start_timing) {
  if (!registry_->OnWorkerStarted(process_id(), embedded_worker_id_))
    return;
  // Stop is requested before OnStarted is sent back from the worker.
  if (status_ == EmbeddedWorkerStatus::STOPPING)
    return;

  if (inflight_start_task_->is_installed()) {
    ServiceWorkerMetrics::RecordEmbeddedWorkerStartTiming(
        std::move(start_timing), inflight_start_task_->start_worker_sent_time(),
        start_situation_);
  }
  DCHECK_EQ(EmbeddedWorkerStatus::STARTING, status_);
  status_ = EmbeddedWorkerStatus::RUNNING;
  inflight_start_task_.reset();
  for (auto& observer : listener_list_)
    observer.OnStarted();
}

void EmbeddedWorkerInstance::OnStopped() {
  registry_->OnWorkerStopped(process_id(), embedded_worker_id_);

  EmbeddedWorkerStatus old_status = status_;
  ReleaseProcess();
  for (auto& observer : listener_list_)
    observer.OnStopped(old_status);
}

void EmbeddedWorkerInstance::Detach() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (status() == EmbeddedWorkerStatus::STOPPED)
    return;
  registry_->DetachWorker(process_id(), embedded_worker_id());

  EmbeddedWorkerStatus old_status = status_;
  ReleaseProcess();
  for (auto& observer : listener_list_)
    observer.OnDetached(old_status);
}

base::WeakPtr<EmbeddedWorkerInstance> EmbeddedWorkerInstance::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void EmbeddedWorkerInstance::OnReportException(
    const base::string16& error_message,
    int line_number,
    int column_number,
    const GURL& source_url) {
  for (auto& observer : listener_list_) {
    observer.OnReportException(error_message, line_number, column_number,
                               source_url);
  }
}

void EmbeddedWorkerInstance::OnReportConsoleMessage(
    int source_identifier,
    int message_level,
    const base::string16& message,
    int line_number,
    const GURL& source_url) {
  for (auto& observer : listener_list_) {
    observer.OnReportConsoleMessage(source_identifier, message_level, message,
                                    line_number, source_url);
  }
}

int EmbeddedWorkerInstance::process_id() const {
  if (process_handle_)
    return process_handle_->process_id();
  return ChildProcessHost::kInvalidUniqueID;
}

int EmbeddedWorkerInstance::worker_devtools_agent_route_id() const {
  if (devtools_proxy_)
    return devtools_proxy_->agent_route_id();
  return MSG_ROUTING_NONE;
}

void EmbeddedWorkerInstance::AddObserver(Listener* listener) {
  listener_list_.AddObserver(listener);
}

void EmbeddedWorkerInstance::RemoveObserver(Listener* listener) {
  listener_list_.RemoveObserver(listener);
}

void EmbeddedWorkerInstance::SetDevToolsAttached(bool attached) {
  devtools_attached_ = attached;
  if (attached)
    registry_->OnDevToolsAttached(embedded_worker_id_);
}

void EmbeddedWorkerInstance::OnNetworkAccessedForScriptLoad() {
  starting_phase_ = SCRIPT_DOWNLOADING;
  network_accessed_for_script_ = true;
}

void EmbeddedWorkerInstance::ReleaseProcess() {
  // Abort an inflight start task.
  inflight_start_task_.reset();

  instance_host_binding_.Close();
  devtools_proxy_.reset();
  process_handle_.reset();
  status_ = EmbeddedWorkerStatus::STOPPED;
  starting_phase_ = NOT_STARTING;
  thread_id_ = kInvalidEmbeddedWorkerThreadId;
}

void EmbeddedWorkerInstance::OnStartFailed(StatusCallback callback,
                                           ServiceWorkerStatusCode status) {
  EmbeddedWorkerStatus old_status = status_;
  ReleaseProcess();
  base::WeakPtr<EmbeddedWorkerInstance> weak_this = weak_factory_.GetWeakPtr();
  std::move(callback).Run(status);
  if (weak_this && old_status != EmbeddedWorkerStatus::STOPPED) {
    for (auto& observer : weak_this->listener_list_)
      observer.OnStopped(old_status);
  }
}

base::TimeDelta EmbeddedWorkerInstance::UpdateStepTime() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!step_time_.is_null());
  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeDelta duration = now - step_time_;
  step_time_ = now;
  return duration;
}

void EmbeddedWorkerInstance::AddMessageToConsole(
    blink::WebConsoleMessage::Level level,
    const std::string& message) {
  if (process_id() == ChildProcessHost::kInvalidUniqueID)
    return;
  DCHECK(client_.is_bound());
  client_->AddMessageToConsole(level, message);
}

// static
std::string EmbeddedWorkerInstance::StatusToString(
    EmbeddedWorkerStatus status) {
  switch (status) {
    case EmbeddedWorkerStatus::STOPPED:
      return "STOPPED";
    case EmbeddedWorkerStatus::STARTING:
      return "STARTING";
    case EmbeddedWorkerStatus::RUNNING:
      return "RUNNING";
    case EmbeddedWorkerStatus::STOPPING:
      return "STOPPING";
  }
  NOTREACHED() << static_cast<int>(status);
  return std::string();
}

// static
std::string EmbeddedWorkerInstance::StartingPhaseToString(StartingPhase phase) {
  switch (phase) {
    case NOT_STARTING:
      return "Not in STARTING status";
    case ALLOCATING_PROCESS:
      return "Allocating process";
    case SENT_START_WORKER:
      return "Sent StartWorker message to renderer";
    case SCRIPT_DOWNLOADING:
      return "Script downloading";
    case SCRIPT_LOADED:
      return "Script loaded";
    case SCRIPT_EVALUATED:
      return "Script evaluated";
    case THREAD_STARTED:
      return "Thread started";
    case SCRIPT_READ_STARTED:
      return "Script read started";
    case SCRIPT_READ_FINISHED:
      return "Script read finished";
    case SCRIPT_STREAMING:
      return "Script streaming";
    case STARTING_PHASE_MAX_VALUE:
      NOTREACHED();
  }
  NOTREACHED() << phase;
  return std::string();
}

}  // namespace content
