// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/service_worker_test_helpers.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "content/browser/service_worker/service_worker_context_core_observer.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace content {

namespace {

class StoppedObserver : public base::RefCountedThreadSafe<StoppedObserver> {
 public:
  StoppedObserver(ServiceWorkerContextWrapper* context,
                  int64_t service_worker_version_id,
                  base::OnceClosure completion_callback_ui)
      : inner_observer_(context,
                        service_worker_version_id,
                        // Adds a ref to StoppedObserver to keep |this| around
                        // until the worker is stopped.
                        base::BindOnce(&StoppedObserver::OnStopped, this)),
        completion_callback_ui_(std::move(completion_callback_ui)) {}

 private:
  friend class base::RefCountedThreadSafe<StoppedObserver>;
  ~StoppedObserver() {}
  class Observer : public ServiceWorkerContextCoreObserver {
   public:
    Observer(ServiceWorkerContextWrapper* context,
             int64_t service_worker_version_id,
             base::OnceClosure stopped_callback)
        : context_(context),
          version_id_(service_worker_version_id),
          stopped_callback_(std::move(stopped_callback)) {
      context_->AddObserver(this);
    }

    // ServiceWorkerContextCoreObserver:
    void OnRunningStateChanged(int64_t version_id,
                               EmbeddedWorkerStatus status) override {
      DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
      if (version_id != version_id_ || status != EmbeddedWorkerStatus::STOPPED)
        return;
      std::move(stopped_callback_).Run();
    }
    ~Observer() override { context_->RemoveObserver(this); }

   private:
    ServiceWorkerContextWrapper* const context_;
    int64_t version_id_;
    base::OnceClosure stopped_callback_;
  };

  void OnStopped() {
    if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(&StoppedObserver::OnStopped, this));
      return;
    }
    std::move(completion_callback_ui_).Run();
  }

  Observer inner_observer_;
  base::OnceClosure completion_callback_ui_;

  DISALLOW_COPY_AND_ASSIGN(StoppedObserver);
};

void FoundReadyRegistration(
    ServiceWorkerContextWrapper* context_wrapper,
    base::OnceClosure completion_callback,
    ServiceWorkerStatusCode service_worker_status,
    scoped_refptr<ServiceWorkerRegistration> service_worker_registration) {
  DCHECK_EQ(SERVICE_WORKER_OK, service_worker_status);
  int64_t version_id =
      service_worker_registration->active_version()->version_id();
  scoped_refptr<StoppedObserver> observer(new StoppedObserver(
      context_wrapper, version_id, std::move(completion_callback)));
  service_worker_registration->active_version()->embedded_worker()->Stop();
}

}  // namespace

void StopServiceWorkerForPattern(ServiceWorkerContext* context,
                                 const GURL& pattern,
                                 base::OnceClosure completion_callback_ui) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&StopServiceWorkerForPattern, context, pattern,
                       std::move(completion_callback_ui)));
    return;
  }
  auto* context_wrapper = static_cast<ServiceWorkerContextWrapper*>(context);
  context_wrapper->FindReadyRegistrationForPattern(
      pattern, base::BindOnce(&FoundReadyRegistration,
                              base::RetainedRef(context_wrapper),
                              std::move(completion_callback_ui)));
}

}  // namespace content
