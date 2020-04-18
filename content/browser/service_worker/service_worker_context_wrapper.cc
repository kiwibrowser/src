// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_context_wrapper.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/service_worker_process_manager.h"
#include "content/browser/service_worker/service_worker_quota_client.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/browser/storage_partition_impl.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "net/base/url_util.h"
#include "storage/browser/quota/quota_manager_proxy.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

namespace {

typedef std::set<std::string> HeaderNameSet;
base::LazyInstance<HeaderNameSet>::DestructorAtExit g_excluded_header_name_set =
    LAZY_INSTANCE_INITIALIZER;

void WorkerStarted(ServiceWorkerContextWrapper::StatusCallback callback,
                   ServiceWorkerStatusCode status) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(std::move(callback), status));
}

void StartActiveWorkerOnIO(
    ServiceWorkerContextWrapper::StatusCallback callback,
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (status == SERVICE_WORKER_OK) {
    // Pass the reference of |registration| to WorkerStarted callback to prevent
    // it from being deleted while starting the worker. If the refcount of
    // |registration| is 1, it will be deleted after WorkerStarted is called.
    registration->active_version()->StartWorker(
        ServiceWorkerMetrics::EventType::UNKNOWN,
        base::BindOnce(WorkerStarted, std::move(callback)));
    return;
  }
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_NOT_FOUND));
}

void SkipWaitingWorkerOnIO(
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (status != SERVICE_WORKER_OK || !registration->waiting_version())
    return;

  registration->waiting_version()->set_skip_waiting(true);
  registration->ActivateWaitingVersionWhenReady();
}

void DidStartActiveWorker(
    scoped_refptr<ServiceWorkerVersion> version,
    ServiceWorkerContext::StartActiveWorkerCallback info_callback,
    base::OnceClosure error_callback,
    ServiceWorkerStatusCode start_worker_status) {
  if (start_worker_status != SERVICE_WORKER_OK) {
    std::move(error_callback).Run();
    return;
  }
  EmbeddedWorkerInstance* instance = version->embedded_worker();
  std::move(info_callback).Run(instance->process_id(), instance->thread_id());
}

void FoundReadyRegistrationForStartActiveWorker(
    ServiceWorkerContext::StartActiveWorkerCallback info_callback,
    base::OnceClosure failure_callback,
    ServiceWorkerStatusCode service_worker_status,
    scoped_refptr<ServiceWorkerRegistration> service_worker_registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (service_worker_status == SERVICE_WORKER_OK) {
    // Note: There might be a remote possibility that
    // |service_worker_registration|'s active version might change
    // between here and DidStartActiveWorker, so
    // bind |active_version| to RunAfterStartWorker.
    scoped_refptr<ServiceWorkerVersion> active_version =
        service_worker_registration->active_version();
    DCHECK(active_version.get());
    active_version->RunAfterStartWorker(
        ServiceWorkerMetrics::EventType::EXTERNAL_REQUEST,
        base::BindOnce(&DidStartActiveWorker, active_version,
                       std::move(info_callback), std::move(failure_callback)));
  } else {
    std::move(failure_callback).Run();
  }
}

void StatusCodeToBoolCallbackAdapter(
    ServiceWorkerContext::ResultCallback callback,
    ServiceWorkerStatusCode code) {
  std::move(callback).Run(code == ServiceWorkerStatusCode::SERVICE_WORKER_OK);
}

void FinishRegistrationOnIO(ServiceWorkerContext::ResultCallback callback,
                            ServiceWorkerStatusCode status,
                            const std::string& status_message,
                            int64_t registration_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), status == SERVICE_WORKER_OK));
}

void FinishUnregistrationOnIO(ServiceWorkerContext::ResultCallback callback,
                              ServiceWorkerStatusCode status) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), status == SERVICE_WORKER_OK));
}

}  // namespace

// static
void ServiceWorkerContext::AddExcludedHeadersForFetchEvent(
    const std::set<std::string>& header_names) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  g_excluded_header_name_set.Get().insert(header_names.begin(),
                                          header_names.end());
}

// static
bool ServiceWorkerContext::IsExcludedHeaderNameForFetchEvent(
    const std::string& header_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return g_excluded_header_name_set.Get().find(header_name) !=
         g_excluded_header_name_set.Get().end();
}

// static
bool ServiceWorkerContext::ScopeMatches(const GURL& scope, const GURL& url) {
  return ServiceWorkerUtils::ScopeMatches(scope, url);
}

ServiceWorkerContextWrapper::ServiceWorkerContextWrapper(
    BrowserContext* browser_context)
    : core_observer_list_(
          base::MakeRefCounted<ServiceWorkerContextObserverList>()),
      process_manager_(
          std::make_unique<ServiceWorkerProcessManager>(browser_context)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Add this object as an observer of the wrapped |context_core_|. This lets us
  // forward observer methods to observers outside of content.
  core_observer_list_->AddObserver(this);
}

void ServiceWorkerContextWrapper::Init(
    const base::FilePath& user_data_directory,
    storage::QuotaManagerProxy* quota_manager_proxy,
    storage::SpecialStoragePolicy* special_storage_policy,
    ChromeBlobStorageContext* blob_context,
    URLLoaderFactoryGetter* loader_factory_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  is_incognito_ = user_data_directory.empty();
  // The database task runner is BLOCK_SHUTDOWN in order to support
  // ClearSessionOnlyOrigins() (called due to the "clear on browser exit"
  // content setting).
  // TODO(falken): Only block shutdown for that particular task, when someday
  // task runners support mixing task shutdown behaviors.
  scoped_refptr<base::SequencedTaskRunner> database_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  InitInternal(user_data_directory, std::move(database_task_runner),
               quota_manager_proxy, special_storage_policy, blob_context,
               loader_factory_getter);
}

void ServiceWorkerContextWrapper::Shutdown() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  storage_partition_ = nullptr;
  process_manager_->Shutdown();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&ServiceWorkerContextWrapper::ShutdownOnIO, this));
}

void ServiceWorkerContextWrapper::InitializeResourceContext(
    ResourceContext* resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  resource_context_ = resource_context;
}

void ServiceWorkerContextWrapper::DeleteAndStartOver() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    // The context could be null due to system shutdown or restart failure. In
    // either case, we should not have to recover the system, so just return
    // here.
    return;
  }
  context_core_->DeleteAndStartOver(base::BindOnce(
      &ServiceWorkerContextWrapper::DidDeleteAndStartOver, this));
}

StoragePartitionImpl* ServiceWorkerContextWrapper::storage_partition() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return storage_partition_;
}

void ServiceWorkerContextWrapper::set_storage_partition(
    StoragePartitionImpl* storage_partition) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  storage_partition_ = storage_partition;
  process_manager_->set_storage_partition(storage_partition_);
}

ResourceContext* ServiceWorkerContextWrapper::resource_context() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return resource_context_;
}

void ServiceWorkerContextWrapper::OnRegistrationCompleted(
    int64_t registration_id,
    const GURL& pattern) {
  for (auto& observer : observer_list_)
    observer.OnRegistrationCompleted(pattern);
}

void ServiceWorkerContextWrapper::AddObserver(
    ServiceWorkerContextObserver* observer) {
  observer_list_.AddObserver(observer);
}

void ServiceWorkerContextWrapper::RemoveObserver(
    ServiceWorkerContextObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void ServiceWorkerContextWrapper::RegisterServiceWorker(
    const GURL& script_url,
    const blink::mojom::ServiceWorkerRegistrationOptions& options,
    ResultCallback callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::RegisterServiceWorker,
                       this, script_url, options, std::move(callback)));
    return;
  }
  if (!context_core_) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(callback), false));
    return;
  }
  blink::mojom::ServiceWorkerRegistrationOptions options_to_pass(
      net::SimplifyUrlForRequest(options.scope), options.update_via_cache);
  context()->RegisterServiceWorker(
      net::SimplifyUrlForRequest(script_url), options_to_pass,
      base::BindOnce(&FinishRegistrationOnIO, std::move(callback)));
}

void ServiceWorkerContextWrapper::UnregisterServiceWorker(
    const GURL& pattern,
    ResultCallback callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::UnregisterServiceWorker,
                       this, pattern, std::move(callback)));
    return;
  }
  if (!context_core_) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(callback), false));
    return;
  }

  context()->UnregisterServiceWorker(
      net::SimplifyUrlForRequest(pattern),
      base::BindOnce(&FinishUnregistrationOnIO, std::move(callback)));
}

bool ServiceWorkerContextWrapper::StartingExternalRequest(
    int64_t service_worker_version_id,
    const std::string& request_uuid) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ServiceWorkerVersion* version =
      context()->GetLiveVersion(service_worker_version_id);
  if (!version)
    return false;
  return version->StartExternalRequest(request_uuid);
}

bool ServiceWorkerContextWrapper::FinishedExternalRequest(
    int64_t service_worker_version_id,
    const std::string& request_uuid) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ServiceWorkerVersion* version =
      context()->GetLiveVersion(service_worker_version_id);
  if (!version)
    return false;
  return version->FinishExternalRequest(request_uuid);
}

void ServiceWorkerContextWrapper::CountExternalRequestsForTest(
    const GURL& origin,
    CountExternalRequestsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&ServiceWorkerContextWrapper::CountExternalRequests, this,
                     origin, std::move(callback)));
}

void ServiceWorkerContextWrapper::GetAllOriginsInfo(
    GetUsageInfoCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(std::move(callback),
                       std::vector<ServiceWorkerUsageInfo>()));
    return;
  }
  context()->storage()->GetAllRegistrationsInfos(base::BindOnce(
      &ServiceWorkerContextWrapper::DidGetAllRegistrationsForGetAllOrigins,
      this, std::move(callback)));
}

void ServiceWorkerContextWrapper::DeleteForOrigin(const GURL& origin,
                                                  ResultCallback callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::DeleteForOrigin, this,
                       origin, std::move(callback)));
    return;
  }
  if (!context_core_) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::BindOnce(std::move(callback), false));
    return;
  }
  context()->DeleteForOrigin(
      origin.GetOrigin(),
      base::BindOnce(&StatusCodeToBoolCallbackAdapter, std::move(callback)));
}

void ServiceWorkerContextWrapper::CheckHasServiceWorker(
    const GURL& url,
    const GURL& other_url,
    CheckHasServiceWorkerCallback callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::CheckHasServiceWorker,
                       this, url, other_url, std::move(callback)));
    return;
  }
  if (!context_core_) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(callback),
                       ServiceWorkerCapability::NO_SERVICE_WORKER));
    return;
  }
  context()->CheckHasServiceWorker(
      net::SimplifyUrlForRequest(url), net::SimplifyUrlForRequest(other_url),
      base::BindOnce(&ServiceWorkerContextWrapper::DidCheckHasServiceWorker,
                     this, std::move(callback)));
}

void ServiceWorkerContextWrapper::ClearAllServiceWorkersForTest(
    base::OnceClosure callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &ServiceWorkerContextWrapper::ClearAllServiceWorkersForTest, this,
            std::move(callback)));
    return;
  }
  if (!context_core_) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, std::move(callback));
    return;
  }
  context_core_->ClearAllServiceWorkersForTest(std::move(callback));
}

void ServiceWorkerContextWrapper::StartActiveWorkerForPattern(
    const GURL& pattern,
    StartActiveWorkerCallback info_callback,
    base::OnceClosure failure_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FindReadyRegistrationForPattern(
      pattern,
      base::BindOnce(&FoundReadyRegistrationForStartActiveWorker,
                     std::move(info_callback), std::move(failure_callback)));
}

void ServiceWorkerContextWrapper::StartServiceWorkerForNavigationHint(
    const GURL& document_url,
    StartServiceWorkerForNavigationHintCallback callback) {
  TRACE_EVENT1("ServiceWorker", "StartServiceWorkerForNavigationHint",
               "document_url", document_url.spec());
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &ServiceWorkerContextWrapper::StartServiceWorkerForNavigationHintOnIO,
          this, document_url,
          base::BindOnce(&ServiceWorkerContextWrapper::
                             RecordStartServiceWorkerForNavigationHintResult,
                         this, std::move(callback))));
}

void ServiceWorkerContextWrapper::StopAllServiceWorkersForOrigin(
    const GURL& origin) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &ServiceWorkerContextWrapper::StopAllServiceWorkersForOrigin, this,
            origin));
    return;
  }
  if (!context_core_.get()) {
    return;
  }
  std::vector<ServiceWorkerVersionInfo> live_versions = GetAllLiveVersionInfo();
  for (const ServiceWorkerVersionInfo& info : live_versions) {
    ServiceWorkerVersion* version = GetLiveVersion(info.version_id);
    if (version && version->scope().GetOrigin() == origin)
      version->StopWorker(base::DoNothing());
  }
}

void ServiceWorkerContextWrapper::StopAllServiceWorkers(
    base::OnceClosure callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::StopAllServiceWorkersOnIO,
                       this, std::move(callback),
                       base::ThreadTaskRunnerHandle::Get()));
    return;
  }
  StopAllServiceWorkersOnIO(std::move(callback),
                            base::ThreadTaskRunnerHandle::Get());
}

ServiceWorkerRegistration* ServiceWorkerContextWrapper::GetLiveRegistration(
    int64_t registration_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_)
    return nullptr;
  return context_core_->GetLiveRegistration(registration_id);
}

ServiceWorkerVersion* ServiceWorkerContextWrapper::GetLiveVersion(
    int64_t version_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_)
    return nullptr;
  return context_core_->GetLiveVersion(version_id);
}

std::vector<ServiceWorkerRegistrationInfo>
ServiceWorkerContextWrapper::GetAllLiveRegistrationInfo() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_)
    return std::vector<ServiceWorkerRegistrationInfo>();
  return context_core_->GetAllLiveRegistrationInfo();
}

std::vector<ServiceWorkerVersionInfo>
ServiceWorkerContextWrapper::GetAllLiveVersionInfo() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_)
    return std::vector<ServiceWorkerVersionInfo>();
  return context_core_->GetAllLiveVersionInfo();
}

void ServiceWorkerContextWrapper::HasMainFrameProviderHost(
    const GURL& origin,
    BoolCallback callback) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false));
    return;
  }
  context_core_->HasMainFrameProviderHost(origin, std::move(callback));
}

std::unique_ptr<std::vector<std::pair<int, int>>>
ServiceWorkerContextWrapper::GetProviderHostIds(const GURL& origin) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<std::vector<std::pair<int, int>>> provider_host_ids(
      new std::vector<std::pair<int, int>>());

  for (std::unique_ptr<ServiceWorkerContextCore::ProviderHostIterator> it =
           context_core_->GetClientProviderHostIterator(
               origin, false /* include_reserved_clients */);
       !it->IsAtEnd(); it->Advance()) {
    ServiceWorkerProviderHost* provider_host = it->GetProviderHost();
    provider_host_ids->push_back(
        std::make_pair(provider_host->process_id(), provider_host->frame_id()));
  }

  return provider_host_ids;
}

void ServiceWorkerContextWrapper::FindReadyRegistrationForDocument(
    const GURL& document_url,
    FindRegistrationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    // FindRegistrationForDocument() can run the callback synchronously.
    std::move(callback).Run(SERVICE_WORKER_ERROR_ABORT, nullptr);
    return;
  }
  context_core_->storage()->FindRegistrationForDocument(
      net::SimplifyUrlForRequest(document_url),
      base::BindOnce(
          &ServiceWorkerContextWrapper::DidFindRegistrationForFindReady, this,
          std::move(callback)));
}

void ServiceWorkerContextWrapper::FindReadyRegistrationForPattern(
    const GURL& scope,
    FindRegistrationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  SERVICE_WORKER_ERROR_ABORT, nullptr));
    return;
  }
  context_core_->storage()->FindRegistrationForPattern(
      net::SimplifyUrlForRequest(scope),
      base::BindOnce(
          &ServiceWorkerContextWrapper::DidFindRegistrationForFindReady, this,
          std::move(callback)));
}

void ServiceWorkerContextWrapper::FindReadyRegistrationForId(
    int64_t registration_id,
    const GURL& origin,
    FindRegistrationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    // FindRegistrationForId() can run the callback synchronously.
    std::move(callback).Run(SERVICE_WORKER_ERROR_ABORT, nullptr);
    return;
  }
  context_core_->storage()->FindRegistrationForId(
      registration_id, origin.GetOrigin(),
      base::BindOnce(
          &ServiceWorkerContextWrapper::DidFindRegistrationForFindReady, this,
          std::move(callback)));
}

void ServiceWorkerContextWrapper::FindReadyRegistrationForIdOnly(
    int64_t registration_id,
    FindRegistrationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    // FindRegistrationForIdOnly() can run the callback synchronously.
    std::move(callback).Run(SERVICE_WORKER_ERROR_ABORT, nullptr);
    return;
  }
  context_core_->storage()->FindRegistrationForIdOnly(
      registration_id,
      base::BindOnce(
          &ServiceWorkerContextWrapper::DidFindRegistrationForFindReady, this,
          std::move(callback)));
}

void ServiceWorkerContextWrapper::GetAllRegistrations(
    GetRegistrationsInfosCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT,
                       std::vector<ServiceWorkerRegistrationInfo>()));
    return;
  }
  context_core_->storage()->GetAllRegistrationsInfos(std::move(callback));
}

void ServiceWorkerContextWrapper::GetRegistrationUserData(
    int64_t registration_id,
    const std::vector<std::string>& keys,
    GetUserDataCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), std::vector<std::string>(),
                       SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->GetUserData(registration_id, keys,
                                        std::move(callback));
}

void ServiceWorkerContextWrapper::GetRegistrationUserDataByKeyPrefix(
    int64_t registration_id,
    const std::string& key_prefix,
    GetUserDataCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), std::vector<std::string>(),
                       SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->GetUserDataByKeyPrefix(registration_id, key_prefix,
                                                   std::move(callback));
}

void ServiceWorkerContextWrapper::GetRegistrationUserKeysAndDataByKeyPrefix(
    int64_t registration_id,
    const std::string& key_prefix,
    GetUserKeysAndDataCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  base::flat_map<std::string, std::string>(),
                                  SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->GetUserKeysAndDataByKeyPrefix(
      registration_id, key_prefix, std::move(callback));
}

void ServiceWorkerContextWrapper::StoreRegistrationUserData(
    int64_t registration_id,
    const GURL& origin,
    const std::vector<std::pair<std::string, std::string>>& key_value_pairs,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->StoreUserData(registration_id, origin.GetOrigin(),
                                          key_value_pairs, std::move(callback));
}

void ServiceWorkerContextWrapper::ClearRegistrationUserData(
    int64_t registration_id,
    const std::vector<std::string>& keys,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->ClearUserData(registration_id, keys,
                                          std::move(callback));
}

void ServiceWorkerContextWrapper::ClearRegistrationUserDataByKeyPrefixes(
    int64_t registration_id,
    const std::vector<std::string>& key_prefixes,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->ClearUserDataByKeyPrefixes(
      registration_id, key_prefixes, std::move(callback));
}

void ServiceWorkerContextWrapper::GetUserDataForAllRegistrations(
    const std::string& key,
    GetUserDataForAllRegistrationsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       std::vector<std::pair<int64_t, std::string>>(),
                       SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->GetUserDataForAllRegistrations(key,
                                                           std::move(callback));
}

void ServiceWorkerContextWrapper::GetUserDataForAllRegistrationsByKeyPrefix(
    const std::string& key_prefix,
    GetUserDataForAllRegistrationsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       std::vector<std::pair<int64_t, std::string>>(),
                       SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->GetUserDataForAllRegistrationsByKeyPrefix(
      key_prefix, std::move(callback));
}

void ServiceWorkerContextWrapper::StartServiceWorker(const GURL& pattern,
                                                     StatusCallback callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::StartServiceWorker, this,
                       pattern, std::move(callback)));
    return;
  }
  if (!context_core_) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_ABORT));
    return;
  }
  context_core_->storage()->FindRegistrationForPattern(
      net::SimplifyUrlForRequest(pattern),
      base::BindOnce(&StartActiveWorkerOnIO, std::move(callback)));
}

void ServiceWorkerContextWrapper::SkipWaitingWorker(const GURL& pattern) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::SkipWaitingWorker, this,
                       pattern));
    return;
  }
  if (!context_core_)
    return;
  context_core_->storage()->FindRegistrationForPattern(
      net::SimplifyUrlForRequest(pattern),
      base::BindOnce(&SkipWaitingWorkerOnIO));
}

void ServiceWorkerContextWrapper::UpdateRegistration(const GURL& pattern) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::UpdateRegistration, this,
                       pattern));
    return;
  }
  if (!context_core_)
    return;
  context_core_->storage()->FindRegistrationForPattern(
      net::SimplifyUrlForRequest(pattern),
      base::BindOnce(&ServiceWorkerContextWrapper::DidFindRegistrationForUpdate,
                     this));
}

void ServiceWorkerContextWrapper::SetForceUpdateOnPageLoad(
    bool force_update_on_page_load) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::SetForceUpdateOnPageLoad,
                       this, force_update_on_page_load));
    return;
  }
  if (!context_core_)
    return;
  context_core_->set_force_update_on_page_load(force_update_on_page_load);
}

void ServiceWorkerContextWrapper::AddObserver(
    ServiceWorkerContextCoreObserver* observer) {
  core_observer_list_->AddObserver(observer);
}

void ServiceWorkerContextWrapper::RemoveObserver(
    ServiceWorkerContextCoreObserver* observer) {
  core_observer_list_->RemoveObserver(observer);
}

base::WeakPtr<ServiceWorkerProviderHost>
ServiceWorkerContextWrapper::PreCreateHostForSharedWorker(
    int process_id,
    mojom::ServiceWorkerProviderInfoForSharedWorkerPtr* out_provider_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  return ServiceWorkerProviderHost::PreCreateForSharedWorker(
      context()->AsWeakPtr(), process_id, out_provider_info);
}

ServiceWorkerContextWrapper::~ServiceWorkerContextWrapper() {
  // Explicitly remove this object as an observer to avoid use-after-frees in
  // tests where this object is not guaranteed to outlive the
  // ServiceWorkerContextCore it wraps.
  core_observer_list_->RemoveObserver(this);
  DCHECK(!resource_context_);
}

void ServiceWorkerContextWrapper::InitInternal(
    const base::FilePath& user_data_directory,
    scoped_refptr<base::SequencedTaskRunner> database_task_runner,
    storage::QuotaManagerProxy* quota_manager_proxy,
    storage::SpecialStoragePolicy* special_storage_policy,
    ChromeBlobStorageContext* blob_context,
    URLLoaderFactoryGetter* loader_factory_getter) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerContextWrapper::InitInternal, this,
                       user_data_directory, std::move(database_task_runner),
                       base::RetainedRef(quota_manager_proxy),
                       base::RetainedRef(special_storage_policy),
                       base::RetainedRef(blob_context),
                       base::RetainedRef(loader_factory_getter)));
    return;
  }
  DCHECK(!context_core_);
  if (quota_manager_proxy) {
    quota_manager_proxy->RegisterClient(new ServiceWorkerQuotaClient(this));
  }

  context_core_ = std::make_unique<ServiceWorkerContextCore>(
      user_data_directory, std::move(database_task_runner), quota_manager_proxy,
      special_storage_policy, loader_factory_getter, core_observer_list_.get(),
      this);
}

void ServiceWorkerContextWrapper::ShutdownOnIO() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  resource_context_ = nullptr;
  context_core_.reset();
}

void ServiceWorkerContextWrapper::DidFindRegistrationForFindReady(
    FindRegistrationCallback callback,
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (status != SERVICE_WORKER_OK) {
    std::move(callback).Run(status, nullptr);
    return;
  }

  // Attempt to activate the waiting version because the registration retrieved
  // from the disk might have only the waiting version.
  if (registration->waiting_version())
    registration->ActivateWaitingVersionWhenReady();

  scoped_refptr<ServiceWorkerVersion> active_version =
      registration->active_version();
  if (!active_version) {
    std::move(callback).Run(SERVICE_WORKER_ERROR_NOT_FOUND, nullptr);
    return;
  }

  if (active_version->status() == ServiceWorkerVersion::ACTIVATING) {
    // Wait until the version is activated.
    active_version->RegisterStatusChangeCallback(base::BindOnce(
        &ServiceWorkerContextWrapper::OnStatusChangedForFindReadyRegistration,
        this, std::move(callback), std::move(registration)));
    return;
  }

  DCHECK_EQ(ServiceWorkerVersion::ACTIVATED, active_version->status());
  std::move(callback).Run(SERVICE_WORKER_OK, std::move(registration));
}

void ServiceWorkerContextWrapper::OnStatusChangedForFindReadyRegistration(
    FindRegistrationCallback callback,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  scoped_refptr<ServiceWorkerVersion> active_version =
      registration->active_version();
  if (!active_version ||
      active_version->status() != ServiceWorkerVersion::ACTIVATED) {
    std::move(callback).Run(SERVICE_WORKER_ERROR_NOT_FOUND, nullptr);
    return;
  }
  std::move(callback).Run(SERVICE_WORKER_OK, registration);
}

void ServiceWorkerContextWrapper::DidDeleteAndStartOver(
    ServiceWorkerStatusCode status) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (status != SERVICE_WORKER_OK) {
    context_core_.reset();
    return;
  }
  context_core_.reset(new ServiceWorkerContextCore(context_core_.get(), this));
  DVLOG(1) << "Restarted ServiceWorkerContextCore successfully.";
  context_core_->OnStorageWiped();
}

void ServiceWorkerContextWrapper::DidGetAllRegistrationsForGetAllOrigins(
    GetUsageInfoCallback callback,
    ServiceWorkerStatusCode status,
    const std::vector<ServiceWorkerRegistrationInfo>& registrations) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::vector<ServiceWorkerUsageInfo> usage_infos;

  std::map<GURL, ServiceWorkerUsageInfo> origins;
  for (const auto& registration_info : registrations) {
    GURL origin = registration_info.pattern.GetOrigin();

    ServiceWorkerUsageInfo& usage_info = origins[origin];
    if (usage_info.origin.is_empty())
      usage_info.origin = origin;
    usage_info.scopes.push_back(registration_info.pattern);
    usage_info.total_size_bytes += registration_info.stored_version_size_bytes;
  }

  for (const auto& origin_info_pair : origins) {
    usage_infos.push_back(origin_info_pair.second);
  }
  std::move(callback).Run(usage_infos);
}

void ServiceWorkerContextWrapper::DidCheckHasServiceWorker(
    CheckHasServiceWorkerCallback callback,
    ServiceWorkerCapability capability) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(std::move(callback), capability));
}

void ServiceWorkerContextWrapper::DidFindRegistrationForUpdate(
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (status != SERVICE_WORKER_OK)
    return;
  if (!context_core_)
    return;
  DCHECK(registration);
  // TODO(jungkees): |force_bypass_cache| is set to true because the call stack
  // is initiated by an update button on DevTools that expects the cache is
  // bypassed. However, in order to provide options for callers to choose the
  // cache bypass mode, plumb |force_bypass_cache| through to
  // UpdateRegistration().
  context_core_->UpdateServiceWorker(registration.get(),
                                     true /* force_bypass_cache */);
}

void ServiceWorkerContextWrapper::CountExternalRequests(
    const GURL& origin,
    CountExternalRequestsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::vector<ServiceWorkerVersionInfo> live_version_info =
      GetAllLiveVersionInfo();
  size_t pending_external_request_count = 0;
  for (const ServiceWorkerVersionInfo& info : live_version_info) {
    ServiceWorkerVersion* version = GetLiveVersion(info.version_id);
    if (version && version->scope().GetOrigin() == origin) {
      pending_external_request_count =
          version->GetExternalRequestCountForTest();
      break;
    }
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), pending_external_request_count));
}

void ServiceWorkerContextWrapper::StartServiceWorkerForNavigationHintOnIO(
    const GURL& document_url,
    StartServiceWorkerForNavigationHintCallback callback) {
  TRACE_EVENT1("ServiceWorker", "StartServiceWorkerForNavigationHintOnIO",
               "document_url", document_url.spec());
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_) {
    std::move(callback).Run(StartServiceWorkerForNavigationHintResult::FAILED);
    return;
  }
  context_core_->storage()->FindRegistrationForDocument(
      net::SimplifyUrlForRequest(document_url),
      base::BindOnce(
          &ServiceWorkerContextWrapper::DidFindRegistrationForNavigationHint,
          this, std::move(callback)));
}

void ServiceWorkerContextWrapper::DidFindRegistrationForNavigationHint(
    StartServiceWorkerForNavigationHintCallback callback,
    ServiceWorkerStatusCode status,
    scoped_refptr<ServiceWorkerRegistration> registration) {
  TRACE_EVENT1("ServiceWorker", "DidFindRegistrationForNavigationHint",
               "status", ServiceWorkerStatusToString(status));
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!registration) {
    DCHECK_NE(status, SERVICE_WORKER_OK);
    std::move(callback).Run(StartServiceWorkerForNavigationHintResult::
                                NO_SERVICE_WORKER_REGISTRATION);
    return;
  }
  if (!registration->active_version()) {
    std::move(callback).Run(StartServiceWorkerForNavigationHintResult::
                                NO_ACTIVE_SERVICE_WORKER_VERSION);
    return;
  }
  if (registration->active_version()->fetch_handler_existence() ==
      ServiceWorkerVersion::FetchHandlerExistence::DOES_NOT_EXIST) {
    std::move(callback).Run(
        StartServiceWorkerForNavigationHintResult::NO_FETCH_HANDLER);
    return;
  }
  if (registration->active_version()->running_status() ==
      EmbeddedWorkerStatus::RUNNING) {
    std::move(callback).Run(
        StartServiceWorkerForNavigationHintResult::ALREADY_RUNNING);
    return;
  }

  registration->active_version()->StartWorker(
      ServiceWorkerMetrics::EventType::NAVIGATION_HINT,
      base::BindOnce(
          &ServiceWorkerContextWrapper::DidStartServiceWorkerForNavigationHint,
          this, registration->pattern(), std::move(callback)));
}

void ServiceWorkerContextWrapper::DidStartServiceWorkerForNavigationHint(
    const GURL& pattern,
    StartServiceWorkerForNavigationHintCallback callback,
    ServiceWorkerStatusCode code) {
  TRACE_EVENT2("ServiceWorker", "DidStartServiceWorkerForNavigationHint", "url",
               pattern.spec(), "code", ServiceWorkerStatusToString(code));
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(
      code == SERVICE_WORKER_OK
          ? StartServiceWorkerForNavigationHintResult::STARTED
          : StartServiceWorkerForNavigationHintResult::FAILED);
}

void ServiceWorkerContextWrapper::
    RecordStartServiceWorkerForNavigationHintResult(
        StartServiceWorkerForNavigationHintCallback callback,
        StartServiceWorkerForNavigationHintResult result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ServiceWorkerMetrics::RecordStartServiceWorkerForNavigationHintResult(result);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(std::move(callback), result));
}

void ServiceWorkerContextWrapper::StopAllServiceWorkersOnIO(
    base::OnceClosure callback,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_for_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context_core_.get()) {
    task_runner_for_callback->PostTask(FROM_HERE, std::move(callback));
    return;
  }
  std::vector<ServiceWorkerVersionInfo> live_versions = GetAllLiveVersionInfo();
  base::RepeatingClosure barrier = base::BarrierClosure(
      live_versions.size(),
      base::BindOnce(
          base::IgnoreResult(&base::SingleThreadTaskRunner::PostTask),
          std::move(task_runner_for_callback), FROM_HERE, std::move(callback)));
  for (const ServiceWorkerVersionInfo& info : live_versions) {
    ServiceWorkerVersion* version = GetLiveVersion(info.version_id);
    DCHECK(version);
    version->StopWorker(base::BindOnce(barrier));
  }
}

ServiceWorkerContextCore* ServiceWorkerContextWrapper::context() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return context_core_.get();
}

}  // namespace content
