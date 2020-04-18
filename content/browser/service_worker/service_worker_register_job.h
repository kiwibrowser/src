// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REGISTER_JOB_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REGISTER_JOB_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "content/browser/service_worker/embedded_worker_instance.h"
#include "content/browser/service_worker/service_worker_register_job_base.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "url/gurl.h"

namespace content {

// Handles the initial registration of a Service Worker and the
// subsequent update of existing registrations.
//
// The control flow includes most or all of the following,
// depending on what is already registered:
//  - creating a ServiceWorkerRegistration instance if there isn't
//    already something registered
//  - creating a ServiceWorkerVersion for the new version.
//  - starting a worker for the ServiceWorkerVersion
//  - firing the 'install' event at the ServiceWorkerVersion
//  - firing the 'activate' event at the ServiceWorkerVersion
//  - waiting for older ServiceWorkerVersions to deactivate
//  - designating the new version to be the 'active' version
//  - updating storage
class ServiceWorkerRegisterJob : public ServiceWorkerRegisterJobBase,
                                 public EmbeddedWorkerInstance::Listener {
 public:
  typedef base::OnceCallback<void(ServiceWorkerStatusCode status,
                                  const std::string& status_message,
                                  ServiceWorkerRegistration* registration)>
      RegistrationCallback;

  // For registration jobs.
  CONTENT_EXPORT ServiceWorkerRegisterJob(
      base::WeakPtr<ServiceWorkerContextCore> context,
      const GURL& script_url,
      const blink::mojom::ServiceWorkerRegistrationOptions& options);

  // For update jobs.
  CONTENT_EXPORT ServiceWorkerRegisterJob(
      base::WeakPtr<ServiceWorkerContextCore> context,
      ServiceWorkerRegistration* registration,
      bool force_bypass_cache,
      bool skip_script_comparison);
  ~ServiceWorkerRegisterJob() override;

  // Registers a callback to be called when the promise would resolve (whether
  // successfully or not). Multiple callbacks may be registered.
  void AddCallback(RegistrationCallback callback);

  // ServiceWorkerRegisterJobBase implementation:
  void Start() override;
  void Abort() override;
  bool Equals(ServiceWorkerRegisterJobBase* job) const override;
  RegistrationJobType GetType() const override;

  void DoomInstallingWorker();

 private:
  enum Phase {
    INITIAL,
    START,
    REGISTER,
    UPDATE,
    INSTALL,
    STORE,
    COMPLETE,
    ABORT,
  };

  // Holds internal state of ServiceWorkerRegistrationJob, to compel use of the
  // getter/setter functions.
  struct Internal {
    Internal();
    ~Internal();
    scoped_refptr<ServiceWorkerRegistration> registration;

    // Holds the version created by this job. It can be the 'installing',
    // 'waiting', or 'active' version depending on the phase.
    scoped_refptr<ServiceWorkerVersion> new_version;
  };

  void set_registration(scoped_refptr<ServiceWorkerRegistration> registration);
  ServiceWorkerRegistration* registration();
  void set_new_version(ServiceWorkerVersion* version);
  ServiceWorkerVersion* new_version();

  void SetPhase(Phase phase);

  void StartImpl();
  void ContinueWithRegistration(
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void ContinueWithUpdate(
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void RegisterAndContinue();
  void ContinueWithUninstallingRegistration(
      scoped_refptr<ServiceWorkerRegistration> existing_registration,
      ServiceWorkerStatusCode status);
  void ContinueWithRegistrationForSameScriptUrl(
      scoped_refptr<ServiceWorkerRegistration> existing_registration,
      ServiceWorkerStatusCode status);
  void UpdateAndContinue();
  void OnStartWorkerFinished(ServiceWorkerStatusCode status);
  void OnStoreRegistrationComplete(ServiceWorkerStatusCode status);
  void InstallAndContinue();
  void DispatchInstallEvent(ServiceWorkerStatusCode start_worker_status);
  void OnInstallFinished(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus event_status,
      bool has_fetch_handler,
      base::Time dispatch_event_time);
  void OnInstallFailed(ServiceWorkerStatusCode status);
  void Complete(ServiceWorkerStatusCode status);
  void Complete(ServiceWorkerStatusCode status,
                const std::string& status_message);
  void CompleteInternal(ServiceWorkerStatusCode status,
                        const std::string& status_message);
  void ResolvePromise(ServiceWorkerStatusCode status,
                      const std::string& status_message,
                      ServiceWorkerRegistration* registration);

  void AddRegistrationToMatchingProviderHosts(
      ServiceWorkerRegistration* registration);

  // EmbeddedWorkerInstance::Listener implementation:
  void OnScriptLoaded() override;

  void BumpLastUpdateCheckTimeIfNeeded();

  // The ServiceWorkerContextCore object should always outlive this.
  base::WeakPtr<ServiceWorkerContextCore> context_;

  RegistrationJobType job_type_;
  const GURL pattern_;
  GURL script_url_;
  const blink::mojom::ServiceWorkerUpdateViaCache update_via_cache_;
  std::vector<RegistrationCallback> callbacks_;
  Phase phase_;
  Internal internal_;
  bool doom_installing_worker_;
  bool is_promise_resolved_;
  bool should_uninstall_on_failure_;
  bool force_bypass_cache_;
  bool skip_script_comparison_;
  ServiceWorkerStatusCode promise_resolved_status_;
  std::string promise_resolved_status_message_;
  scoped_refptr<ServiceWorkerRegistration> promise_resolved_registration_;
  ScopedObserver<EmbeddedWorkerInstance, EmbeddedWorkerInstance::Listener>
      observer_;
  base::WeakPtrFactory<ServiceWorkerRegisterJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerRegisterJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REGISTER_JOB_H_
