// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_register_job.h"

#include <stdint.h>

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_job_coordinator.h"
#include "content/browser/service_worker/service_worker_metrics.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "content/browser/service_worker/service_worker_type_converters.h"
#include "content/browser/service_worker/service_worker_write_to_cache_job.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "net/base/net_errors.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"

namespace content {

typedef ServiceWorkerRegisterJobBase::RegistrationJobType RegistrationJobType;

ServiceWorkerRegisterJob::ServiceWorkerRegisterJob(
    base::WeakPtr<ServiceWorkerContextCore> context,
    const GURL& script_url,
    const blink::mojom::ServiceWorkerRegistrationOptions& options)
    : context_(context),
      job_type_(REGISTRATION_JOB),
      pattern_(options.scope),
      script_url_(script_url),
      update_via_cache_(options.update_via_cache),
      phase_(INITIAL),
      doom_installing_worker_(false),
      is_promise_resolved_(false),
      should_uninstall_on_failure_(false),
      force_bypass_cache_(false),
      skip_script_comparison_(false),
      promise_resolved_status_(SERVICE_WORKER_OK),
      observer_(this),
      weak_factory_(this) {}

ServiceWorkerRegisterJob::ServiceWorkerRegisterJob(
    base::WeakPtr<ServiceWorkerContextCore> context,
    ServiceWorkerRegistration* registration,
    bool force_bypass_cache,
    bool skip_script_comparison)
    : context_(context),
      job_type_(UPDATE_JOB),
      pattern_(registration->pattern()),
      update_via_cache_(registration->update_via_cache()),
      phase_(INITIAL),
      doom_installing_worker_(false),
      is_promise_resolved_(false),
      should_uninstall_on_failure_(false),
      force_bypass_cache_(force_bypass_cache),
      skip_script_comparison_(skip_script_comparison),
      promise_resolved_status_(SERVICE_WORKER_OK),
      observer_(this),
      weak_factory_(this) {
  internal_.registration = registration;
}

ServiceWorkerRegisterJob::~ServiceWorkerRegisterJob() {
  DCHECK(!context_ ||
         phase_ == INITIAL || phase_ == COMPLETE || phase_ == ABORT)
      << "Jobs should only be interrupted during shutdown.";
}

void ServiceWorkerRegisterJob::AddCallback(RegistrationCallback callback) {
  if (!is_promise_resolved_) {
    callbacks_.emplace_back(std::move(callback));
    return;
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), promise_resolved_status_,
                     promise_resolved_status_message_,
                     base::RetainedRef(promise_resolved_registration_)));
}

void ServiceWorkerRegisterJob::Start() {
  BrowserThread::PostAfterStartupTask(
      FROM_HERE, base::ThreadTaskRunnerHandle::Get(),
      base::BindOnce(&ServiceWorkerRegisterJob::StartImpl,
                     weak_factory_.GetWeakPtr()));
}

void ServiceWorkerRegisterJob::StartImpl() {
  SetPhase(START);
  ServiceWorkerStorage::FindRegistrationCallback next_step;
  if (job_type_ == REGISTRATION_JOB) {
    next_step =
        base::BindOnce(&ServiceWorkerRegisterJob::ContinueWithRegistration,
                       weak_factory_.GetWeakPtr());
  } else {
    next_step = base::BindOnce(&ServiceWorkerRegisterJob::ContinueWithUpdate,
                               weak_factory_.GetWeakPtr());
  }

  scoped_refptr<ServiceWorkerRegistration> registration =
      context_->storage()->GetUninstallingRegistration(pattern_);
  if (registration.get())
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(next_step), SERVICE_WORKER_OK, registration));
  else
    context_->storage()->FindRegistrationForPattern(pattern_,
                                                    std::move(next_step));
}

void ServiceWorkerRegisterJob::Abort() {
  SetPhase(ABORT);
  CompleteInternal(SERVICE_WORKER_ERROR_ABORT, std::string());
  // Don't have to call FinishJob() because the caller takes care of removing
  // the jobs from the queue.
}

bool ServiceWorkerRegisterJob::Equals(ServiceWorkerRegisterJobBase* job) const {
  if (job->GetType() != job_type_)
    return false;
  ServiceWorkerRegisterJob* register_job =
      static_cast<ServiceWorkerRegisterJob*>(job);
  if (job_type_ == UPDATE_JOB)
    return register_job->pattern_ == pattern_;
  DCHECK_EQ(REGISTRATION_JOB, job_type_);
  return register_job->pattern_ == pattern_ &&
         register_job->script_url_ == script_url_;
}

RegistrationJobType ServiceWorkerRegisterJob::GetType() const {
  return job_type_;
}

void ServiceWorkerRegisterJob::DoomInstallingWorker() {
  doom_installing_worker_ = true;
  if (phase_ == INSTALL)
    Complete(SERVICE_WORKER_ERROR_INSTALL_WORKER_FAILED, std::string());
}

ServiceWorkerRegisterJob::Internal::Internal() {}

ServiceWorkerRegisterJob::Internal::~Internal() {}

void ServiceWorkerRegisterJob::set_registration(
    scoped_refptr<ServiceWorkerRegistration> registration) {
  DCHECK(phase_ == START || phase_ == REGISTER) << phase_;
  DCHECK(!internal_.registration.get());
  internal_.registration = std::move(registration);
}

ServiceWorkerRegistration* ServiceWorkerRegisterJob::registration() {
  DCHECK(phase_ >= REGISTER || job_type_ == UPDATE_JOB) << phase_;
  return internal_.registration.get();
}

void ServiceWorkerRegisterJob::set_new_version(
    ServiceWorkerVersion* version) {
  DCHECK(phase_ == UPDATE) << phase_;
  DCHECK(!internal_.new_version.get());
  internal_.new_version = version;
}

ServiceWorkerVersion* ServiceWorkerRegisterJob::new_version() {
  DCHECK(phase_ >= UPDATE) << phase_;
  return internal_.new_version.get();
}

void ServiceWorkerRegisterJob::SetPhase(Phase phase) {
  switch (phase) {
    case INITIAL:
      NOTREACHED();
      break;
    case START:
      DCHECK(phase_ == INITIAL) << phase_;
      break;
    case REGISTER:
      DCHECK(phase_ == START) << phase_;
      break;
    case UPDATE:
      DCHECK(phase_ == START || phase_ == REGISTER) << phase_;
      break;
    case INSTALL:
      DCHECK(phase_ == UPDATE) << phase_;
      break;
    case STORE:
      DCHECK(phase_ == INSTALL) << phase_;
      break;
    case COMPLETE:
      DCHECK(phase_ != INITIAL && phase_ != COMPLETE) << phase_;
      break;
    case ABORT:
      break;
  }
  phase_ = phase;
}

// This function corresponds to the steps in [[Register]] following
// "Let registration be the result of running the [[GetRegistration]] algorithm.
// Throughout this file, comments in quotes are excerpts from the spec.
void ServiceWorkerRegisterJob::ContinueWithRegistration(
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> existing_registration) {
  DCHECK_EQ(REGISTRATION_JOB, job_type_);
  if (status != SERVICE_WORKER_ERROR_NOT_FOUND && status != SERVICE_WORKER_OK) {
    Complete(status);
    return;
  }

  if (!existing_registration.get() || existing_registration->is_uninstalled()) {
    RegisterAndContinue();
    return;
  }

  DCHECK(existing_registration->GetNewestVersion());
  // "If scriptURL is equal to registration.[[ScriptURL]] and
  // "update_via_cache is equal to registration.[[update_via_cache]], then:"
  if (existing_registration->GetNewestVersion()->script_url() == script_url_ &&
      existing_registration->update_via_cache() == update_via_cache_) {
    // "Set registration.[[Uninstalling]] to false."
    existing_registration->AbortPendingClear(base::BindOnce(
        &ServiceWorkerRegisterJob::ContinueWithRegistrationForSameScriptUrl,
        weak_factory_.GetWeakPtr(), existing_registration));
    return;
  }

  if (existing_registration->is_uninstalling()) {
    existing_registration->AbortPendingClear(base::BindOnce(
        &ServiceWorkerRegisterJob::ContinueWithUninstallingRegistration,
        weak_factory_.GetWeakPtr(), existing_registration));
    return;
  }

  // "Invoke Set Registration algorithm with job’s scope url and
  // job’s update via cache mode."
  existing_registration->SetUpdateViaCache(update_via_cache_);
  set_registration(existing_registration);
  // "Return the result of running the [[Update]] algorithm, or its equivalent,
  // passing registration as the argument."
  UpdateAndContinue();
}

void ServiceWorkerRegisterJob::ContinueWithUpdate(
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> existing_registration) {
  DCHECK_EQ(UPDATE_JOB, job_type_);
  if (status != SERVICE_WORKER_OK) {
    Complete(status);
    return;
  }

  if (existing_registration.get() != registration()) {
    Complete(SERVICE_WORKER_ERROR_NOT_FOUND);
    return;
  }

  // A previous job may have unregistered this registration.
  if (registration()->is_uninstalling() ||
      !registration()->GetNewestVersion()) {
    Complete(SERVICE_WORKER_ERROR_NOT_FOUND);
    return;
  }

  DCHECK(script_url_.is_empty());
  script_url_ = registration()->GetNewestVersion()->script_url();

  // TODO(michaeln): If the last update check was less than 24 hours
  // ago, depending on the freshness of the cached worker script we
  // may be able to complete the update job right here.

  UpdateAndContinue();
}

// Creates a new ServiceWorkerRegistration.
void ServiceWorkerRegisterJob::RegisterAndContinue() {
  SetPhase(REGISTER);

  int64_t registration_id = context_->storage()->NewRegistrationId();
  if (registration_id == blink::mojom::kInvalidServiceWorkerRegistrationId) {
    Complete(SERVICE_WORKER_ERROR_ABORT);
    return;
  }

  blink::mojom::ServiceWorkerRegistrationOptions options(pattern_,
                                                         update_via_cache_);
  set_registration(
      new ServiceWorkerRegistration(options, registration_id, context_));
  AddRegistrationToMatchingProviderHosts(registration());
  UpdateAndContinue();
}

void ServiceWorkerRegisterJob::ContinueWithUninstallingRegistration(
    scoped_refptr<ServiceWorkerRegistration> existing_registration,
    ServiceWorkerStatusCode status) {
  if (status != SERVICE_WORKER_OK) {
    Complete(status);
    return;
  }
  should_uninstall_on_failure_ = true;
  set_registration(existing_registration);
  UpdateAndContinue();
}

void ServiceWorkerRegisterJob::ContinueWithRegistrationForSameScriptUrl(
    scoped_refptr<ServiceWorkerRegistration> existing_registration,
    ServiceWorkerStatusCode status) {
  if (status != SERVICE_WORKER_OK) {
    Complete(status);
    return;
  }
  set_registration(existing_registration);

  // "If newestWorker is not null, scriptURL is equal to newestWorker.scriptURL,
  // and job’s update via cache mode's value equals registration’s
  // update via cache mode then:
  // Return a promise resolved with registration."
  // We resolve only if there's an active version. If there's not,
  // then there is either no version or only a waiting version from
  // the last browser session; it makes sense to proceed with registration in
  // either case.
  DCHECK(!existing_registration->installing_version());
  if (existing_registration->active_version()) {
    ResolvePromise(status, std::string(), existing_registration.get());
    Complete(SERVICE_WORKER_OK);
    return;
  }

  // "Return the result of running the [[Update]] algorithm, or its equivalent,
  // passing registration as the argument."
  UpdateAndContinue();
}

// This function corresponds to the spec's [[Update]] algorithm.
void ServiceWorkerRegisterJob::UpdateAndContinue() {
  SetPhase(UPDATE);
  context_->storage()->NotifyInstallingRegistration(registration());

  int64_t version_id = context_->storage()->NewVersionId();
  if (version_id == blink::mojom::kInvalidServiceWorkerVersionId) {
    Complete(SERVICE_WORKER_ERROR_ABORT);
    return;
  }

  // "Let worker be a new ServiceWorker object..." and start
  // the worker.
  set_new_version(new ServiceWorkerVersion(registration(), script_url_,
                                           version_id, context_));
  new_version()->set_force_bypass_cache_for_scripts(force_bypass_cache_);
  if (registration()->has_installed_version() && !skip_script_comparison_) {
    new_version()->set_pause_after_download(true);
    observer_.Add(new_version()->embedded_worker());
  } else {
    new_version()->set_pause_after_download(false);
  }
  new_version()->StartWorker(
      ServiceWorkerMetrics::EventType::INSTALL,
      base::BindOnce(&ServiceWorkerRegisterJob::OnStartWorkerFinished,
                     weak_factory_.GetWeakPtr()));
}

void ServiceWorkerRegisterJob::OnStartWorkerFinished(
    ServiceWorkerStatusCode status) {
  BumpLastUpdateCheckTimeIfNeeded();

  if (status == SERVICE_WORKER_OK) {
    InstallAndContinue();
    return;
  }

  // "If serviceWorker fails to start up..." then reject the promise with an
  // error and abort.
  if (status == SERVICE_WORKER_ERROR_TIMEOUT) {
    Complete(status, "Timed out while trying to start the Service Worker.");
    return;
  }

  const net::URLRequestStatus& main_script_status =
      new_version()->script_cache_map()->main_script_status();
  std::string message;
  if (main_script_status.status() != net::URLRequestStatus::SUCCESS) {
    message = new_version()->script_cache_map()->main_script_status_message();
    if (message.empty())
      message = kServiceWorkerFetchScriptError;
  }
  Complete(status, message);
}

// This function corresponds to the spec's [[Install]] algorithm.
void ServiceWorkerRegisterJob::InstallAndContinue() {
  SetPhase(INSTALL);

  // "Set registration.installingWorker to worker."
  DCHECK(!registration()->installing_version());
  registration()->SetInstallingVersion(new_version());

  // "Run the Update State algorithm passing registration's installing worker
  // and installing as the arguments."
  new_version()->SetStatus(ServiceWorkerVersion::INSTALLING);

  // "Resolve registrationPromise with registration."
  ResolvePromise(SERVICE_WORKER_OK, std::string(), registration());

  // "Fire a simple event named updatefound..."
  registration()->NotifyUpdateFound();

  // "Fire an event named install..."
  new_version()->RunAfterStartWorker(
      ServiceWorkerMetrics::EventType::INSTALL,
      base::BindOnce(&ServiceWorkerRegisterJob::DispatchInstallEvent,
                     weak_factory_.GetWeakPtr()));

  // A subsequent registration job may terminate our installing worker. It can
  // only do so after we've started the worker and dispatched the install
  // event, as those are atomic substeps in the [[Install]] algorithm.
  if (doom_installing_worker_)
    Complete(SERVICE_WORKER_ERROR_INSTALL_WORKER_FAILED);
}

void ServiceWorkerRegisterJob::DispatchInstallEvent(
    ServiceWorkerStatusCode start_worker_status) {
  if (start_worker_status != SERVICE_WORKER_OK) {
    OnInstallFailed(start_worker_status);
    return;
  }

  DCHECK_EQ(ServiceWorkerVersion::INSTALLING, new_version()->status())
      << new_version()->status();
  DCHECK_EQ(EmbeddedWorkerStatus::RUNNING, new_version()->running_status())
      << "Worker stopped too soon after it was started.";
  int request_id = new_version()->StartRequest(
      ServiceWorkerMetrics::EventType::INSTALL,
      base::BindOnce(&ServiceWorkerRegisterJob::OnInstallFailed,
                     weak_factory_.GetWeakPtr()));

  new_version()->event_dispatcher()->DispatchInstallEvent(
      base::BindOnce(&ServiceWorkerRegisterJob::OnInstallFinished,
                     weak_factory_.GetWeakPtr(), request_id));
}

void ServiceWorkerRegisterJob::OnInstallFinished(
    int request_id,
    blink::mojom::ServiceWorkerEventStatus event_status,
    bool has_fetch_handler,
    base::Time dispatch_event_time) {
  bool succeeded =
      event_status == blink::mojom::ServiceWorkerEventStatus::COMPLETED;
  new_version()->FinishRequest(request_id, succeeded, dispatch_event_time);

  if (!succeeded) {
    OnInstallFailed(mojo::ConvertTo<ServiceWorkerStatusCode>(event_status));
    return;
  }

  ServiceWorkerMetrics::RecordInstallEventStatus(SERVICE_WORKER_OK);

  SetPhase(STORE);
  DCHECK(!registration()->last_update_check().is_null());
  new_version()->set_fetch_handler_existence(
      has_fetch_handler
          ? ServiceWorkerVersion::FetchHandlerExistence::EXISTS
          : ServiceWorkerVersion::FetchHandlerExistence::DOES_NOT_EXIST);
  context_->storage()->StoreRegistration(
      registration(), new_version(),
      base::BindOnce(&ServiceWorkerRegisterJob::OnStoreRegistrationComplete,
                     weak_factory_.GetWeakPtr()));
}

void ServiceWorkerRegisterJob::OnInstallFailed(ServiceWorkerStatusCode status) {
  ServiceWorkerMetrics::RecordInstallEventStatus(status);
  DCHECK_NE(status, SERVICE_WORKER_OK)
      << "OnInstallFailed should not handle SERVICE_WORKER_OK";
  Complete(status, std::string("ServiceWorker failed to install: ") +
                       ServiceWorkerStatusToString(status));
}

void ServiceWorkerRegisterJob::OnStoreRegistrationComplete(
    ServiceWorkerStatusCode status) {
  if (status != SERVICE_WORKER_OK) {
    Complete(status);
    return;
  }

  // "9. If registration.waitingWorker is not null, then:..."
  if (registration()->waiting_version()) {
    // 1. Set redundantWorker to registration’s waiting worker.
    // 2. Terminate redundantWorker.
    registration()->waiting_version()->StopWorker(base::DoNothing());
    // TODO(falken): Move this further down. The spec says to set status to
    // 'redundant' after promoting the new version to .waiting attribute and
    // 'installed' status.
    registration()->waiting_version()->SetStatus(
        ServiceWorkerVersion::REDUNDANT);
  }

  // "10. Set registration.waitingWorker to registration.installingWorker."
  // "11. Set registration.installingWorker to null."
  registration()->SetWaitingVersion(new_version());

  // "12. Run the [[UpdateState]] algorithm passing registration.waitingWorker
  // and "installed" as the arguments."
  new_version()->SetStatus(ServiceWorkerVersion::INSTALLED);

  // "If registration's waiting worker's skip waiting flag is set:" then
  // activate the worker immediately otherwise "wait until no service worker
  // client is using registration as their service worker registration."
  registration()->ActivateWaitingVersionWhenReady();

  Complete(SERVICE_WORKER_OK);
}

void ServiceWorkerRegisterJob::Complete(ServiceWorkerStatusCode status) {
  Complete(status, std::string());
}

void ServiceWorkerRegisterJob::Complete(ServiceWorkerStatusCode status,
                                        const std::string& status_message) {
  CompleteInternal(status, status_message);
  context_->job_coordinator()->FinishJob(pattern_, this);
}

void ServiceWorkerRegisterJob::CompleteInternal(
    ServiceWorkerStatusCode status,
    const std::string& status_message) {
  SetPhase(COMPLETE);

  if (new_version()) {
    new_version()->set_pause_after_download(false);
    observer_.RemoveAll();
  }

  if (status != SERVICE_WORKER_OK) {
    if (registration()) {
      if (should_uninstall_on_failure_)
        registration()->ClearWhenReady();
      if (new_version()) {
        if (status == SERVICE_WORKER_ERROR_EXISTS)
          new_version()->SetStartWorkerStatusCode(SERVICE_WORKER_ERROR_EXISTS);
        else
          new_version()->ReportError(status, status_message);
        registration()->UnsetVersion(new_version());
        new_version()->Doom();
      }
      if (!registration()->waiting_version() &&
          !registration()->active_version()) {
        registration()->NotifyRegistrationFailed();
        context_->storage()->DeleteRegistration(
            registration()->id(), registration()->pattern().GetOrigin(),
            base::DoNothing());
      }
    }
    if (!is_promise_resolved_)
      ResolvePromise(status, status_message, nullptr);
  }
  DCHECK(callbacks_.empty());
  if (registration()) {
    context_->storage()->NotifyDoneInstallingRegistration(
        registration(), new_version(), status);
    if (registration()->has_installed_version())
      registration()->set_is_uninstalled(false);
  }
}

void ServiceWorkerRegisterJob::ResolvePromise(
    ServiceWorkerStatusCode status,
    const std::string& status_message,
    ServiceWorkerRegistration* registration) {
  DCHECK(!is_promise_resolved_);

  is_promise_resolved_ = true;
  promise_resolved_status_ = status;
  promise_resolved_status_message_ = status_message,
  promise_resolved_registration_ = registration;
  for (RegistrationCallback& callback : callbacks_)
    std::move(callback).Run(status, status_message, registration);
  callbacks_.clear();
}

void ServiceWorkerRegisterJob::AddRegistrationToMatchingProviderHosts(
    ServiceWorkerRegistration* registration) {
  DCHECK(registration);
  for (std::unique_ptr<ServiceWorkerContextCore::ProviderHostIterator> it =
           context_->GetClientProviderHostIterator(
               registration->pattern().GetOrigin(),
               true /* include_reserved_clients */);
       !it->IsAtEnd(); it->Advance()) {
    ServiceWorkerProviderHost* host = it->GetProviderHost();
    if (!ServiceWorkerUtils::ScopeMatches(registration->pattern(),
                                          host->document_url())) {
      continue;
    }
    host->AddMatchingRegistration(registration);
  }
}

void ServiceWorkerRegisterJob::OnScriptLoaded() {
  DCHECK(new_version()->pause_after_download());
  new_version()->set_pause_after_download(false);
  net::URLRequestStatus status =
      new_version()->script_cache_map()->main_script_status();
  if (!status.is_success()) {
    // OnScriptLoaded signifies a successful network load, which translates into
    // a script cache error only in the byte-for-byte identical case.
    DCHECK_EQ(status.error(),
              ServiceWorkerWriteToCacheJob::kIdenticalScriptError);

    BumpLastUpdateCheckTimeIfNeeded();
    ResolvePromise(SERVICE_WORKER_OK, std::string(), registration());
    Complete(SERVICE_WORKER_ERROR_EXISTS,
             "The updated worker is identical to the incumbent.");
    return;
  }

  new_version()->embedded_worker()->ResumeAfterDownload();
}

void ServiceWorkerRegisterJob::BumpLastUpdateCheckTimeIfNeeded() {
  // Bump the last update check time only when the register/update job fetched
  // the version having bypassed the network cache. We assume that the
  // BYPASS_CACHE flag evicts an existing cache entry, so even if the install
  // ultimately failed for whatever reason, we know the version in the HTTP
  // cache is not stale, so it's OK to bump the update check time.
  if (new_version()->embedded_worker()->network_accessed_for_script() ||
      new_version()->force_bypass_cache_for_scripts() ||
      registration()->last_update_check().is_null()) {
    registration()->set_last_update_check(base::Time::Now());

    if (registration()->has_installed_version())
      context_->storage()->UpdateLastUpdateCheckTime(registration());
  }
}

}  // namespace content
