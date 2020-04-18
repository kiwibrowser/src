// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_version.h"

#include <stddef.h>

#include <limits>
#include <map>
#include <string>

#include "base/command_line.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "base/time/default_tick_clock.h"
#include "content/browser/bad_message.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/service_worker/embedded_worker_registry.h"
#include "content/browser/service_worker/payment_handler_support.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_installed_scripts_sender.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_type_converters.h"
#include "content/common/service_worker/embedded_worker.mojom.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/result_codes.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "third_party/blink/public/common/origin_trials/trial_token_validator.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_error_type.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_installed_scripts_manager.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/web/web_console_message.h"

namespace content {
namespace {

// Timeout for an installed worker to start.
constexpr base::TimeDelta kStartInstalledWorkerTimeout =
    base::TimeDelta::FromSeconds(60);

// Timeout for a request to be handled.
constexpr base::TimeDelta kRequestTimeout = base::TimeDelta::FromMinutes(5);

// Time to wait until stopping an idle worker.
constexpr base::TimeDelta kIdleWorkerTimeout = base::TimeDelta::FromSeconds(30);

// Timeout for waiting for a response to a ping.
constexpr base::TimeDelta kPingTimeout = base::TimeDelta::FromSeconds(30);

// Default delay for scheduled update.
constexpr base::TimeDelta kUpdateDelay = base::TimeDelta::FromSeconds(1);

const char kClaimClientsStateErrorMesage[] =
    "Only the active worker can claim clients.";

const char kClaimClientsShutdownErrorMesage[] =
    "Failed to claim clients due to Service Worker system shutdown.";

const char kNotRespondingErrorMesage[] = "Service Worker is not responding.";
const char kForceUpdateInfoMessage[] =
    "Service Worker was updated because \"Update on reload\" was "
    "checked in the DevTools Application panel.";

void RunSoon(base::OnceClosure callback) {
  if (!callback.is_null())
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(callback));
}

template <typename CallbackArray, typename Arg>
void RunCallbacks(ServiceWorkerVersion* version,
                  CallbackArray* callbacks_ptr,
                  const Arg& arg) {
  CallbackArray callbacks;
  callbacks.swap(*callbacks_ptr);
  for (auto& callback : callbacks)
    std::move(callback).Run(arg);
}

// An adapter to run a |callback| after StartWorker.
void RunCallbackAfterStartWorker(base::WeakPtr<ServiceWorkerVersion> version,
                                 ServiceWorkerVersion::StatusCallback callback,
                                 ServiceWorkerStatusCode status) {
  if (status == SERVICE_WORKER_OK &&
      version->running_status() != EmbeddedWorkerStatus::RUNNING) {
    // We've tried to start the worker (and it has succeeded), but
    // it looks it's not running yet.
    NOTREACHED() << "The worker's not running after successful StartWorker";
    std::move(callback).Run(SERVICE_WORKER_ERROR_START_WORKER_FAILED);
    return;
  }
  std::move(callback).Run(status);
}

void ClearTick(base::TimeTicks* time) {
  *time = base::TimeTicks();
}

std::string VersionStatusToString(ServiceWorkerVersion::Status status) {
  switch (status) {
    case ServiceWorkerVersion::NEW:
      return "new";
    case ServiceWorkerVersion::INSTALLING:
      return "installing";
    case ServiceWorkerVersion::INSTALLED:
      return "installed";
    case ServiceWorkerVersion::ACTIVATING:
      return "activating";
    case ServiceWorkerVersion::ACTIVATED:
      return "activated";
    case ServiceWorkerVersion::REDUNDANT:
      return "redundant";
  }
  NOTREACHED() << status;
  return std::string();
}

const int kInvalidTraceId = -1;

int NextTraceId() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  static int trace_id = 0;
  if (trace_id == std::numeric_limits<int>::max())
    trace_id = 0;
  else
    ++trace_id;
  DCHECK_NE(kInvalidTraceId, trace_id);
  return trace_id;
}

void OnEventDispatcherConnectionError(
    base::WeakPtr<EmbeddedWorkerInstance> embedded_worker) {
  if (!embedded_worker)
    return;

  switch (embedded_worker->status()) {
    case EmbeddedWorkerStatus::STARTING:
    case EmbeddedWorkerStatus::RUNNING:
      // In this case the disconnection might be happening because of sudden
      // renderer shutdown like crash.
      embedded_worker->Detach();
      break;
    case EmbeddedWorkerStatus::STOPPING:
    case EmbeddedWorkerStatus::STOPPED:
      // Do nothing
      break;
  }
}

// In S13nServiceWorker, |loader_factory| is the factory to use loading new
// scripts from network (or other sources, e.g., for chrome-extension:// URLs).
mojom::ServiceWorkerProviderInfoForStartWorkerPtr
CompleteProviderHostPreparation(
    ServiceWorkerVersion* version,
    std::unique_ptr<ServiceWorkerProviderHost> provider_host,
    base::WeakPtr<ServiceWorkerContextCore> context,
    int process_id,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory) {
  // Caller should ensure |context| is alive when completing StartWorker
  // preparation.
  DCHECK(context);
  auto info = provider_host->CompleteStartWorkerPreparation(
      process_id, version, std::move(loader_factory));
  context->AddProviderHost(std::move(provider_host));
  return info;
}

void OnOpenWindowFinished(
    blink::mojom::ServiceWorkerHost::OpenNewTabCallback callback,
    ServiceWorkerStatusCode status,
    blink::mojom::ServiceWorkerClientInfoPtr client_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const bool success = (status == SERVICE_WORKER_OK);
  base::Optional<std::string> error_msg;
  if (!success) {
    DCHECK(!client_info);
    error_msg.emplace("Something went wrong while trying to open the window.");
  }
  std::move(callback).Run(success, std::move(client_info), error_msg);
}

void DidShowPaymentHandlerWindow(
    const GURL& url,
    const base::WeakPtr<ServiceWorkerContextCore>& context,
    blink::mojom::ServiceWorkerHost::OpenPaymentHandlerWindowCallback callback,
    bool success,
    int render_process_id,
    int render_frame_id) {
  if (success) {
    service_worker_client_utils::DidNavigate(
        context, url.GetOrigin(),
        base::BindOnce(&OnOpenWindowFinished, std::move(callback)),
        render_process_id, render_frame_id);
  } else {
    OnOpenWindowFinished(std::move(callback), SERVICE_WORKER_ERROR_FAILED,
                         nullptr /* client_info */);
  }
}

void DidGetClients(
    blink::mojom::ServiceWorkerHost::GetClientsCallback callback,
    std::unique_ptr<service_worker_client_utils::ServiceWorkerClientPtrs>
        clients) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(std::move(*clients));
}

void DidNavigateClient(
    blink::mojom::ServiceWorkerHost::NavigateClientCallback callback,
    const GURL& url,
    ServiceWorkerStatusCode status,
    blink::mojom::ServiceWorkerClientInfoPtr client) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const bool success = (status == SERVICE_WORKER_OK);
  base::Optional<std::string> error_msg;
  if (!success) {
    DCHECK(!client);
    error_msg.emplace("Cannot navigate to URL: " + url.spec());
  }
  std::move(callback).Run(success, std::move(client), error_msg);
}

}  // namespace

constexpr base::TimeDelta ServiceWorkerVersion::kTimeoutTimerDelay;
constexpr base::TimeDelta ServiceWorkerVersion::kStartNewWorkerTimeout;
constexpr base::TimeDelta ServiceWorkerVersion::kStopWorkerTimeout;

void ServiceWorkerVersion::RestartTick(base::TimeTicks* time) const {
  *time = tick_clock_->NowTicks();
}

bool ServiceWorkerVersion::RequestExpired(
    const base::TimeTicks& expiration) const {
  if (expiration.is_null())
    return false;
  return tick_clock_->NowTicks() >= expiration;
}

base::TimeDelta ServiceWorkerVersion::GetTickDuration(
    const base::TimeTicks& time) const {
  if (time.is_null())
    return base::TimeDelta();
  return tick_clock_->NowTicks() - time;
}

// A controller for periodically sending a ping to the worker to see
// if the worker is not stalling.
class ServiceWorkerVersion::PingController {
 public:
  explicit PingController(ServiceWorkerVersion* version) : version_(version) {}
  ~PingController() {}

  void Activate() { ping_state_ = PINGING; }

  void Deactivate() {
    ClearTick(&ping_time_);
    ping_state_ = NOT_PINGING;
  }

  void OnPongReceived() { ClearTick(&ping_time_); }

  bool IsTimedOut() { return ping_state_ == PING_TIMED_OUT; }

  // Checks ping status. This is supposed to be called periodically.
  // This may call:
  // - OnPingTimeout() if the worker hasn't reponded within a certain period.
  // - PingWorker() if we're running ping timer and can send next ping.
  void CheckPingStatus() {
    if (version_->GetTickDuration(ping_time_) > kPingTimeout) {
      ping_state_ = PING_TIMED_OUT;
      version_->OnPingTimeout();
      return;
    }

    // Check if we want to send a next ping.
    if (ping_state_ != PINGING || !ping_time_.is_null())
      return;

    version_->PingWorker();
    version_->RestartTick(&ping_time_);
  }

  void SimulateTimeoutForTesting() {
    version_->PingWorker();
    ping_state_ = PING_TIMED_OUT;
    version_->OnPingTimeout();
  }

 private:
  enum PingState { NOT_PINGING, PINGING, PING_TIMED_OUT };
  ServiceWorkerVersion* version_;  // Not owned.
  base::TimeTicks ping_time_;
  PingState ping_state_ = NOT_PINGING;
  DISALLOW_COPY_AND_ASSIGN(PingController);
};

ServiceWorkerVersion::ServiceWorkerVersion(
    ServiceWorkerRegistration* registration,
    const GURL& script_url,
    int64_t version_id,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : version_id_(version_id),
      registration_id_(registration->id()),
      script_url_(script_url),
      script_origin_(url::Origin::Create(script_url_)),
      scope_(registration->pattern()),
      fetch_handler_existence_(FetchHandlerExistence::UNKNOWN),
      site_for_uma_(ServiceWorkerMetrics::SiteFromURL(scope_)),
      binding_(this),
      context_(context),
      script_cache_map_(this, context),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      clock_(base::DefaultClock::GetInstance()),
      ping_controller_(new PingController(this)),
      validator_(std::make_unique<blink::TrialTokenValidator>()),
      weak_factory_(this) {
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerVersionId, version_id);
  DCHECK(context_);
  DCHECK(registration);
  DCHECK(script_url_.is_valid());
  embedded_worker_ = context_->embedded_worker_registry()->CreateWorker(this);
  embedded_worker_->AddObserver(this);
  context_->AddLiveVersion(this);
}

ServiceWorkerVersion::~ServiceWorkerVersion() {
  in_dtor_ = true;

  // Record UMA if the worker was trying to start. One way we get here is if the
  // user closed the tab before the SW could start up.
  if (!start_callbacks_.empty()) {
    // RecordStartWorkerResult must be the first element of start_callbacks_.
    StatusCallback record_start_worker_result = std::move(start_callbacks_[0]);
    start_callbacks_.clear();
    std::move(record_start_worker_result).Run(SERVICE_WORKER_ERROR_ABORT);
  }

  if (context_)
    context_->RemoveLiveVersion(version_id_);

  if (running_status() == EmbeddedWorkerStatus::STARTING ||
      running_status() == EmbeddedWorkerStatus::RUNNING) {
    embedded_worker_->Stop();
  }
  embedded_worker_->RemoveObserver(this);
}

void ServiceWorkerVersion::SetNavigationPreloadState(
    const blink::mojom::NavigationPreloadState& state) {
  navigation_preload_state_ = state;
}

void ServiceWorkerVersion::SetStatus(Status status) {
  if (status_ == status)
    return;

  TRACE_EVENT2("ServiceWorker", "ServiceWorkerVersion::SetStatus", "Script URL",
               script_url_.spec(), "New Status", VersionStatusToString(status));

  // |fetch_handler_existence_| must be set before setting the status to
  // INSTALLED,
  // ACTIVATING or ACTIVATED.
  DCHECK(fetch_handler_existence_ != FetchHandlerExistence::UNKNOWN ||
         !(status == INSTALLED || status == ACTIVATING || status == ACTIVATED));

  status_ = status;
  if (skip_waiting_) {
    switch (status_) {
      case NEW:
        // |skip_waiting_| should not be set before the version is NEW.
        NOTREACHED();
        return;
      case INSTALLING:
        // Do nothing until INSTALLED time.
        break;
      case INSTALLED:
        // Start recording the time when the version is trying to skip waiting.
        RestartTick(&skip_waiting_time_);
        break;
      case ACTIVATING:
        // Do nothing until ACTIVATED time.
        break;
      case ACTIVATED:
        // Resolve skip waiting promises.
        ClearTick(&skip_waiting_time_);
        for (SkipWaitingCallback& callback : pending_skip_waiting_requests_) {
          std::move(callback).Run(true);
        }
        pending_skip_waiting_requests_.clear();
        break;
      case REDUNDANT:
        // Fail any pending skip waiting requests since this version is dead.
        for (SkipWaitingCallback& callback : pending_skip_waiting_requests_) {
          std::move(callback).Run(false);
        }
        pending_skip_waiting_requests_.clear();
        break;
    }
  }

  // OnVersionStateChanged() invokes updates of the status using state
  // change IPC at ServiceWorkerHandle (for JS-land on renderer process) and
  // ServiceWorkerContextCore (for devtools and serviceworker-internals).
  // This should be done before using the new status by
  // |status_change_callbacks_| which sends the IPC for resolving the .ready
  // property.
  // TODO(shimazu): Clarify the dependency of OnVersionStateChanged and
  // |status_change_callbacks_|
  for (auto& observer : listeners_)
    observer.OnVersionStateChanged(this);

  std::vector<base::OnceClosure> callbacks;
  callbacks.swap(status_change_callbacks_);
  for (auto& callback : callbacks)
    std::move(callback).Run();

  if (status == INSTALLED)
    embedded_worker_->OnWorkerVersionInstalled();
  else if (status == REDUNDANT)
    embedded_worker_->OnWorkerVersionDoomed();
}

void ServiceWorkerVersion::RegisterStatusChangeCallback(
    base::OnceClosure callback) {
  status_change_callbacks_.push_back(std::move(callback));
}

ServiceWorkerVersionInfo ServiceWorkerVersion::GetInfo() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ServiceWorkerVersionInfo info(
      running_status(), status(), fetch_handler_existence(), script_url(),
      registration_id(), version_id(), embedded_worker()->process_id(),
      embedded_worker()->thread_id(),
      embedded_worker()->worker_devtools_agent_route_id());
  for (const auto& controllee : controllee_map_) {
    const ServiceWorkerProviderHost* host = controllee.second;
    info.clients.insert(std::make_pair(
        host->client_uuid(),
        ServiceWorkerVersionInfo::ClientInfo(
            host->process_id(), host->route_id(), host->web_contents_getter(),
            host->provider_type())));
  }
  if (!main_script_http_info_)
    return info;
  info.script_response_time = main_script_http_info_->response_time;
  if (main_script_http_info_->headers)
    main_script_http_info_->headers->GetLastModifiedValue(
        &info.script_last_modified);
  return info;
}

void ServiceWorkerVersion::set_fetch_handler_existence(
    FetchHandlerExistence existence) {
  DCHECK_EQ(fetch_handler_existence_, FetchHandlerExistence::UNKNOWN);
  DCHECK_NE(existence, FetchHandlerExistence::UNKNOWN);
  fetch_handler_existence_ = existence;
  if (site_for_uma_ != ServiceWorkerMetrics::Site::OTHER)
    return;
  if (existence == FetchHandlerExistence::EXISTS)
    site_for_uma_ = ServiceWorkerMetrics::Site::WITH_FETCH_HANDLER;
  else
    site_for_uma_ = ServiceWorkerMetrics::Site::WITHOUT_FETCH_HANDLER;
}

void ServiceWorkerVersion::StartWorker(ServiceWorkerMetrics::EventType purpose,
                                       StatusCallback callback) {
  TRACE_EVENT_INSTANT2(
      "ServiceWorker", "ServiceWorkerVersion::StartWorker (instant)",
      TRACE_EVENT_SCOPE_THREAD, "Script", script_url_.spec(), "Purpose",
      ServiceWorkerMetrics::EventTypeToString(purpose));

  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const bool is_browser_startup_complete =
      GetContentClient()->browser()->IsBrowserStartupComplete();
  if (!context_) {
    RecordStartWorkerResult(purpose, status_, kInvalidTraceId,
                            is_browser_startup_complete,
                            SERVICE_WORKER_ERROR_ABORT);
    RunSoon(base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  if (is_redundant()) {
    RecordStartWorkerResult(purpose, status_, kInvalidTraceId,
                            is_browser_startup_complete,
                            SERVICE_WORKER_ERROR_REDUNDANT);
    RunSoon(
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_REDUNDANT));
    return;
  }
  if (!IsStartWorkerAllowed()) {
    RecordStartWorkerResult(purpose, status_, kInvalidTraceId,
                            is_browser_startup_complete,
                            SERVICE_WORKER_ERROR_DISALLOWED);
    RunSoon(
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_DISALLOWED));
    return;
  }

  // Ensure the live registration during starting worker so that the worker can
  // get associated with it in
  // ServiceWorkerProviderHost::CompleteStartWorkerPreparation.
  context_->storage()->FindRegistrationForId(
      registration_id_, scope_.GetOrigin(),
      base::BindOnce(
          &ServiceWorkerVersion::DidEnsureLiveRegistrationForStartWorker,
          weak_factory_.GetWeakPtr(), purpose, status_,
          is_browser_startup_complete, std::move(callback)));
}

void ServiceWorkerVersion::StopWorker(base::OnceClosure callback) {
  TRACE_EVENT_INSTANT2("ServiceWorker",
                       "ServiceWorkerVersion::StopWorker (instant)",
                       TRACE_EVENT_SCOPE_THREAD, "Script", script_url_.spec(),
                       "Status", VersionStatusToString(status_));

  switch (running_status()) {
    case EmbeddedWorkerStatus::STARTING:
    case EmbeddedWorkerStatus::RUNNING:
      embedded_worker_->Stop();
      if (running_status() == EmbeddedWorkerStatus::STOPPED) {
        RunSoon(std::move(callback));
        return;
      }
      stop_callbacks_.push_back(std::move(callback));
      return;
    case EmbeddedWorkerStatus::STOPPING:
      stop_callbacks_.push_back(std::move(callback));
      return;
    case EmbeddedWorkerStatus::STOPPED:
      RunSoon(std::move(callback));
      return;
  }
  NOTREACHED();
}

void ServiceWorkerVersion::ScheduleUpdate() {
  if (!context_)
    return;
  if (update_timer_.IsRunning()) {
    update_timer_.Reset();
    return;
  }
  if (is_update_scheduled_)
    return;
  is_update_scheduled_ = true;

  // Protect |this| until the timer fires, since we may be stopping
  // and soon no one might hold a reference to us.
  context_->ProtectVersion(base::WrapRefCounted(this));
  update_timer_.Start(FROM_HERE, kUpdateDelay,
                      base::Bind(&ServiceWorkerVersion::StartUpdate,
                                 weak_factory_.GetWeakPtr()));
}

void ServiceWorkerVersion::StartUpdate() {
  if (!context_)
    return;
  context_->storage()->FindRegistrationForId(
      registration_id_, scope_.GetOrigin(),
      base::BindOnce(&ServiceWorkerVersion::FoundRegistrationForUpdate,
                     weak_factory_.GetWeakPtr()));
}

void ServiceWorkerVersion::DeferScheduledUpdate() {
  if (update_timer_.IsRunning())
    update_timer_.Reset();
}

int ServiceWorkerVersion::StartRequest(
    ServiceWorkerMetrics::EventType event_type,
    StatusCallback error_callback) {
  return StartRequestWithCustomTimeout(event_type, std::move(error_callback),
                                       kRequestTimeout, KILL_ON_TIMEOUT);
}

int ServiceWorkerVersion::StartRequestWithCustomTimeout(
    ServiceWorkerMetrics::EventType event_type,
    StatusCallback error_callback,
    const base::TimeDelta& timeout,
    TimeoutBehavior timeout_behavior) {
  DCHECK_EQ(EmbeddedWorkerStatus::RUNNING, running_status())
      << "Can only start a request with a running worker.";
  DCHECK(event_type == ServiceWorkerMetrics::EventType::INSTALL ||
         event_type == ServiceWorkerMetrics::EventType::ACTIVATE ||
         event_type == ServiceWorkerMetrics::EventType::MESSAGE ||
         status() == ACTIVATED)
      << "Event of type " << static_cast<int>(event_type)
      << " can only be dispatched to an active worker: " << status();

  auto request = std::make_unique<InflightRequest>(
      std::move(error_callback), clock_->Now(), tick_clock_->NowTicks(),
      event_type);
  InflightRequest* request_rawptr = request.get();
  int request_id = inflight_requests_.Add(std::move(request));
  TRACE_EVENT_ASYNC_BEGIN2("ServiceWorker", "ServiceWorkerVersion::Request",
                           request_rawptr, "Request id", request_id,
                           "Event type",
                           ServiceWorkerMetrics::EventTypeToString(event_type));

  base::TimeTicks expiration_time = tick_clock_->NowTicks() + timeout;
  bool is_inserted = false;
  std::set<InflightRequestTimeoutInfo>::iterator iter;
  std::tie(iter, is_inserted) = request_timeouts_.emplace(
      request_id, event_type, expiration_time, timeout_behavior);
  DCHECK(is_inserted);
  request_rawptr->timeout_iter = iter;
  if (expiration_time > max_request_expiration_time_)
    max_request_expiration_time_ = expiration_time;

  // S13nServiceWorker:
  // Even if the worker is in the idle state, the new event which is about to
  // be dispatched will reset the idle status. That means the worker can receive
  // events directly from any clients, so we cannot trigger OnNoWork after this
  // point.
  idle_timer_fired_in_renderer_ = false;
  return request_id;
}

bool ServiceWorkerVersion::StartExternalRequest(
    const std::string& request_uuid) {
  // It's possible that the renderer is lying or the version started stopping
  // right around the time of the IPC.
  if (running_status() != EmbeddedWorkerStatus::RUNNING)
    return false;

  if (external_request_uuid_to_request_id_.count(request_uuid) > 0u)
    return false;

  int request_id =
      StartRequest(ServiceWorkerMetrics::EventType::EXTERNAL_REQUEST,
                   base::BindOnce(&ServiceWorkerVersion::CleanUpExternalRequest,
                                  this, request_uuid));
  external_request_uuid_to_request_id_[request_uuid] = request_id;
  return true;
}

bool ServiceWorkerVersion::FinishRequest(int request_id,
                                         bool was_handled,
                                         base::Time dispatch_event_time) {
  InflightRequest* request = inflight_requests_.Lookup(request_id);
  if (!request)
    return false;
  if (event_recorder_)
    event_recorder_->RecordEventHandledStatus(request->event_type, was_handled);
  ServiceWorkerMetrics::RecordEventDuration(
      request->event_type, tick_clock_->NowTicks() - request->start_time_ticks,
      was_handled);
  ServiceWorkerMetrics::RecordEventDispatchingDelay(
      request->event_type, dispatch_event_time - request->start_time,
      site_for_uma());

  RestartTick(&idle_time_);
  TRACE_EVENT_ASYNC_END1("ServiceWorker", "ServiceWorkerVersion::Request",
                         request, "Handled", was_handled);
  request_timeouts_.erase(request->timeout_iter);
  inflight_requests_.Remove(request_id);

  if (!HasWorkInBrowser())
    OnNoWorkInBrowser();
  return true;
}

bool ServiceWorkerVersion::FinishExternalRequest(
    const std::string& request_uuid) {
  // It's possible that the renderer is lying or the version started stopping
  // right around the time of the IPC.
  if (running_status() != EmbeddedWorkerStatus::RUNNING)
    return false;

  RequestUUIDToRequestIDMap::iterator iter =
      external_request_uuid_to_request_id_.find(request_uuid);
  if (iter != external_request_uuid_to_request_id_.end()) {
    int request_id = iter->second;
    external_request_uuid_to_request_id_.erase(iter);
    return FinishRequest(request_id, true, clock_->Now());
  }

  // It is possible that the request was cancelled or timed out before and we
  // won't find it in |external_request_uuid_to_request_id_|.
  // Return true so we don't kill the process.
  return true;
}

ServiceWorkerVersion::SimpleEventCallback
ServiceWorkerVersion::CreateSimpleEventCallback(int request_id) {
  // The weak reference to |this| is safe because storage of the callbacks, the
  // inflight responses of the ServiceWorkerEventDispatcher, is owned by |this|.
  return base::BindOnce(&ServiceWorkerVersion::OnSimpleEventFinished,
                        base::Unretained(this), request_id);
}

void ServiceWorkerVersion::RunAfterStartWorker(
    ServiceWorkerMetrics::EventType purpose,
    StatusCallback callback) {
  if (running_status() == EmbeddedWorkerStatus::RUNNING) {
    DCHECK(start_callbacks_.empty());
    std::move(callback).Run(SERVICE_WORKER_OK);
    return;
  }
  StartWorker(purpose,
              base::BindOnce(&RunCallbackAfterStartWorker,
                             weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ServiceWorkerVersion::AddControllee(
    ServiceWorkerProviderHost* provider_host) {
  const std::string& uuid = provider_host->client_uuid();
  CHECK(!provider_host->client_uuid().empty());
  DCHECK(!base::ContainsKey(controllee_map_, uuid));
  controllee_map_[uuid] = provider_host;
  // Keep the worker alive a bit longer right after a new controllee is added.
  RestartTick(&idle_time_);
  ClearTick(&no_controllees_time_);
  for (auto& observer : listeners_)
    observer.OnControlleeAdded(this, provider_host);
}

void ServiceWorkerVersion::RemoveControllee(
    ServiceWorkerProviderHost* provider_host) {
  const std::string& uuid = provider_host->client_uuid();
  DCHECK(base::ContainsKey(controllee_map_, uuid));
  controllee_map_.erase(uuid);
  for (auto& observer : listeners_)
    observer.OnControlleeRemoved(this, provider_host);
  if (!HasControllee()) {
    RestartTick(&no_controllees_time_);
    for (auto& observer : listeners_)
      observer.OnNoControllees(this);
  }
}

void ServiceWorkerVersion::OnStreamResponseStarted() {
  CHECK_LT(inflight_stream_response_count_, std::numeric_limits<int>::max());
  inflight_stream_response_count_++;
}

void ServiceWorkerVersion::OnStreamResponseFinished() {
  DCHECK_GT(inflight_stream_response_count_, 0);
  inflight_stream_response_count_--;
  if (!HasWorkInBrowser())
    OnNoWorkInBrowser();
}

void ServiceWorkerVersion::AddListener(Listener* listener) {
  listeners_.AddObserver(listener);
}

void ServiceWorkerVersion::RemoveListener(Listener* listener) {
  listeners_.RemoveObserver(listener);
}

void ServiceWorkerVersion::ReportError(ServiceWorkerStatusCode status,
                                       const std::string& status_message) {
  if (status_message.empty()) {
    OnReportException(base::UTF8ToUTF16(ServiceWorkerStatusToString(status)),
                      -1, -1, GURL());
  } else {
    OnReportException(base::UTF8ToUTF16(status_message), -1, -1, GURL());
  }
}

void ServiceWorkerVersion::ReportForceUpdateToDevTools() {
  embedded_worker_->AddMessageToConsole(blink::WebConsoleMessage::kLevelWarning,
                                        kForceUpdateInfoMessage);
}

void ServiceWorkerVersion::SetStartWorkerStatusCode(
    ServiceWorkerStatusCode status) {
  start_worker_status_ = status;
}

void ServiceWorkerVersion::Doom() {
  // Tell controllees that this version is dead. Each controllee will call
  // ServiceWorkerVersion::RemoveControllee(), so be careful with iterators.
  auto iter = controllee_map_.begin();
  while (iter != controllee_map_.end()) {
    ServiceWorkerProviderHost* host = iter->second;
    ++iter;
    host->NotifyControllerLost();
  }
  // Any controllee this version had should have removed itself.
  DCHECK(!HasControllee());

  SetStatus(REDUNDANT);
  if (running_status() == EmbeddedWorkerStatus::STARTING ||
      running_status() == EmbeddedWorkerStatus::RUNNING) {
    if (embedded_worker()->devtools_attached())
      stop_when_devtools_detached_ = true;
    else
      embedded_worker_->Stop();
  }
  if (!context_)
    return;
  std::vector<ServiceWorkerDatabase::ResourceRecord> resources;
  script_cache_map_.GetResources(&resources);
  context_->storage()->PurgeResources(resources);
}

void ServiceWorkerVersion::SetValidOriginTrialTokens(
    const blink::TrialTokenValidator::FeatureToTokensMap& tokens) {
  origin_trial_tokens_ = validator_->GetValidTokens(
      url::Origin::Create(scope()), tokens, clock_->Now());
}

void ServiceWorkerVersion::SetDevToolsAttached(bool attached) {
  embedded_worker()->SetDevToolsAttached(attached);

  if (stop_when_devtools_detached_ && !attached) {
    DCHECK_EQ(REDUNDANT, status());
    if (running_status() == EmbeddedWorkerStatus::STARTING ||
        running_status() == EmbeddedWorkerStatus::RUNNING) {
      embedded_worker_->Stop();
    }
    return;
  }
  if (attached) {
    // TODO(falken): Canceling the timeouts when debugging could cause
    // heisenbugs; we should instead run them as normal show an educational
    // message in DevTools when they occur. crbug.com/470419

    // Don't record the startup time metric once DevTools is attached.
    ClearTick(&start_time_);
    skip_recording_startup_time_ = true;

    // Cancel request timeouts.
    SetAllRequestExpirations(base::TimeTicks());
    return;
  }
  if (!start_callbacks_.empty()) {
    // Reactivate the timer for start timeout.
    DCHECK(timeout_timer_.IsRunning());
    DCHECK(running_status() == EmbeddedWorkerStatus::STARTING ||
           running_status() == EmbeddedWorkerStatus::STOPPING)
        << static_cast<int>(running_status());
    RestartTick(&start_time_);
  }

  // Reactivate request timeouts, setting them all to the same expiration time.
  SetAllRequestExpirations(tick_clock_->NowTicks() + kRequestTimeout);
}

void ServiceWorkerVersion::SetMainScriptHttpResponseInfo(
    const net::HttpResponseInfo& http_info) {
  main_script_http_info_.reset(new net::HttpResponseInfo(http_info));

  // Updates |origin_trial_tokens_| if it is not set yet. This happens when:
  //  1) The worker is a new one.
  //  OR
  //  2) The worker is an existing one but the entry in ServiceWorkerDatabase
  //     was written by old version Chrome (< M56), so |origin_trial_tokens|
  //     wasn't set in the entry.
  if (!origin_trial_tokens_) {
    origin_trial_tokens_ = validator_->GetValidTokensFromHeaders(
        url::Origin::Create(scope()), http_info.headers.get(), clock_->Now());
  }

  for (auto& observer : listeners_)
    observer.OnMainScriptHttpResponseInfoSet(this);
}

void ServiceWorkerVersion::SimulatePingTimeoutForTesting() {
  ping_controller_->SimulateTimeoutForTesting();
}

void ServiceWorkerVersion::SetTickClockForTesting(
    const base::TickClock* tick_clock) {
  tick_clock_ = tick_clock;
}

void ServiceWorkerVersion::SetClockForTesting(base::Clock* clock) {
  clock_ = clock;
}

const net::HttpResponseInfo*
ServiceWorkerVersion::GetMainScriptHttpResponseInfo() {
  return main_script_http_info_.get();
}

ServiceWorkerVersion::InflightRequestTimeoutInfo::InflightRequestTimeoutInfo(
    int id,
    ServiceWorkerMetrics::EventType event_type,
    const base::TimeTicks& expiration,
    TimeoutBehavior timeout_behavior)
    : id(id),
      event_type(event_type),
      expiration(expiration),
      timeout_behavior(timeout_behavior) {}

ServiceWorkerVersion::InflightRequestTimeoutInfo::
    ~InflightRequestTimeoutInfo() {}

bool ServiceWorkerVersion::InflightRequestTimeoutInfo::operator<(
    const InflightRequestTimeoutInfo& other) const {
  if (expiration == other.expiration)
    return id < other.id;
  return expiration < other.expiration;
}

ServiceWorkerVersion::InflightRequest::InflightRequest(
    StatusCallback callback,
    base::Time time,
    const base::TimeTicks& time_ticks,
    ServiceWorkerMetrics::EventType event_type)
    : error_callback(std::move(callback)),
      start_time(time),
      start_time_ticks(time_ticks),
      event_type(event_type) {}

ServiceWorkerVersion::InflightRequest::~InflightRequest() {}

void ServiceWorkerVersion::OnThreadStarted() {
  DCHECK_EQ(EmbeddedWorkerStatus::STARTING, running_status());
  // Activate ping/pong now that JavaScript execution will start.
  ping_controller_->Activate();
}

void ServiceWorkerVersion::OnStarting() {
  for (auto& observer : listeners_)
    observer.OnRunningStateChanged(this);
}

void ServiceWorkerVersion::OnStarted() {
  DCHECK_EQ(EmbeddedWorkerStatus::RUNNING, running_status());
  RestartTick(&idle_time_);

  // Fire all start callbacks.
  scoped_refptr<ServiceWorkerVersion> protect(this);
  FinishStartWorker(SERVICE_WORKER_OK);
  for (auto& observer : listeners_)
    observer.OnRunningStateChanged(this);
}

void ServiceWorkerVersion::OnStopping() {
  DCHECK(stop_time_.is_null());
  RestartTick(&stop_time_);
  TRACE_EVENT_ASYNC_BEGIN2("ServiceWorker", "ServiceWorkerVersion::StopWorker",
                           stop_time_.ToInternalValue(), "Script",
                           script_url_.spec(), "Version Status",
                           VersionStatusToString(status_));

  // Shorten the interval so stalling in stopped can be fixed quickly. Once the
  // worker stops, the timer is disabled. The interval will be reset to normal
  // when the worker starts up again.
  SetTimeoutTimerInterval(kStopWorkerTimeout);
  for (auto& observer : listeners_)
    observer.OnRunningStateChanged(this);
}

void ServiceWorkerVersion::OnStopped(EmbeddedWorkerStatus old_status) {
  if (IsInstalled(status())) {
    ServiceWorkerMetrics::RecordWorkerStopped(
        ServiceWorkerMetrics::StopStatus::NORMAL);
  }
  if (!stop_time_.is_null())
    ServiceWorkerMetrics::RecordStopWorkerTime(GetTickDuration(stop_time_));

  OnStoppedInternal(old_status);
}

void ServiceWorkerVersion::OnDetached(EmbeddedWorkerStatus old_status) {
  if (IsInstalled(status())) {
    ServiceWorkerMetrics::RecordWorkerStopped(
        ServiceWorkerMetrics::StopStatus::DETACH_BY_REGISTRY);
  }
  OnStoppedInternal(old_status);
}

void ServiceWorkerVersion::OnScriptLoaded() {
  DCHECK(GetMainScriptHttpResponseInfo());
  if (IsInstalled(status()))
    UMA_HISTOGRAM_BOOLEAN("ServiceWorker.ScriptLoadSuccess", true);
}

void ServiceWorkerVersion::OnScriptLoadFailed() {
  if (IsInstalled(status()))
    UMA_HISTOGRAM_BOOLEAN("ServiceWorker.ScriptLoadSuccess", false);
}

void ServiceWorkerVersion::OnRegisteredToDevToolsManager() {
  for (auto& observer : listeners_)
    observer.OnDevToolsRoutingIdChanged(this);
}

void ServiceWorkerVersion::OnReportException(
    const base::string16& error_message,
    int line_number,
    int column_number,
    const GURL& source_url) {
  for (auto& observer : listeners_) {
    observer.OnErrorReported(this, error_message, line_number, column_number,
                             source_url);
  }
}

void ServiceWorkerVersion::OnReportConsoleMessage(int source_identifier,
                                                  int message_level,
                                                  const base::string16& message,
                                                  int line_number,
                                                  const GURL& source_url) {
  for (auto& observer : listeners_) {
    observer.OnReportConsoleMessage(this, source_identifier, message_level,
                                    message, line_number, source_url);
  }
}

void ServiceWorkerVersion::OnStartSentAndScriptEvaluated(
    ServiceWorkerStatusCode status) {
  if (status != SERVICE_WORKER_OK) {
    scoped_refptr<ServiceWorkerVersion> protect(this);
    FinishStartWorker(DeduceStartWorkerFailureReason(status));
  }
}

void ServiceWorkerVersion::SetCachedMetadata(const GURL& url,
                                             const std::vector<uint8_t>& data) {
  int64_t callback_id = tick_clock_->NowTicks().ToInternalValue();
  TRACE_EVENT_ASYNC_BEGIN1("ServiceWorker",
                           "ServiceWorkerVersion::SetCachedMetadata",
                           callback_id, "URL", url.spec());
  script_cache_map_.WriteMetadata(
      url, data,
      base::Bind(&ServiceWorkerVersion::OnSetCachedMetadataFinished,
                 weak_factory_.GetWeakPtr(), callback_id, data.size()));
}

void ServiceWorkerVersion::ClearCachedMetadata(const GURL& url) {
  int64_t callback_id = tick_clock_->NowTicks().ToInternalValue();
  TRACE_EVENT_ASYNC_BEGIN1("ServiceWorker",
                           "ServiceWorkerVersion::ClearCachedMetadata",
                           callback_id, "URL", url.spec());
  script_cache_map_.ClearMetadata(
      url, base::Bind(&ServiceWorkerVersion::OnClearCachedMetadataFinished,
                      weak_factory_.GetWeakPtr(), callback_id));
}

void ServiceWorkerVersion::ClaimClients(ClaimClientsCallback callback) {
  if (status_ != ACTIVATING && status_ != ACTIVATED) {
    std::move(callback).Run(blink::mojom::ServiceWorkerErrorType::kState,
                            std::string(kClaimClientsStateErrorMesage));
    return;
  }
  if (!context_) {
    std::move(callback).Run(blink::mojom::ServiceWorkerErrorType::kAbort,
                            std::string(kClaimClientsShutdownErrorMesage));
    return;
  }

  ServiceWorkerRegistration* registration =
      context_->GetLiveRegistration(registration_id_);
  // Registration must be kept alive by ServiceWorkerGlobalScope#registration.
  if (!registration) {
    mojo::ReportBadMessage("ClaimClients: No live registration");
    // ReportBadMessage() will kill the renderer process, but Mojo complains if
    // the callback is not run. Just run it with nonsense arguments.
    std::move(callback).Run(blink::mojom::ServiceWorkerErrorType::kUnknown,
                            std::string());
    return;
  }

  registration->ClaimClients();
  std::move(callback).Run(blink::mojom::ServiceWorkerErrorType::kNone,
                          base::nullopt);
}

void ServiceWorkerVersion::GetClients(
    blink::mojom::ServiceWorkerClientQueryOptionsPtr options,
    GetClientsCallback callback) {
  service_worker_client_utils::GetClients(
      weak_factory_.GetWeakPtr(), std::move(options),
      base::BindOnce(&DidGetClients, std::move(callback)));
}

void ServiceWorkerVersion::GetClient(const std::string& client_uuid,
                                     GetClientCallback callback) {
  if (!context_) {
    // The promise will be resolved to 'undefined'.
    std::move(callback).Run(nullptr);
    return;
  }
  ServiceWorkerProviderHost* provider_host =
      context_->GetProviderHostByClientID(client_uuid);
  if (!provider_host ||
      provider_host->document_url().GetOrigin() != script_url_.GetOrigin()) {
    // The promise will be resolved to 'undefined'.
    // Note that we don't BadMessage here since Clients#get() can be passed an
    // arbitrary UUID. The BadMessages for the origin mismatches below are
    // appropriate because the UUID is taken directly from a Client object so we
    // expect it to be valid.
    std::move(callback).Run(nullptr);
    return;
  }
  service_worker_client_utils::GetClient(provider_host, std::move(callback));
}

void ServiceWorkerVersion::OpenNewTab(const GURL& url,
                                      OpenNewTabCallback callback) {
  OpenWindow(url, service_worker_client_utils::WindowType::NEW_TAB_WINDOW,
             std::move(callback));
}

void ServiceWorkerVersion::OpenPaymentHandlerWindow(
    const GURL& url,
    OpenPaymentHandlerWindowCallback callback) {
  // Just respond failure if we are shutting down.
  if (!context_) {
    std::move(callback).Run(
        false /* success */, nullptr /* client */,
        std::string("The service worker system is shutting down."));
    return;
  }

  PaymentHandlerSupport::ShowPaymentHandlerWindow(
      url, context_.get(),
      base::BindOnce(&DidShowPaymentHandlerWindow, url, context_),
      base::BindOnce(
          &ServiceWorkerVersion::OpenWindow, weak_factory_.GetWeakPtr(), url,
          service_worker_client_utils::WindowType::PAYMENT_HANDLER_WINDOW),
      std::move(callback));
}

void ServiceWorkerVersion::PostMessageToClient(
    const std::string& client_uuid,
    blink::TransferableMessage message) {
  if (!context_)
    return;
  ServiceWorkerProviderHost* provider_host =
      context_->GetProviderHostByClientID(client_uuid);
  if (!provider_host) {
    // The client may already have been closed, just ignore.
    return;
  }
  if (provider_host->document_url().GetOrigin() != script_url_.GetOrigin()) {
    mojo::ReportBadMessage(
        "Received Client#postMessage() request for a cross-origin client.");
    binding_.Close();
    return;
  }
  provider_host->PostMessageToClient(this, std::move(message));
}

void ServiceWorkerVersion::FocusClient(const std::string& client_uuid,
                                       FocusClientCallback callback) {
  if (!context_) {
    std::move(callback).Run(nullptr /* client */);
    return;
  }
  ServiceWorkerProviderHost* provider_host =
      context_->GetProviderHostByClientID(client_uuid);
  if (!provider_host) {
    // The client may already have been closed, just fail.
    std::move(callback).Run(nullptr /* client */);
    return;
  }
  if (provider_host->document_url().GetOrigin() != script_url_.GetOrigin()) {
    mojo::ReportBadMessage(
        "Received WindowClient#focus() request for a cross-origin client.");
    binding_.Close();
    return;
  }
  if (provider_host->client_type() !=
      blink::mojom::ServiceWorkerClientType::kWindow) {
    // focus() should be called only for WindowClient.
    mojo::ReportBadMessage(
        "Received WindowClient#focus() request for a non-window client.");
    binding_.Close();
    return;
  }

  service_worker_client_utils::FocusWindowClient(provider_host,
                                                 std::move(callback));
}

void ServiceWorkerVersion::NavigateClient(const std::string& client_uuid,
                                          const GURL& url,
                                          NavigateClientCallback callback) {
  if (!context_) {
    std::move(callback).Run(
        false /* success */, nullptr /* client */,
        std::string("The service worker system is shutting down."));
    return;
  }

  if (!url.is_valid() || !base::IsValidGUID(client_uuid)) {
    mojo::ReportBadMessage(
        "Received unexpected invalid URL/UUID from renderer process.");
    binding_.Close();
    return;
  }

  // Reject requests for URLs that the process is not allowed to access. It's
  // possible to receive such requests since the renderer-side checks are
  // slightly different. For example, the view-source scheme will not be
  // filtered out by Blink.
  if (!ChildProcessSecurityPolicyImpl::GetInstance()->CanRequestURL(
          embedded_worker_->process_id(), url)) {
    std::move(callback).Run(
        false /* success */, nullptr /* client */,
        "The service worker is not allowed to access URL: " + url.spec());
    return;
  }

  ServiceWorkerProviderHost* provider_host =
      context_->GetProviderHostByClientID(client_uuid);
  if (!provider_host) {
    std::move(callback).Run(false /* success */, nullptr /* client */,
                            std::string("The client was not found."));
    return;
  }
  if (provider_host->document_url().GetOrigin() != script_url_.GetOrigin()) {
    mojo::ReportBadMessage(
        "Received WindowClient#navigate() request for a cross-origin client.");
    binding_.Close();
    return;
  }
  if (provider_host->client_type() !=
      blink::mojom::ServiceWorkerClientType::kWindow) {
    // navigate() should be called only for WindowClient.
    mojo::ReportBadMessage(
        "Received WindowClient#navigate() request for a non-window client.");
    binding_.Close();
    return;
  }
  if (provider_host->active_version() != this) {
    std::move(callback).Run(
        false /* success */, nullptr /* client */,
        std::string(
            "This service worker is not the client's active service worker."));
    return;
  }

  service_worker_client_utils::NavigateClient(
      url, script_url_, provider_host->process_id(), provider_host->frame_id(),
      context_, base::BindOnce(&DidNavigateClient, std::move(callback), url));
}

void ServiceWorkerVersion::SkipWaiting(SkipWaitingCallback callback) {
  skip_waiting_ = true;

  // Per spec, resolve the skip waiting promise now if activation won't be
  // triggered here. The ActivateWaitingVersionWhenReady() call below only
  // triggers it if we're in INSTALLED state. So if we're not in INSTALLED
  // state, resolve the promise now. Even if we're in INSTALLED state, there are
  // still cases where ActivateWaitingVersionWhenReady() won't trigger the
  // activation. In that case, it's a slight spec violation to not resolve now,
  // but we'll eventually resolve the promise in SetStatus().
  if (status_ != INSTALLED) {
    std::move(callback).Run(true);
    return;
  }

  if (!context_) {
    std::move(callback).Run(false);
    return;
  }
  ServiceWorkerRegistration* registration =
      context_->GetLiveRegistration(registration_id_);
  // TODO(leonhsl): Here we should be guaranteed a registration since
  // ServiceWorkerGlobalScope#registration should be keeping the registration
  // alive currently. So we need to confirm and remove this nullable check
  // later.
  if (!registration) {
    std::move(callback).Run(false);
    return;
  }
  if (skip_waiting_time_.is_null())
    RestartTick(&skip_waiting_time_);
  pending_skip_waiting_requests_.push_back(std::move(callback));
  if (pending_skip_waiting_requests_.size() == 1)
    registration->ActivateWaitingVersionWhenReady();
}

void ServiceWorkerVersion::OnSetCachedMetadataFinished(int64_t callback_id,
                                                       size_t size,
                                                       int result) {
  TRACE_EVENT_ASYNC_END1("ServiceWorker",
                         "ServiceWorkerVersion::SetCachedMetadata", callback_id,
                         "result", result);
  for (auto& listener : listeners_)
    listener.OnCachedMetadataUpdated(this, size);
}

void ServiceWorkerVersion::OnClearCachedMetadataFinished(int64_t callback_id,
                                                         int result) {
  TRACE_EVENT_ASYNC_END1("ServiceWorker",
                         "ServiceWorkerVersion::ClearCachedMetadata",
                         callback_id, "result", result);
  for (auto& listener : listeners_)
    listener.OnCachedMetadataUpdated(this, 0);
}

void ServiceWorkerVersion::OpenWindow(
    GURL url,
    service_worker_client_utils::WindowType type,
    OpenNewTabCallback callback) {
  // Just respond failure if we are shutting down.
  if (!context_) {
    std::move(callback).Run(
        false /* success */, nullptr /* client */,
        std::string("The service worker system is shutting down."));
    return;
  }

  if (!url.is_valid()) {
    mojo::ReportBadMessage(
        "Received unexpected invalid URL from renderer process.");
    binding_.Close();
    return;
  }

  // The renderer treats all URLs in the about: scheme as being about:blank.
  // Canonicalize about: URLs to about:blank.
  if (url.SchemeIs(url::kAboutScheme))
    url = GURL(url::kAboutBlankURL);

  // Reject requests for URLs that the process is not allowed to access. It's
  // possible to receive such requests since the renderer-side checks are
  // slightly different. For example, the view-source scheme will not be
  // filtered out by Blink.
  if (!ChildProcessSecurityPolicyImpl::GetInstance()->CanRequestURL(
          embedded_worker_->process_id(), url)) {
    std::move(callback).Run(false /* success */, nullptr /* client */,
                            url.spec() + " cannot be opened.");
    return;
  }

  service_worker_client_utils::OpenWindow(
      url, script_url_, embedded_worker_->process_id(), context_, type,
      base::BindOnce(&OnOpenWindowFinished, std::move(callback)));
}

void ServiceWorkerVersion::OnSimpleEventFinished(
    int request_id,
    blink::mojom::ServiceWorkerEventStatus status,
    base::Time dispatch_event_time) {
  InflightRequest* request = inflight_requests_.Lookup(request_id);
  // |request| will be null when the request has been timed out.
  if (!request)
    return;
  // Copy error callback before calling FinishRequest.
  StatusCallback callback = std::move(request->error_callback);

  FinishRequest(request_id,
                status == blink::mojom::ServiceWorkerEventStatus::COMPLETED,
                dispatch_event_time);

  std::move(callback).Run(mojo::ConvertTo<ServiceWorkerStatusCode>(status));
}

void ServiceWorkerVersion::CountFeature(blink::mojom::WebFeature feature) {
  if (!used_features_.insert(feature).second)
    return;
  for (auto provider_host_by_uuid : controllee_map_)
    provider_host_by_uuid.second->CountFeature(feature);
}

bool ServiceWorkerVersion::IsInstalled(ServiceWorkerVersion::Status status) {
  switch (status) {
    case ServiceWorkerVersion::NEW:
    case ServiceWorkerVersion::INSTALLING:
    case ServiceWorkerVersion::REDUNDANT:
      return false;
    case ServiceWorkerVersion::INSTALLED:
    case ServiceWorkerVersion::ACTIVATING:
    case ServiceWorkerVersion::ACTIVATED:
      return true;
  }
  NOTREACHED() << "Unexpected status: " << status;
  return false;
}

void ServiceWorkerVersion::OnPongFromWorker() {
  ping_controller_->OnPongReceived();
}

void ServiceWorkerVersion::DidEnsureLiveRegistrationForStartWorker(
    ServiceWorkerMetrics::EventType purpose,
    Status prestart_status,
    bool is_browser_startup_complete,
    StatusCallback callback,
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  scoped_refptr<ServiceWorkerRegistration> protect = registration;
  if (status == SERVICE_WORKER_ERROR_NOT_FOUND) {
    // When the registration has already been deleted from the storage but its
    // active worker is still controlling clients, the event should be
    // dispatched on the worker. However, the storage cannot find the
    // registration. To handle the case, check the live registrations here.
    protect = context_->GetLiveRegistration(registration_id_);
    if (protect) {
      DCHECK(protect->is_deleted());
      status = SERVICE_WORKER_OK;
    }
  }
  if (status != SERVICE_WORKER_OK) {
    RecordStartWorkerResult(purpose, prestart_status, kInvalidTraceId,
                            is_browser_startup_complete, status);
    RunSoon(base::BindOnce(std::move(callback),
                           SERVICE_WORKER_ERROR_START_WORKER_FAILED));
    return;
  }
  if (is_redundant()) {
    RecordStartWorkerResult(purpose, prestart_status, kInvalidTraceId,
                            is_browser_startup_complete,
                            SERVICE_WORKER_ERROR_REDUNDANT);
    RunSoon(
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_REDUNDANT));
    return;
  }

  MarkIfStale();

  switch (running_status()) {
    case EmbeddedWorkerStatus::RUNNING:
      RunSoon(base::BindOnce(std::move(callback), SERVICE_WORKER_OK));
      return;
    case EmbeddedWorkerStatus::STARTING:
      DCHECK(!start_callbacks_.empty());
      break;
    case EmbeddedWorkerStatus::STOPPING:
    case EmbeddedWorkerStatus::STOPPED:
      if (start_callbacks_.empty()) {
        int trace_id = NextTraceId();
        TRACE_EVENT_ASYNC_BEGIN2(
            "ServiceWorker", "ServiceWorkerVersion::StartWorker", trace_id,
            "Script", script_url_.spec(), "Purpose",
            ServiceWorkerMetrics::EventTypeToString(purpose));
        DCHECK(!start_worker_first_purpose_);
        start_worker_first_purpose_ = purpose;
        start_callbacks_.push_back(
            base::BindOnce(&ServiceWorkerVersion::RecordStartWorkerResult,
                           weak_factory_.GetWeakPtr(), purpose, prestart_status,
                           trace_id, is_browser_startup_complete));
      }
      break;
  }

  // Keep the live registration while starting the worker.
  start_callbacks_.push_back(base::BindOnce(
      [](StatusCallback callback,
         scoped_refptr<ServiceWorkerRegistration> protect,
         ServiceWorkerStatusCode status) { std::move(callback).Run(status); },
      std::move(callback), protect));

  if (running_status() == EmbeddedWorkerStatus::STOPPED)
    StartWorkerInternal();
  DCHECK(timeout_timer_.IsRunning());
}

void ServiceWorkerVersion::StartWorkerInternal() {
  DCHECK_EQ(EmbeddedWorkerStatus::STOPPED, running_status());
  DCHECK(inflight_requests_.IsEmpty());
  DCHECK(request_timeouts_.empty());
  DCHECK(start_worker_first_purpose_);

  if (!ServiceWorkerMetrics::ShouldExcludeSiteFromHistogram(site_for_uma_)) {
    DCHECK(!event_recorder_);
    event_recorder_ =
        std::make_unique<ServiceWorkerMetrics::ScopedEventRecorder>(
            start_worker_first_purpose_.value());
  }
  // We don't clear |start_worker_first_purpose_| here but clear in
  // FinishStartWorker. This is because StartWorkerInternal may be called
  // again from OnStoppedInternal if StopWorker is called before OnStarted.

  StartTimeoutTimer();
  idle_timer_fired_in_renderer_ = false;

  std::unique_ptr<ServiceWorkerProviderHost> provider_host =
      ServiceWorkerProviderHost::PreCreateForController(context());
  provider_host_ = provider_host->AsWeakPtr();

  auto params = mojom::EmbeddedWorkerStartParams::New();
  params->service_worker_version_id = version_id_;
  params->scope = scope_;
  params->script_url = script_url_;
  params->is_installed = IsInstalled(status_);
  params->pause_after_download = pause_after_download_;

  if (IsInstalled(status()) && !pause_after_download_) {
    DCHECK(!installed_scripts_sender_);
    installed_scripts_sender_ =
        std::make_unique<ServiceWorkerInstalledScriptsSender>(this);
    params->installed_scripts_info =
        installed_scripts_sender_->CreateInfoAndBind();
    installed_scripts_sender_->Start();
  }

  params->dispatcher_request = mojo::MakeRequest(&event_dispatcher_);
  // TODO(horo): These CHECKs are for debugging crbug.com/759938.
  CHECK(event_dispatcher_.is_bound());
  CHECK(params->dispatcher_request.is_pending());
  event_dispatcher_.set_connection_error_handler(base::BindOnce(
      &OnEventDispatcherConnectionError, embedded_worker_->AsWeakPtr()));
  blink::mojom::ServiceWorkerHostAssociatedPtrInfo service_worker_host;
  binding_.Close();
  binding_.Bind(mojo::MakeRequest(&service_worker_host));
  ServiceWorkerRegistration* registration =
      context_->GetLiveRegistration(registration_id_);
  DCHECK(registration);
  provider_host->SetDocumentUrl(script_url());
  event_dispatcher_->InitializeGlobalScope(
      std::move(service_worker_host),
      provider_host->CreateServiceWorkerRegistrationObjectInfo(
          scoped_refptr<ServiceWorkerRegistration>(registration)));

  // S13nServiceWorker:
  if (!controller_request_.is_pending()) {
    DCHECK(!controller_ptr_.is_bound());
    controller_request_ = mojo::MakeRequest(&controller_ptr_);
  }
  params->controller_request = std::move(controller_request_);

  embedded_worker_->Start(
      std::move(params),
      // Unretained is used here because the callback will be owned by
      // |embedded_worker_| whose owner is |this|.
      base::BindOnce(&CompleteProviderHostPreparation, base::Unretained(this),
                     std::move(provider_host), context()),
      base::BindOnce(&ServiceWorkerVersion::OnStartSentAndScriptEvaluated,
                     weak_factory_.GetWeakPtr()));
}

void ServiceWorkerVersion::StartTimeoutTimer() {
  DCHECK(!timeout_timer_.IsRunning());

  if (embedded_worker_->devtools_attached()) {
    // Don't record the startup time metric once DevTools is attached.
    ClearTick(&start_time_);
    skip_recording_startup_time_ = true;
  } else {
    RestartTick(&start_time_);
    skip_recording_startup_time_ = false;
  }

  // The worker is starting up and not yet idle.
  ClearTick(&idle_time_);

  // Ping will be activated in OnScriptLoaded.
  ping_controller_->Deactivate();

  timeout_timer_.Start(FROM_HERE, kTimeoutTimerDelay, this,
                       &ServiceWorkerVersion::OnTimeoutTimer);
}

void ServiceWorkerVersion::StopTimeoutTimer() {
  timeout_timer_.Stop();
  ClearTick(&idle_time_);

  // Trigger update if worker is stale.
  if (!in_dtor_ && !stale_time_.is_null()) {
    ClearTick(&stale_time_);
    if (!update_timer_.IsRunning())
      ScheduleUpdate();
  }
}

void ServiceWorkerVersion::SetTimeoutTimerInterval(base::TimeDelta interval) {
  DCHECK(timeout_timer_.IsRunning());
  if (timeout_timer_.GetCurrentDelay() != interval) {
    timeout_timer_.Stop();
    timeout_timer_.Start(FROM_HERE, interval, this,
                         &ServiceWorkerVersion::OnTimeoutTimer);
  }
}

void ServiceWorkerVersion::OnTimeoutTimer() {
  // TODO(horo): This CHECK is for debugging crbug.com/759938.
  CHECK(running_status() == EmbeddedWorkerStatus::STARTING ||
        running_status() == EmbeddedWorkerStatus::RUNNING ||
        running_status() == EmbeddedWorkerStatus::STOPPING)
      << static_cast<int>(running_status());

  if (!context_)
    return;

  MarkIfStale();

  // Stopping the worker hasn't finished within a certain period.
  if (GetTickDuration(stop_time_) > kStopWorkerTimeout) {
    DCHECK_EQ(EmbeddedWorkerStatus::STOPPING, running_status());
    if (IsInstalled(status())) {
      ServiceWorkerMetrics::RecordWorkerStopped(
          ServiceWorkerMetrics::StopStatus::TIMEOUT);
    }
    ReportError(SERVICE_WORKER_ERROR_TIMEOUT, "DETACH_STALLED_IN_STOPPING");

    // Detach the worker. Remove |this| as a listener first; otherwise
    // OnStoppedInternal might try to restart before the new worker
    // is created. Also, protect |this|, since swapping out the
    // EmbeddedWorkerInstance could destroy our ServiceWorkerProviderHost
    // which could in turn destroy |this|.
    scoped_refptr<ServiceWorkerVersion> protect_this(this);
    embedded_worker_->RemoveObserver(this);
    embedded_worker_->Detach();
    embedded_worker_ = context_->embedded_worker_registry()->CreateWorker(this);
    embedded_worker_->AddObserver(this);

    // Call OnStoppedInternal to fail callbacks and possibly restart.
    OnStoppedInternal(EmbeddedWorkerStatus::STOPPING);
    return;
  }

  // Trigger update if worker is stale and we waited long enough for it to go
  // idle.
  if (GetTickDuration(stale_time_) > kRequestTimeout) {
    ClearTick(&stale_time_);
    if (!update_timer_.IsRunning())
      ScheduleUpdate();
  }

  // Starting a worker hasn't finished within a certain period.
  const base::TimeDelta start_limit = IsInstalled(status())
                                          ? kStartInstalledWorkerTimeout
                                          : kStartNewWorkerTimeout;
  if (GetTickDuration(start_time_) > start_limit) {
    DCHECK(running_status() == EmbeddedWorkerStatus::STARTING ||
           running_status() == EmbeddedWorkerStatus::STOPPING)
        << static_cast<int>(running_status());
    scoped_refptr<ServiceWorkerVersion> protect(this);
    FinishStartWorker(SERVICE_WORKER_ERROR_TIMEOUT);
    if (running_status() == EmbeddedWorkerStatus::STARTING)
      embedded_worker_->Stop();
    return;
  }

  // Requests have not finished before their expiration.
  bool stop_for_timeout = false;
  auto timeout_iter = request_timeouts_.begin();
  while (timeout_iter != request_timeouts_.end()) {
    const InflightRequestTimeoutInfo& info = *timeout_iter;
    if (!RequestExpired(info.expiration))
      break;
    if (MaybeTimeoutRequest(info)) {
      stop_for_timeout =
          stop_for_timeout || info.timeout_behavior == KILL_ON_TIMEOUT;
      ServiceWorkerMetrics::RecordEventTimeout(info.event_type);
    }
    timeout_iter = request_timeouts_.erase(timeout_iter);
  }
  if (stop_for_timeout && running_status() != EmbeddedWorkerStatus::STOPPING)
    embedded_worker_->Stop();

  // For the timeouts below, there are no callbacks to timeout so there is
  // nothing more to do if the worker is already stopping.
  if (running_status() == EmbeddedWorkerStatus::STOPPING)
    return;

  // The worker has been idle for longer than a certain period.
  // S13nServiceWorker: The idle timer is implemented on the renderer, so we can
  // skip this check.
  if (!ServiceWorkerUtils::IsServicificationEnabled() &&
      GetTickDuration(idle_time_) > kIdleWorkerTimeout) {
    StopWorkerIfIdle(false /* requested_from_renderer */);
    return;
  }

  // Check ping status.
  ping_controller_->CheckPingStatus();
}

void ServiceWorkerVersion::PingWorker() {
  // TODO(horo): This CHECK is for debugging crbug.com/759938.
  CHECK(running_status() == EmbeddedWorkerStatus::STARTING ||
        running_status() == EmbeddedWorkerStatus::RUNNING);
  // base::Unretained here is safe because event_dispatcher is owned by |this|.
  event_dispatcher()->Ping(base::BindOnce(
      &ServiceWorkerVersion::OnPongFromWorker, base::Unretained(this)));
}

void ServiceWorkerVersion::OnPingTimeout() {
  DCHECK(running_status() == EmbeddedWorkerStatus::STARTING ||
         running_status() == EmbeddedWorkerStatus::RUNNING);
  // TODO(falken): Change the error code to SERVICE_WORKER_ERROR_TIMEOUT.
  embedded_worker_->AddMessageToConsole(blink::WebConsoleMessage::kLevelVerbose,
                                        kNotRespondingErrorMesage);
  StopWorkerIfIdle(false /* requested_from_renderer */);
}

void ServiceWorkerVersion::StopWorkerIfIdle(bool requested_from_renderer) {
  if (running_status() == EmbeddedWorkerStatus::STOPPED ||
      running_status() == EmbeddedWorkerStatus::STOPPING ||
      !stop_callbacks_.empty()) {
    return;
  }

  if (!ServiceWorkerUtils::IsServicificationEnabled()) {
    // StopWorkerIfIdle() may be called for two reasons: "idle-timeout" or
    // "ping-timeout". For idle-timeout (i.e. ping hasn't timed out), check if
    // the worker really is idle.
    if (!ping_controller_->IsTimedOut() && HasWorkInBrowser())
      return;
    embedded_worker_->StopIfNotAttachedToDevTools();
    return;
  }

  DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
  // Ping timeout
  if (ping_controller_->IsTimedOut()) {
    DCHECK(!requested_from_renderer);
    embedded_worker_->StopIfNotAttachedToDevTools();
    return;
  }

  // Idle timeout
  DCHECK(requested_from_renderer);
  DCHECK(start_callbacks_.empty());
  idle_timer_fired_in_renderer_ = true;

  // We may still have some work in the browser-side that are not
  // observable by the renderer, i.e. response streaming. In such case
  // we return here with setting |idle_timer_fired_in_renderer_| to true.
  // It will be checked later (i.e. when streaming finishes) to see if we
  // we should fire OnNoWork().
  if (HasWorkInBrowser())
    return;
  embedded_worker_->StopIfNotAttachedToDevTools();
  OnNoWorkInBrowser();
}

bool ServiceWorkerVersion::HasWorkInBrowser() const {
  return !inflight_requests_.IsEmpty() || inflight_stream_response_count_ > 0 ||
         !start_callbacks_.empty();
}

void ServiceWorkerVersion::RecordStartWorkerResult(
    ServiceWorkerMetrics::EventType purpose,
    Status prestart_status,
    int trace_id,
    bool is_browser_startup_complete,
    ServiceWorkerStatusCode status) {
  if (trace_id != kInvalidTraceId) {
    TRACE_EVENT_ASYNC_END1("ServiceWorker", "ServiceWorkerVersion::StartWorker",
                           trace_id, "Status",
                           ServiceWorkerStatusToString(status));
  }
  base::TimeTicks start_time = start_time_;
  ClearTick(&start_time_);

  if (context_ && IsInstalled(prestart_status))
    context_->UpdateVersionFailureCount(version_id_, status);

  if (installed_scripts_sender_) {
    ServiceWorkerMetrics::RecordInstalledScriptsSenderStatus(
        installed_scripts_sender_->last_finished_reason());
  }
  ServiceWorkerMetrics::RecordStartWorkerStatus(status, purpose,
                                                IsInstalled(prestart_status));

  if (status == SERVICE_WORKER_OK && !start_time.is_null() &&
      !skip_recording_startup_time_) {
    ServiceWorkerMetrics::RecordStartWorkerTime(
        GetTickDuration(start_time), IsInstalled(prestart_status),
        embedded_worker_->start_situation(), purpose);
  }

  if (status != SERVICE_WORKER_ERROR_TIMEOUT)
    return;
  EmbeddedWorkerInstance::StartingPhase phase =
      EmbeddedWorkerInstance::NOT_STARTING;
  EmbeddedWorkerStatus running_status = embedded_worker_->status();
  // Build an artifical JavaScript exception to show in the ServiceWorker
  // log for developers; it's not user-facing so it's not a localized resource.
  std::string message = "ServiceWorker startup timed out. ";
  if (running_status != EmbeddedWorkerStatus::STARTING) {
    message.append("The worker had unexpected status: ");
    message.append(EmbeddedWorkerInstance::StatusToString(running_status));
  } else {
    phase = embedded_worker_->starting_phase();
    message.append("The worker was in startup phase: ");
    message.append(EmbeddedWorkerInstance::StartingPhaseToString(phase));
  }
  message.append(".");
  OnReportException(base::UTF8ToUTF16(message), -1, -1, GURL());
  DVLOG(1) << message;
  UMA_HISTOGRAM_ENUMERATION("ServiceWorker.StartWorker.TimeoutPhase",
                            phase,
                            EmbeddedWorkerInstance::STARTING_PHASE_MAX_VALUE);
}

bool ServiceWorkerVersion::MaybeTimeoutRequest(
    const InflightRequestTimeoutInfo& info) {
  InflightRequest* request = inflight_requests_.Lookup(info.id);
  if (!request)
    return false;

  TRACE_EVENT_ASYNC_END1("ServiceWorker", "ServiceWorkerVersion::Request",
                         request, "Error", "Timeout");
  std::move(request->error_callback).Run(SERVICE_WORKER_ERROR_TIMEOUT);
  inflight_requests_.Remove(info.id);
  return true;
}

void ServiceWorkerVersion::SetAllRequestExpirations(
    const base::TimeTicks& expiration) {
  std::set<InflightRequestTimeoutInfo> new_timeouts;
  for (const auto& info : request_timeouts_) {
    bool is_inserted = false;
    std::set<InflightRequestTimeoutInfo>::iterator iter;
    std::tie(iter, is_inserted) = new_timeouts.emplace(
        info.id, info.event_type, expiration, info.timeout_behavior);
    DCHECK(is_inserted);
    InflightRequest* request = inflight_requests_.Lookup(info.id);
    DCHECK(request);
    request->timeout_iter = iter;
  }
  request_timeouts_.swap(new_timeouts);
}

ServiceWorkerStatusCode ServiceWorkerVersion::DeduceStartWorkerFailureReason(
    ServiceWorkerStatusCode default_code) {
  if (ping_controller_->IsTimedOut())
    return SERVICE_WORKER_ERROR_TIMEOUT;

  if (start_worker_status_ != SERVICE_WORKER_OK)
    return start_worker_status_;

  const net::URLRequestStatus& main_script_status =
      script_cache_map()->main_script_status();
  if (main_script_status.status() != net::URLRequestStatus::SUCCESS) {
    if (net::IsCertificateError(main_script_status.error()))
      return SERVICE_WORKER_ERROR_SECURITY;
    switch (main_script_status.error()) {
      case net::ERR_INSECURE_RESPONSE:
      case net::ERR_UNSAFE_REDIRECT:
        return SERVICE_WORKER_ERROR_SECURITY;
      case net::ERR_ABORTED:
        return SERVICE_WORKER_ERROR_ABORT;
      default:
        return SERVICE_WORKER_ERROR_NETWORK;
    }
  }

  return default_code;
}

void ServiceWorkerVersion::MarkIfStale() {
  if (!context_)
    return;
  if (update_timer_.IsRunning() || !stale_time_.is_null())
    return;
  ServiceWorkerRegistration* registration =
      context_->GetLiveRegistration(registration_id_);
  if (!registration || registration->active_version() != this)
    return;
  base::TimeDelta time_since_last_check =
      clock_->Now() - registration->last_update_check();
  if (time_since_last_check > kServiceWorkerScriptMaxCacheAge)
    RestartTick(&stale_time_);
}

void ServiceWorkerVersion::FoundRegistrationForUpdate(
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  if (!context_)
    return;

  const scoped_refptr<ServiceWorkerVersion> protect = this;
  if (is_update_scheduled_) {
    context_->UnprotectVersion(version_id_);
    is_update_scheduled_ = false;
  }

  if (status != SERVICE_WORKER_OK || registration->active_version() != this)
    return;
  context_->UpdateServiceWorker(registration.get(),
                                false /* force_bypass_cache */);
}

void ServiceWorkerVersion::OnStoppedInternal(EmbeddedWorkerStatus old_status) {
  DCHECK_EQ(EmbeddedWorkerStatus::STOPPED, running_status());
  scoped_refptr<ServiceWorkerVersion> protect;
  if (!in_dtor_)
    protect = this;

  event_recorder_.reset();

  // |start_callbacks_| can be non-empty if a start worker request arrived while
  // the worker was stopping. The worker must be restarted to fulfill the
  // request.
  bool should_restart = !start_callbacks_.empty();
  if (is_redundant() || in_dtor_) {
    // This worker will be destroyed soon.
    should_restart = false;
  } else if (ping_controller_->IsTimedOut()) {
    // This worker is unresponsive and restart may fail.
    should_restart = false;
  } else if (old_status == EmbeddedWorkerStatus::STARTING) {
    // This worker unexpectedly stopped because start failed.  Attempting to
    // restart on start failure could cause an endless loop of start attempts,
    // so don't try to restart now.
    should_restart = false;
  }

  if (!stop_time_.is_null()) {
    TRACE_EVENT_ASYNC_END1("ServiceWorker", "ServiceWorkerVersion::StopWorker",
                           stop_time_.ToInternalValue(), "Restart",
                           should_restart);
    ClearTick(&stop_time_);
  }
  idle_timer_fired_in_renderer_ = false;
  StopTimeoutTimer();

  // Fire all stop callbacks.
  std::vector<base::OnceClosure> callbacks;
  callbacks.swap(stop_callbacks_);
  for (auto& callback : callbacks)
    std::move(callback).Run();

  if (!should_restart) {
    // Let all start callbacks fail.
    FinishStartWorker(DeduceStartWorkerFailureReason(
        SERVICE_WORKER_ERROR_START_WORKER_FAILED));
  }

  // Let all message callbacks fail (this will also fire and clear all
  // callbacks for events).
  // TODO(kinuko): Consider if we want to add queue+resend mechanism here.
  base::IDMap<std::unique_ptr<InflightRequest>>::iterator iter(
      &inflight_requests_);
  while (!iter.IsAtEnd()) {
    TRACE_EVENT_ASYNC_END1("ServiceWorker", "ServiceWorkerVersion::Request",
                           iter.GetCurrentValue(), "Error", "Worker Stopped");
    std::move(iter.GetCurrentValue()->error_callback)
        .Run(SERVICE_WORKER_ERROR_FAILED);
    iter.Advance();
  }
  inflight_requests_.Clear();
  request_timeouts_.clear();
  external_request_uuid_to_request_id_.clear();
  event_dispatcher_.reset();
  controller_ptr_.reset();
  DCHECK(!controller_request_.is_pending());
  installed_scripts_sender_.reset();
  binding_.Close();

  for (auto& observer : listeners_)
    observer.OnRunningStateChanged(this);
  if (should_restart) {
    StartWorkerInternal();
  } else if (!HasWorkInBrowser()) {
    OnNoWorkInBrowser();
  }
}

void ServiceWorkerVersion::FinishStartWorker(ServiceWorkerStatusCode status) {
  start_worker_first_purpose_ = base::nullopt;
  RunCallbacks(this, &start_callbacks_, status);
}

void ServiceWorkerVersion::CleanUpExternalRequest(
    const std::string& request_uuid,
    ServiceWorkerStatusCode status) {
  if (status == SERVICE_WORKER_OK)
    return;
  external_request_uuid_to_request_id_.erase(request_uuid);
}

void ServiceWorkerVersion::OnNoWorkInBrowser() {
  DCHECK(!HasWorkInBrowser());
  if (!ServiceWorkerUtils::IsServicificationEnabled()) {
    for (auto& observer : listeners_)
      observer.OnNoWork(this);
    return;
  }

  DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
  if (!idle_timer_fired_in_renderer_ &&
      running_status() != EmbeddedWorkerStatus::STOPPED) {
    return;
  }

  for (auto& observer : listeners_)
    observer.OnNoWork(this);
  idle_timer_fired_in_renderer_ = false;
}

bool ServiceWorkerVersion::IsStartWorkerAllowed() const {
  // Check that the worker is allowed on this origin. It's possible a
  // worker was previously allowed and installed, but later the embedder's
  // policy or binary changed to disallow this origin.
  if (!ServiceWorkerUtils::AllOriginsMatchAndCanAccessServiceWorkers(
          {script_url_})) {
    return false;
  }

  // Check that the worker is allowed on the given scope. It's possible a worker
  // was previously allowed and installed, but later content settings changed to
  // disallow this scope. Since this worker might not be used for a specific
  // tab, pass a null callback as WebContents getter.
  // resource_context() can return null in unit tests.
  if ((context_->wrapper()->resource_context() &&
       !GetContentClient()->browser()->AllowServiceWorker(
           scope_, scope_, context_->wrapper()->resource_context(),
           base::Callback<WebContents*(void)>()))) {
    return false;
  }

  return true;
}

}  // namespace content
