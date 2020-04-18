// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_registration.h"

#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_info.h"
#include "content/browser/service_worker/service_worker_metrics.h"
#include "content/browser/service_worker/service_worker_register_job.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

namespace {

// If an outgoing active worker has no controllees or the waiting worker called
// skipWaiting(), it is given |kMaxLameDuckTime| time to finish its requests
// before it is removed. If the waiting worker called skipWaiting() more than
// this time ago, or the outgoing worker has had no controllees for a continuous
// period of time exceeding this time, the outgoing worker will be removed even
// if it has ongoing requests.
constexpr base::TimeDelta kMaxLameDuckTime = base::TimeDelta::FromMinutes(5);

ServiceWorkerVersionInfo GetVersionInfo(ServiceWorkerVersion* version) {
  if (!version)
    return ServiceWorkerVersionInfo();
  return version->GetInfo();
}

}  // namespace

ServiceWorkerRegistration::ServiceWorkerRegistration(
    const blink::mojom::ServiceWorkerRegistrationOptions& options,
    int64_t registration_id,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : pattern_(options.scope),
      update_via_cache_(options.update_via_cache),
      registration_id_(registration_id),
      is_deleted_(false),
      is_uninstalling_(false),
      is_uninstalled_(false),
      should_activate_when_ready_(false),
      resources_total_size_bytes_(0),
      context_(context),
      task_runner_(base::ThreadTaskRunnerHandle::Get()) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerRegistrationId, registration_id);
  DCHECK(context_);
  context_->AddLiveRegistration(this);
}

ServiceWorkerRegistration::~ServiceWorkerRegistration() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!listeners_.might_have_observers());
  if (context_)
    context_->RemoveLiveRegistration(registration_id_);
  if (active_version())
    active_version()->RemoveListener(this);
}

ServiceWorkerVersion* ServiceWorkerRegistration::GetNewestVersion() const {
  if (installing_version())
    return installing_version();
  if (waiting_version())
    return waiting_version();
  return active_version();
}

void ServiceWorkerRegistration::AddListener(Listener* listener) {
  listeners_.AddObserver(listener);
}

void ServiceWorkerRegistration::RemoveListener(Listener* listener) {
  listeners_.RemoveObserver(listener);
}

void ServiceWorkerRegistration::NotifyRegistrationFailed() {
  for (auto& observer : listeners_)
    observer.OnRegistrationFailed(this);
  NotifyRegistrationFinished();
}

void ServiceWorkerRegistration::NotifyUpdateFound() {
  for (auto& observer : listeners_)
    observer.OnUpdateFound(this);
}

void ServiceWorkerRegistration::NotifyVersionAttributesChanged(
    ChangedVersionAttributesMask mask) {
  for (auto& observer : listeners_)
    observer.OnVersionAttributesChanged(this, mask, GetInfo());
  if (mask.active_changed() || mask.waiting_changed())
    NotifyRegistrationFinished();
}

ServiceWorkerRegistrationInfo ServiceWorkerRegistration::GetInfo() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return ServiceWorkerRegistrationInfo(
      pattern(), update_via_cache(), registration_id_,
      is_deleted_ ? ServiceWorkerRegistrationInfo::IS_DELETED
                  : ServiceWorkerRegistrationInfo::IS_NOT_DELETED,
      GetVersionInfo(active_version_.get()),
      GetVersionInfo(waiting_version_.get()),
      GetVersionInfo(installing_version_.get()), resources_total_size_bytes_,
      navigation_preload_state_.enabled,
      navigation_preload_state_.header.length());
}

void ServiceWorkerRegistration::SetActiveVersion(
    const scoped_refptr<ServiceWorkerVersion>& version) {
  if (active_version_ == version)
    return;

  should_activate_when_ready_ = false;

  ChangedVersionAttributesMask mask;
  if (version)
    UnsetVersionInternal(version.get(), &mask);
  if (active_version_)
    active_version_->RemoveListener(this);
  active_version_ = version;
  if (active_version_) {
    active_version_->AddListener(this);
    active_version_->SetNavigationPreloadState(navigation_preload_state_);
  }
  mask.add(ChangedVersionAttributesMask::ACTIVE_VERSION);

  NotifyVersionAttributesChanged(mask);
}

void ServiceWorkerRegistration::SetWaitingVersion(
    const scoped_refptr<ServiceWorkerVersion>& version) {
  if (waiting_version_ == version)
    return;

  should_activate_when_ready_ = false;

  ChangedVersionAttributesMask mask;
  if (version)
    UnsetVersionInternal(version.get(), &mask);
  waiting_version_ = version;
  mask.add(ChangedVersionAttributesMask::WAITING_VERSION);

  NotifyVersionAttributesChanged(mask);
}

void ServiceWorkerRegistration::SetInstallingVersion(
    const scoped_refptr<ServiceWorkerVersion>& version) {
  if (installing_version_ == version)
    return;

  ChangedVersionAttributesMask mask;
  if (version)
    UnsetVersionInternal(version.get(), &mask);
  installing_version_ = version;
  mask.add(ChangedVersionAttributesMask::INSTALLING_VERSION);

  NotifyVersionAttributesChanged(mask);
}

void ServiceWorkerRegistration::UnsetVersion(ServiceWorkerVersion* version) {
  if (!version)
    return;
  ChangedVersionAttributesMask mask;
  UnsetVersionInternal(version, &mask);
  if (mask.changed())
    NotifyVersionAttributesChanged(mask);
}

void ServiceWorkerRegistration::UnsetVersionInternal(
    ServiceWorkerVersion* version,
    ChangedVersionAttributesMask* mask) {
  DCHECK(version);
  if (installing_version_.get() == version) {
    installing_version_ = nullptr;
    mask->add(ChangedVersionAttributesMask::INSTALLING_VERSION);
  } else if (waiting_version_.get() == version) {
    waiting_version_ = nullptr;
    should_activate_when_ready_ = false;
    mask->add(ChangedVersionAttributesMask::WAITING_VERSION);
  } else if (active_version_.get() == version) {
    active_version_->RemoveListener(this);
    active_version_ = nullptr;
    mask->add(ChangedVersionAttributesMask::ACTIVE_VERSION);
  }
}

void ServiceWorkerRegistration::SetUpdateViaCache(
    blink::mojom::ServiceWorkerUpdateViaCache update_via_cache) {
  if (update_via_cache_ == update_via_cache)
    return;
  update_via_cache_ = update_via_cache;
  for (auto& observer : listeners_)
    observer.OnUpdateViaCacheChanged(this);
}

void ServiceWorkerRegistration::ActivateWaitingVersionWhenReady() {
  DCHECK(waiting_version());
  should_activate_when_ready_ = true;
  if (IsReadyToActivate()) {
    ActivateWaitingVersion(false /* delay */);
    return;
  }

  if (IsLameDuckActiveVersion()) {
    if (ServiceWorkerUtils::IsServicificationEnabled() &&
        active_version()->running_status() == EmbeddedWorkerStatus::RUNNING) {
      // If the waiting worker is ready and the active worker needs to be
      // swapped out, ask the active worker to trigger idle timer as soon as
      // possible.
      active_version()->event_dispatcher()->SetIdleTimerDelayToZero();
    }
    StartLameDuckTimer();
  }
}

void ServiceWorkerRegistration::ClaimClients() {
  DCHECK(context_);
  DCHECK(active_version());

  for (std::unique_ptr<ServiceWorkerContextCore::ProviderHostIterator> it =
           context_->GetClientProviderHostIterator(
               pattern_.GetOrigin(), false /* include_reserved_clients */);
       !it->IsAtEnd(); it->Advance()) {
    ServiceWorkerProviderHost* host = it->GetProviderHost();
    if (host->controller() == active_version())
      continue;
    if (!host->IsContextSecureForServiceWorker())
      continue;
    if (host->MatchRegistration() == this)
      host->ClaimedByRegistration(this);
  }
}

void ServiceWorkerRegistration::ClearWhenReady() {
  DCHECK(context_);
  if (is_uninstalling_)
    return;
  is_uninstalling_ = true;

  context_->storage()->NotifyUninstallingRegistration(this);
  context_->storage()->DeleteRegistration(
      id(), pattern().GetOrigin(),
      AdaptCallbackForRepeating(
          base::BindOnce(&ServiceWorkerRegistration::OnDeleteFinished, this)));

  if (!active_version() || !active_version()->HasControllee())
    Clear();
}

void ServiceWorkerRegistration::AbortPendingClear(StatusCallback callback) {
  DCHECK(context_);
  if (!is_uninstalling()) {
    std::move(callback).Run(SERVICE_WORKER_OK);
    return;
  }
  is_uninstalling_ = false;
  context_->storage()->NotifyDoneUninstallingRegistration(this);

  scoped_refptr<ServiceWorkerVersion> most_recent_version =
      waiting_version() ? waiting_version() : active_version();
  DCHECK(most_recent_version.get());
  context_->storage()->NotifyInstallingRegistration(this);
  context_->storage()->StoreRegistration(
      this, most_recent_version.get(),
      base::BindOnce(&ServiceWorkerRegistration::OnRestoreFinished, this,
                     std::move(callback), most_recent_version));
}

void ServiceWorkerRegistration::OnNoControllees(ServiceWorkerVersion* version) {
  if (!context_)
    return;
  DCHECK_EQ(active_version(), version);
  if (is_uninstalling_) {
    Clear();
    return;
  }

  if (IsReadyToActivate()) {
    ActivateWaitingVersion(true /* delay */);
    return;
  }

  if (IsLameDuckActiveVersion()) {
    if (ServiceWorkerUtils::IsServicificationEnabled() &&
        should_activate_when_ready_ &&
        active_version()->running_status() == EmbeddedWorkerStatus::RUNNING) {
      // If the waiting worker is ready and the active worker needs to be
      // swapped out, ask the active worker to trigger idle timer as soon as
      // possible.
      active_version()->event_dispatcher()->SetIdleTimerDelayToZero();
    }
    StartLameDuckTimer();
  }
}

void ServiceWorkerRegistration::OnNoWork(ServiceWorkerVersion* version) {
  if (!context_)
    return;
  DCHECK_EQ(active_version(), version);
  if (IsReadyToActivate())
    ActivateWaitingVersion(false /* delay */);
}

bool ServiceWorkerRegistration::IsReadyToActivate() const {
  if (!should_activate_when_ready_)
    return false;

  DCHECK(waiting_version());
  const ServiceWorkerVersion* waiting = waiting_version();
  const ServiceWorkerVersion* active = active_version();
  if (!active) {
    return true;
  }
  if (IsLameDuckActiveVersion()) {
    return !active->HasWorkInBrowser() ||
           waiting->TimeSinceSkipWaiting() > kMaxLameDuckTime ||
           active->TimeSinceNoControllees() > kMaxLameDuckTime;
  }
  return false;
}

bool ServiceWorkerRegistration::IsLameDuckActiveVersion() const {
  if (!waiting_version() || !active_version())
    return false;
  return waiting_version()->skip_waiting() ||
         !active_version()->HasControllee();
}

void ServiceWorkerRegistration::StartLameDuckTimer() {
  DCHECK(IsLameDuckActiveVersion());
  if (lame_duck_timer_.IsRunning())
    return;

  lame_duck_timer_.Start(
      FROM_HERE, kMaxLameDuckTime,
      base::Bind(&ServiceWorkerRegistration::RemoveLameDuckIfNeeded,
                 Unretained(this) /* OK because |this| owns the timer */));
}

void ServiceWorkerRegistration::RemoveLameDuckIfNeeded() {
  if (!should_activate_when_ready_) {
    lame_duck_timer_.Stop();
    return;
  }

  if (IsReadyToActivate()) {
    ActivateWaitingVersion(false /* delay */);
    return;
  }

  if (!IsLameDuckActiveVersion()) {
    lame_duck_timer_.Stop();
  }
}

void ServiceWorkerRegistration::ActivateWaitingVersion(bool delay) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(context_);
  DCHECK(IsReadyToActivate());
  should_activate_when_ready_ = false;
  lame_duck_timer_.Stop();

  scoped_refptr<ServiceWorkerVersion> activating_version = waiting_version();
  scoped_refptr<ServiceWorkerVersion> exiting_version = active_version();

  if (activating_version->is_redundant())
    return;  // Activation is no longer relevant.

  // "5. If exitingWorker is not null,
  if (exiting_version.get()) {
    // TODO(falken): Update the quoted spec comments once
    // https://github.com/slightlyoff/ServiceWorker/issues/916 is codified in
    // the spec.
    // "1. Wait for exitingWorker to finish handling any in-progress requests."
    // This is already handled by IsReadyToActivate().
    // "2. Terminate exitingWorker."
    exiting_version->StopWorker(base::DoNothing());
    // "3. Run the [[UpdateState]] algorithm passing exitingWorker and
    // "redundant" as the arguments."
    exiting_version->SetStatus(ServiceWorkerVersion::REDUNDANT);
  }

  // "6. Set serviceWorkerRegistration.activeWorker to activatingWorker."
  // "7. Set serviceWorkerRegistration.waitingWorker to null."
  SetActiveVersion(activating_version);

  // "8. Run the [[UpdateState]] algorithm passing registration.activeWorker and
  // "activating" as arguments."
  activating_version->SetStatus(ServiceWorkerVersion::ACTIVATING);
  // "9. Fire a simple event named controllerchange..."
  if (activating_version->skip_waiting()) {
    for (auto& observer : listeners_)
      observer.OnSkippedWaiting(this);
  }

  // "10. Queue a task to fire an event named activate..."
  // The browser could be shutting down. To avoid spurious start worker
  // failures, wait a bit before continuing.
  if (delay) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&ServiceWorkerRegistration::ContinueActivation, this,
                       activating_version),
        base::TimeDelta::FromSeconds(1));
  } else {
    ContinueActivation(std::move(activating_version));
  }
}

void ServiceWorkerRegistration::ContinueActivation(
    scoped_refptr<ServiceWorkerVersion> activating_version) {
  if (!context_)
    return;
  if (active_version() != activating_version.get())
    return;
  DCHECK_EQ(ServiceWorkerVersion::ACTIVATING, activating_version->status());
  activating_version->RunAfterStartWorker(
      ServiceWorkerMetrics::EventType::ACTIVATE,
      base::BindOnce(&ServiceWorkerRegistration::DispatchActivateEvent, this,
                     activating_version));
}

void ServiceWorkerRegistration::DeleteVersion(
    const scoped_refptr<ServiceWorkerVersion>& version) {
  DCHECK_EQ(id(), version->registration_id());

  // Protect the registration since version->Doom() can stop |version|, which
  // destroys start worker callbacks, which might be the only things holding a
  // reference to |this|.
  scoped_refptr<ServiceWorkerRegistration> protect(this);

  UnsetVersion(version.get());
  version->Doom();

  if (!active_version() && !waiting_version()) {
    // Delete the records from the db.
    context_->storage()->DeleteRegistration(
        id(), pattern().GetOrigin(),
        base::BindOnce(&ServiceWorkerRegistration::OnDeleteFinished, protect));
    // But not from memory if there is a version in the pipeline.
    // TODO(falken): Fix this logic. There could be a running register job for
    // this registration that hasn't set installing_version() yet.
    if (installing_version()) {
      is_deleted_ = false;
    } else {
      is_uninstalled_ = true;
      NotifyRegistrationFailed();
    }
  }
}

void ServiceWorkerRegistration::NotifyRegistrationFinished() {
  std::vector<base::Closure> callbacks;
  callbacks.swap(registration_finished_callbacks_);
  for (const auto& callback : callbacks)
    callback.Run();
}

void ServiceWorkerRegistration::SetTaskRunnerForTest(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  task_runner_ = task_runner;
}

void ServiceWorkerRegistration::EnableNavigationPreload(bool enable) {
  navigation_preload_state_.enabled = enable;
  if (active_version_)
    active_version_->SetNavigationPreloadState(navigation_preload_state_);
}

void ServiceWorkerRegistration::SetNavigationPreloadHeader(
    const std::string& header) {
  navigation_preload_state_.header = header;
  if (active_version_)
    active_version_->SetNavigationPreloadState(navigation_preload_state_);
}

void ServiceWorkerRegistration::RegisterRegistrationFinishedCallback(
    const base::Closure& callback) {
  // This should only be called if the registration is in progress.
  DCHECK(!active_version() && !waiting_version() && !is_uninstalled() &&
         !is_uninstalling());
  registration_finished_callbacks_.push_back(callback);
}

void ServiceWorkerRegistration::DispatchActivateEvent(
    scoped_refptr<ServiceWorkerVersion> activating_version,
    ServiceWorkerStatusCode start_worker_status) {
  if (start_worker_status != SERVICE_WORKER_OK) {
    OnActivateEventFinished(activating_version, start_worker_status);
    return;
  }
  if (activating_version != active_version()) {
    OnActivateEventFinished(activating_version, SERVICE_WORKER_ERROR_FAILED);
    return;
  }

  DCHECK_EQ(ServiceWorkerVersion::ACTIVATING, activating_version->status());
  DCHECK_EQ(EmbeddedWorkerStatus::RUNNING, activating_version->running_status())
      << "Worker stopped too soon after it was started.";
  int request_id = activating_version->StartRequest(
      ServiceWorkerMetrics::EventType::ACTIVATE,
      base::BindOnce(&ServiceWorkerRegistration::OnActivateEventFinished, this,
                     activating_version));
  activating_version->event_dispatcher()->DispatchActivateEvent(
      activating_version->CreateSimpleEventCallback(request_id));
}

void ServiceWorkerRegistration::OnActivateEventFinished(
    scoped_refptr<ServiceWorkerVersion> activating_version,
    ServiceWorkerStatusCode status) {
  // Activate is prone to failing due to shutdown, because it's triggered when
  // tabs close.
  bool is_shutdown =
      !context_ || context_->wrapper()->process_manager()->IsShutdown();
  ServiceWorkerMetrics::RecordActivateEventStatus(status, is_shutdown);

  if (!context_ || activating_version != active_version() ||
      activating_version->status() != ServiceWorkerVersion::ACTIVATING) {
    return;
  }

  // Normally, the worker is committed to become activated once we get here, per
  // spec. E.g., if the script rejected waitUntil or had an unhandled exception,
  // it should still be activated. However, if the failure occurred during
  // shutdown, ignore it to give the worker another chance the next time the
  // browser starts up.
  if (is_shutdown && status != SERVICE_WORKER_OK)
    return;

  // "Run the Update State algorithm passing registration's active worker and
  // 'activated' as the arguments."
  activating_version->SetStatus(ServiceWorkerVersion::ACTIVATED);
  context_->storage()->UpdateToActiveState(this, base::DoNothing());
}

void ServiceWorkerRegistration::OnDeleteFinished(
    ServiceWorkerStatusCode status) {
  for (auto& listener : listeners_)
    listener.OnRegistrationDeleted(this);
}

void ServiceWorkerRegistration::Clear() {
  is_uninstalling_ = false;
  is_uninstalled_ = true;
  should_activate_when_ready_ = false;
  if (context_)
    context_->storage()->NotifyDoneUninstallingRegistration(this);

  std::vector<scoped_refptr<ServiceWorkerVersion>> versions_to_doom;
  ChangedVersionAttributesMask mask;
  if (installing_version_.get()) {
    versions_to_doom.push_back(installing_version_);
    installing_version_ = nullptr;
    mask.add(ChangedVersionAttributesMask::INSTALLING_VERSION);
  }
  if (waiting_version_.get()) {
    versions_to_doom.push_back(waiting_version_);
    waiting_version_ = nullptr;
    mask.add(ChangedVersionAttributesMask::WAITING_VERSION);
  }
  if (active_version_.get()) {
    versions_to_doom.push_back(active_version_);
    active_version_->RemoveListener(this);
    active_version_ = nullptr;
    mask.add(ChangedVersionAttributesMask::ACTIVE_VERSION);
  }

  if (mask.changed()) {
    NotifyVersionAttributesChanged(mask);

    // Doom only after notifying attributes changed, because the spec requires
    // the attributes to be cleared by the time the statechange event is
    // dispatched.
    for (const auto& version : versions_to_doom)
      version->Doom();
  }

  for (auto& observer : listeners_)
    observer.OnRegistrationFinishedUninstalling(this);
}

void ServiceWorkerRegistration::OnRestoreFinished(
    StatusCallback callback,
    scoped_refptr<ServiceWorkerVersion> version,
    ServiceWorkerStatusCode status) {
  if (!context_) {
    std::move(callback).Run(SERVICE_WORKER_ERROR_ABORT);
    return;
  }
  context_->storage()->NotifyDoneInstallingRegistration(
      this, version.get(), status);
  std::move(callback).Run(status);
}

}  // namespace content
