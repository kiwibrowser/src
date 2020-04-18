// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/embedded_worker_instance.h"

#include <stdint.h>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "content/browser/service_worker/embedded_worker_registry.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/embedded_worker.mojom.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

namespace {

void SaveStatusAndCall(ServiceWorkerStatusCode* out,
                       base::OnceClosure callback,
                       ServiceWorkerStatusCode status) {
  *out = status;
  std::move(callback).Run();
}

}  // namespace

class ProviderHostEndpoints : public mojom::ServiceWorkerContainerHost {
 public:
  ProviderHostEndpoints() : binding_(this) {}

  ~ProviderHostEndpoints() override {}

  mojom::ServiceWorkerProviderInfoForStartWorkerPtr CreateProviderInfoPtr() {
    DCHECK(!binding_.is_bound());
    DCHECK(!client_.is_bound());
    // Just keep the endpoints.
    mojom::ServiceWorkerProviderInfoForStartWorkerPtr provider_info =
        mojom::ServiceWorkerProviderInfoForStartWorker::New();
    binding_.Bind(mojo::MakeRequest(&provider_info->host_ptr_info));
    provider_info->client_request = mojo::MakeRequest(&client_);
    mojo::MakeRequest(&provider_info->interface_provider);
    return provider_info;
  }

 private:
  // Implements mojom::ServiceWorkerContainerHost.
  void Register(const GURL& script_url,
                blink::mojom::ServiceWorkerRegistrationOptionsPtr options,
                RegisterCallback callback) override {
    NOTIMPLEMENTED();
  }
  void GetRegistration(const GURL& client_url,
                       GetRegistrationCallback callback) override {
    NOTIMPLEMENTED();
  }
  void GetRegistrations(GetRegistrationsCallback callback) override {
    NOTIMPLEMENTED();
  }
  void GetRegistrationForReady(
      GetRegistrationForReadyCallback callback) override {
    NOTIMPLEMENTED();
  }
  void EnsureControllerServiceWorker(
      mojom::ControllerServiceWorkerRequest request,
      mojom::ControllerServiceWorkerPurpose purpose) override {
    NOTIMPLEMENTED();
  }
  void CloneForWorker(
      mojom::ServiceWorkerContainerHostRequest request) override {
    NOTIMPLEMENTED();
  }
  void Ping(PingCallback callback) override { NOTIMPLEMENTED(); }

  mojom::ServiceWorkerContainerAssociatedPtr client_;
  mojo::AssociatedBinding<mojom::ServiceWorkerContainerHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(ProviderHostEndpoints);
};

class EmbeddedWorkerInstanceTest : public testing::Test,
                                   public EmbeddedWorkerInstance::Listener {
 protected:
  EmbeddedWorkerInstanceTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  enum EventType {
    PROCESS_ALLOCATED,
    START_WORKER_MESSAGE_SENT,
    STARTED,
    STOPPED,
    DETACHED,
  };

  struct EventLog {
    EventType type;
    EmbeddedWorkerStatus status;
  };

  void RecordEvent(
      EventType type,
      EmbeddedWorkerStatus status = EmbeddedWorkerStatus::STOPPED) {
    EventLog log = {type, status};
    events_.push_back(log);
  }

  void OnProcessAllocated() override { RecordEvent(PROCESS_ALLOCATED); }
  void OnStartWorkerMessageSent() override {
    RecordEvent(START_WORKER_MESSAGE_SENT);
  }
  void OnStarted() override { RecordEvent(STARTED); }
  void OnStopped(EmbeddedWorkerStatus old_status) override {
    RecordEvent(STOPPED, old_status);
  }
  void OnDetached(EmbeddedWorkerStatus old_status) override {
    RecordEvent(DETACHED, old_status);
  }

  void SetUp() override {
    helper_.reset(new EmbeddedWorkerTestHelper(base::FilePath()));
  }

  void TearDown() override { helper_.reset(); }

  using RegistrationAndVersionPair =
      std::pair<scoped_refptr<ServiceWorkerRegistration>,
                scoped_refptr<ServiceWorkerVersion>>;

  RegistrationAndVersionPair PrepareRegistrationAndVersion(
      const GURL& scope,
      const GURL& script_url) {
    base::RunLoop loop;
    if (!context()->storage()->LazyInitializeForTest(loop.QuitClosure())) {
      loop.Run();
    }
    RegistrationAndVersionPair pair;
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope;
    pair.first = base::MakeRefCounted<ServiceWorkerRegistration>(
        options, context()->storage()->NewRegistrationId(),
        context()->AsWeakPtr());
    pair.second = base::MakeRefCounted<ServiceWorkerVersion>(
        pair.first.get(), script_url, context()->storage()->NewVersionId(),
        context()->AsWeakPtr());
    return pair;
  }

  ServiceWorkerStatusCode StartWorker(EmbeddedWorkerInstance* worker,
                                      int id, const GURL& pattern,
                                      const GURL& url) {
    ServiceWorkerStatusCode status;
    base::RunLoop run_loop;
    mojom::EmbeddedWorkerStartParamsPtr params =
        CreateStartParams(id, pattern, url);
    worker->Start(
        std::move(params), CreateProviderInfoGetter(),
        base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
    run_loop.Run();
    return status;
  }

  mojom::EmbeddedWorkerStartParamsPtr
  CreateStartParams(int version_id, const GURL& scope, const GURL& script_url) {
    auto params = mojom::EmbeddedWorkerStartParams::New();
    params->service_worker_version_id = version_id;
    params->scope = scope;
    params->script_url = script_url;
    params->pause_after_download = false;
    params->is_installed = false;

    params->dispatcher_request = CreateEventDispatcher();
    params->controller_request = CreateController();
    params->installed_scripts_info = GetInstalledScriptsInfoPtr();
    return params;
  }

  mojom::ServiceWorkerProviderInfoForStartWorkerPtr CreateProviderInfo(
      int /* process_id */,
      scoped_refptr<network::SharedURLLoaderFactory>) {
    provider_host_endpoints_.emplace_back(
        std::make_unique<ProviderHostEndpoints>());
    return provider_host_endpoints_.back()->CreateProviderInfoPtr();
  }

  EmbeddedWorkerInstance::ProviderInfoGetter CreateProviderInfoGetter() {
    return base::BindOnce(&EmbeddedWorkerInstanceTest::CreateProviderInfo,
                          base::Unretained(this));
  }

  mojom::ServiceWorkerEventDispatcherRequest CreateEventDispatcher() {
    dispatchers_.emplace_back();
    return mojo::MakeRequest(&dispatchers_.back());
  }

  mojom::ControllerServiceWorkerRequest CreateController() {
    controllers_.emplace_back();
    return mojo::MakeRequest(&controllers_.back());
  }

  void SetWorkerStatus(EmbeddedWorkerInstance* worker,
                       EmbeddedWorkerStatus status) {
    worker->status_ = status;
  }

  blink::mojom::ServiceWorkerInstalledScriptsInfoPtr
  GetInstalledScriptsInfoPtr() {
    installed_scripts_managers_.emplace_back();
    auto info = blink::mojom::ServiceWorkerInstalledScriptsInfo::New();
    info->manager_request =
        mojo::MakeRequest(&installed_scripts_managers_.back());
    installed_scripts_manager_host_requests_.push_back(
        mojo::MakeRequest(&info->manager_host_ptr));
    return info;
  }

  ServiceWorkerContextCore* context() { return helper_->context(); }

  EmbeddedWorkerRegistry* embedded_worker_registry() {
    DCHECK(context());
    return context()->embedded_worker_registry();
  }

  IPC::TestSink* ipc_sink() { return helper_->ipc_sink(); }

  std::vector<std::unique_ptr<
      EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient>>*
  mock_instance_clients() {
    return helper_->mock_instance_clients();
  }

  // Mojo endpoints.
  std::vector<mojom::ServiceWorkerEventDispatcherPtr> dispatchers_;
  std::vector<mojom::ControllerServiceWorkerPtr> controllers_;
  std::vector<blink::mojom::ServiceWorkerInstalledScriptsManagerPtr>
      installed_scripts_managers_;
  std::vector<blink::mojom::ServiceWorkerInstalledScriptsManagerHostRequest>
      installed_scripts_manager_host_requests_;
  std::vector<std::unique_ptr<ProviderHostEndpoints>> provider_host_endpoints_;

  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  std::vector<EventLog> events_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EmbeddedWorkerInstanceTest);
};

// A helper to simulate the start worker sequence is stalled in a worker
// process.
class StalledInStartWorkerHelper : public EmbeddedWorkerTestHelper {
 public:
  StalledInStartWorkerHelper() : EmbeddedWorkerTestHelper(base::FilePath()) {}
  ~StalledInStartWorkerHelper() override {}

  void OnStartWorker(
      int embedded_worker_id,
      int64_t service_worker_version_id,
      const GURL& scope,
      const GURL& script_url,
      bool pause_after_download,
      mojom::ServiceWorkerEventDispatcherRequest dispatcher_request,
      mojom::ControllerServiceWorkerRequest controller_request,
      mojom::EmbeddedWorkerInstanceHostAssociatedPtrInfo instance_host,
      mojom::ServiceWorkerProviderInfoForStartWorkerPtr provider_info,
      blink::mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info)
      override {
    if (force_stall_in_start_) {
      // Prepare for OnStopWorker().
      instance_host_ptr_map_[embedded_worker_id].Bind(std::move(instance_host));
      // Do nothing to simulate a stall in the worker process.
      return;
    }
    EmbeddedWorkerTestHelper::OnStartWorker(
        embedded_worker_id, service_worker_version_id, scope, script_url,
        pause_after_download, std::move(dispatcher_request),
        std::move(controller_request), std::move(instance_host),
        std::move(provider_info), std::move(installed_scripts_info));
  }

  void OnStopWorker(int embedded_worker_id) override {
    if (instance_host_ptr_map_[embedded_worker_id]) {
      instance_host_ptr_map_[embedded_worker_id]->OnStopped();
      base::RunLoop().RunUntilIdle();
      return;
    }
    EmbeddedWorkerTestHelper::OnStopWorker(embedded_worker_id);
  }

  void set_force_stall_in_start(bool force_stall_in_start) {
    force_stall_in_start_ = force_stall_in_start;
  }

 private:
  bool force_stall_in_start_ = true;

  std::map<
      int /* embedded_worker_id */,
      mojom::EmbeddedWorkerInstanceHostAssociatedPtr /* instance_host_ptr */>
      instance_host_ptr_map_;
};

TEST_F(EmbeddedWorkerInstanceTest, StartAndStop) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t service_worker_version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
  worker->AddObserver(this);

  // Start should succeed.
  ServiceWorkerStatusCode status;
  base::RunLoop run_loop;
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(service_worker_version_id, pattern, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
  EXPECT_EQ(EmbeddedWorkerStatus::STARTING, worker->status());
  run_loop.Run();
  EXPECT_EQ(SERVICE_WORKER_OK, status);

  // The 'WorkerStarted' message should have been sent by
  // EmbeddedWorkerTestHelper.
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());
  EXPECT_EQ(helper_->mock_render_process_id(), worker->process_id());

  // Stop the worker.
  worker->Stop();
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPING, worker->status());
  base::RunLoop().RunUntilIdle();

  // The 'WorkerStopped' message should have been sent by
  // EmbeddedWorkerTestHelper.
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());

  // Check if the IPCs are fired in expected order.
  ASSERT_EQ(4u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  EXPECT_EQ(STARTED, events_[2].type);
  EXPECT_EQ(STOPPED, events_[3].type);
}

// Test that a worker that failed twice will use a new render process
// on the next attempt.
TEST_F(EmbeddedWorkerInstanceTest, ForceNewProcess) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t service_worker_version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());

  {
    // Start once normally.
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
    base::RunLoop run_loop;
    mojom::EmbeddedWorkerStartParamsPtr params =
        CreateStartParams(service_worker_version_id, pattern, url);
    worker->Start(
        std::move(params), CreateProviderInfoGetter(),
        base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_EQ(SERVICE_WORKER_OK, status);
    EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());
    // The worker should be using the default render process.
    EXPECT_EQ(helper_->mock_render_process_id(), worker->process_id());

    worker->Stop();
    base::RunLoop().RunUntilIdle();
  }

  // Fail twice.
  context()->UpdateVersionFailureCount(service_worker_version_id,
                                       SERVICE_WORKER_ERROR_FAILED);
  context()->UpdateVersionFailureCount(service_worker_version_id,
                                       SERVICE_WORKER_ERROR_FAILED);

  {
    // Start again.
    ServiceWorkerStatusCode status;
    base::RunLoop run_loop;
    mojom::EmbeddedWorkerStartParamsPtr params =
        CreateStartParams(service_worker_version_id, pattern, url);
    worker->Start(
        std::move(params), CreateProviderInfoGetter(),
        base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
    EXPECT_EQ(EmbeddedWorkerStatus::STARTING, worker->status());
    run_loop.Run();
    EXPECT_EQ(SERVICE_WORKER_OK, status);

    EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());
    // The worker should be using the new render process.
    EXPECT_EQ(helper_->new_render_process_id(), worker->process_id());
    worker->Stop();
    base::RunLoop().RunUntilIdle();
  }
}

TEST_F(EmbeddedWorkerInstanceTest, StopWhenDevToolsAttached) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t service_worker_version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());

  // Start the worker and then call StopIfNotAttachedToDevTools().
  EXPECT_EQ(SERVICE_WORKER_OK,
            StartWorker(worker.get(), service_worker_version_id, pattern, url));
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());
  EXPECT_EQ(helper_->mock_render_process_id(), worker->process_id());
  worker->StopIfNotAttachedToDevTools();
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPING, worker->status());
  base::RunLoop().RunUntilIdle();

  // The worker must be stopped now.
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());

  // Set devtools_attached to true, and do the same.
  worker->SetDevToolsAttached(true);

  EXPECT_EQ(SERVICE_WORKER_OK,
            StartWorker(worker.get(), service_worker_version_id, pattern, url));
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());
  EXPECT_EQ(helper_->mock_render_process_id(), worker->process_id());
  worker->StopIfNotAttachedToDevTools();
  base::RunLoop().RunUntilIdle();

  // The worker must not be stopped this time.
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());

  // Calling Stop() actually stops the worker regardless of whether devtools
  // is attached or not.
  worker->Stop();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
}

// Test that the removal of a worker from the registry doesn't remove
// other workers in the same process.
TEST_F(EmbeddedWorkerInstanceTest, RemoveWorkerInSharedProcess) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair1 =
      PrepareRegistrationAndVersion(pattern, url);
  std::unique_ptr<EmbeddedWorkerInstance> worker1 =
      embedded_worker_registry()->CreateWorker(pair1.second.get());
  RegistrationAndVersionPair pair2 =
      PrepareRegistrationAndVersion(pattern, url);
  std::unique_ptr<EmbeddedWorkerInstance> worker2 =
      embedded_worker_registry()->CreateWorker(pair2.second.get());

  const int64_t version_id1 = pair1.second->version_id();
  const int64_t version_id2 = pair2.second->version_id();
  int process_id = helper_->mock_render_process_id();

  {
    // Start worker1.
    ServiceWorkerStatusCode status;
    base::RunLoop run_loop;
    mojom::EmbeddedWorkerStartParamsPtr params =
        CreateStartParams(version_id1, pattern, url);
    worker1->Start(
        std::move(params), CreateProviderInfoGetter(),
        base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_EQ(SERVICE_WORKER_OK, status);
  }

  {
    // Start worker2.
    ServiceWorkerStatusCode status;
    base::RunLoop run_loop;
    mojom::EmbeddedWorkerStartParamsPtr params =
        CreateStartParams(version_id2, pattern, url);
    worker2->Start(
        std::move(params), CreateProviderInfoGetter(),
        base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_EQ(SERVICE_WORKER_OK, status);
  }

  // The two workers share the same process.
  EXPECT_EQ(worker1->process_id(), worker2->process_id());

  // Destroy worker1. It removes itself from the registry.
  int worker1_id = worker1->embedded_worker_id();
  worker1->Stop();
  worker1.reset();

  // Only worker1 should be removed from the registry's process_map.
  EmbeddedWorkerRegistry* registry =
      helper_->context()->embedded_worker_registry();
  EXPECT_EQ(0UL, registry->worker_process_map_[process_id].count(worker1_id));
  EXPECT_EQ(1UL, registry->worker_process_map_[process_id].count(
                     worker2->embedded_worker_id()));

  worker2->Stop();
}

TEST_F(EmbeddedWorkerInstanceTest, DetachDuringProcessAllocation) {
  const GURL scope("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(scope, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Run the start worker sequence and detach during process allocation.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, scope, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, base::DoNothing::Once<>()));
  worker->Detach();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
  EXPECT_EQ(ChildProcessHost::kInvalidUniqueID, worker->process_id());

  // The start callback should not be aborted by detach (see a comment on the
  // dtor of EmbeddedWorkerInstance::StartTask).
  EXPECT_EQ(SERVICE_WORKER_ERROR_MAX_VALUE, status);

  // "PROCESS_ALLOCATED" event should not be recorded.
  ASSERT_EQ(1u, events_.size());
  EXPECT_EQ(DETACHED, events_[0].type);
  EXPECT_EQ(EmbeddedWorkerStatus::STARTING, events_[0].status);
}

TEST_F(EmbeddedWorkerInstanceTest, DetachAfterSendingStartWorkerMessage) {
  const GURL scope("http://example.com/");
  const GURL url("http://example.com/worker.js");

  helper_.reset(new StalledInStartWorkerHelper());
  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(scope, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Run the start worker sequence until a start worker message is sent.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, scope, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, base::DoNothing::Once<>()));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(2u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  events_.clear();

  worker->Detach();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
  EXPECT_EQ(ChildProcessHost::kInvalidUniqueID, worker->process_id());

  // The start callback should not be aborted by detach (see a comment on the
  // dtor of EmbeddedWorkerInstance::StartTask).
  EXPECT_EQ(SERVICE_WORKER_ERROR_MAX_VALUE, status);

  // "STARTED" event should not be recorded.
  ASSERT_EQ(1u, events_.size());
  EXPECT_EQ(DETACHED, events_[0].type);
  EXPECT_EQ(EmbeddedWorkerStatus::STARTING, events_[0].status);
}

TEST_F(EmbeddedWorkerInstanceTest, StopDuringProcessAllocation) {
  const GURL scope("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(scope, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Stop the start worker sequence before a process is allocated.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;

  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, scope, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, base::DoNothing::Once<>()));
  worker->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
  EXPECT_EQ(ChildProcessHost::kInvalidUniqueID, worker->process_id());

  // The start callback should not be aborted by stop (see a comment on the dtor
  // of EmbeddedWorkerInstance::StartTask).
  EXPECT_EQ(SERVICE_WORKER_ERROR_MAX_VALUE, status);

  // "PROCESS_ALLOCATED" event should not be recorded.
  ASSERT_EQ(1u, events_.size());
  EXPECT_EQ(STOPPED, events_[0].type);
  EXPECT_EQ(EmbeddedWorkerStatus::STARTING, events_[0].status);
  events_.clear();

  // Restart the worker.
  status = SERVICE_WORKER_ERROR_MAX_VALUE;
  std::unique_ptr<base::RunLoop> run_loop(new base::RunLoop);
  params = CreateStartParams(version_id, scope, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, run_loop->QuitClosure()));
  run_loop->Run();

  EXPECT_EQ(SERVICE_WORKER_OK, status);
  ASSERT_EQ(3u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  EXPECT_EQ(STARTED, events_[2].type);

  // Tear down the worker.
  worker->Stop();
}

class DontReceiveResumeAfterDownloadInstanceClient
    : public EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient {
 public:
  explicit DontReceiveResumeAfterDownloadInstanceClient(
      base::WeakPtr<EmbeddedWorkerTestHelper> helper,
      bool* was_resume_after_download_called)
      : EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient(helper),
        was_resume_after_download_called_(was_resume_after_download_called) {}

 private:
  void ResumeAfterDownload() override {
    *was_resume_after_download_called_ = true;
  }

  bool* const was_resume_after_download_called_;
};

TEST_F(EmbeddedWorkerInstanceTest, StopDuringPausedAfterDownload) {
  const GURL scope("http://example.com/");
  const GURL url("http://example.com/worker.js");

  bool was_resume_after_download_called = false;
  helper_->RegisterMockInstanceClient(
      std::make_unique<DontReceiveResumeAfterDownloadInstanceClient>(
          helper_->AsWeakPtr(), &was_resume_after_download_called));

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(scope, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Run the start worker sequence until pause after download.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;

  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, scope, url);
  params->pause_after_download = true;
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, base::DoNothing::Once<>()));
  base::RunLoop().RunUntilIdle();

  // Make the worker stopping and attempt to send a resume after download
  // message.
  worker->Stop();
  worker->ResumeAfterDownload();
  base::RunLoop().RunUntilIdle();

  // The resume after download message should not have been sent.
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
  EXPECT_FALSE(was_resume_after_download_called);
}

TEST_F(EmbeddedWorkerInstanceTest, StopAfterSendingStartWorkerMessage) {
  const GURL scope("http://example.com/");
  const GURL url("http://example.com/worker.js");

  helper_.reset(new StalledInStartWorkerHelper);
  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(scope, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Run the start worker sequence until a start worker message is sent.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, scope, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, base::DoNothing::Once<>()));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(2u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  events_.clear();

  worker->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
  EXPECT_EQ(ChildProcessHost::kInvalidUniqueID, worker->process_id());

  // The start callback should not be aborted by stop (see a comment on the dtor
  // of EmbeddedWorkerInstance::StartTask).
  EXPECT_EQ(SERVICE_WORKER_ERROR_MAX_VALUE, status);

  // "STARTED" event should not be recorded.
  ASSERT_EQ(1u, events_.size());
  EXPECT_EQ(STOPPED, events_[0].type);
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPING, events_[0].status);
  events_.clear();

  // Restart the worker.
  static_cast<StalledInStartWorkerHelper*>(helper_.get())
      ->set_force_stall_in_start(false);
  status = SERVICE_WORKER_ERROR_MAX_VALUE;
  std::unique_ptr<base::RunLoop> run_loop(new base::RunLoop);

  params = CreateStartParams(version_id, scope, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, run_loop->QuitClosure()));
  run_loop->Run();

  // The worker should be started.
  EXPECT_EQ(SERVICE_WORKER_OK, status);
  ASSERT_EQ(3u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  EXPECT_EQ(STARTED, events_[2].type);

  // Tear down the worker.
  worker->Stop();
}

TEST_F(EmbeddedWorkerInstanceTest, Detach) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
  worker->AddObserver(this);

  // Start the worker.
  base::RunLoop run_loop;
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, pattern, url);
  worker->Start(
      std::move(params), CreateProviderInfoGetter(),
      base::BindOnce(&SaveStatusAndCall, &status, run_loop.QuitClosure()));
  run_loop.Run();

  // Detach.
  int process_id = worker->process_id();
  worker->Detach();
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());

  // Send the registry a message from the detached worker. Nothing should
  // happen.
  embedded_worker_registry()->OnWorkerStarted(process_id,
                                              worker->embedded_worker_id());
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
}

// Test for when sending the start IPC failed.
TEST_F(EmbeddedWorkerInstanceTest, FailToSendStartIPC) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  // Let StartWorker fail; mojo IPC fails to connect to a remote interface.
  helper_->RegisterMockInstanceClient(nullptr);

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Attempt to start the worker.
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, pattern, url);
  worker->Start(std::move(params), CreateProviderInfoGetter(),
                base::DoNothing());
  base::RunLoop().RunUntilIdle();

  // Worker should handle the failure of binding on the remote side as detach.
  ASSERT_EQ(3u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  EXPECT_EQ(DETACHED, events_[2].type);
  EXPECT_EQ(EmbeddedWorkerStatus::STARTING, events_[2].status);
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
}

class FailEmbeddedWorkerInstanceClientImpl
    : public EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient {
 public:
  explicit FailEmbeddedWorkerInstanceClientImpl(
      base::WeakPtr<EmbeddedWorkerTestHelper> helper)
      : EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient(helper) {}

 private:
  void StartWorker(mojom::EmbeddedWorkerStartParamsPtr) override {
    helper_->mock_instance_clients()->clear();
  }
};

TEST_F(EmbeddedWorkerInstanceTest, RemoveRemoteInterface) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");

  // Let StartWorker fail; binding is discarded in the middle of IPC
  helper_->RegisterMockInstanceClient(
      std::make_unique<FailEmbeddedWorkerInstanceClientImpl>(
          helper_->AsWeakPtr()));
  ASSERT_EQ(mock_instance_clients()->size(), 1UL);

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Attempt to start the worker.
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, pattern, url);
  worker->Start(std::move(params), CreateProviderInfoGetter(),
                base::DoNothing());
  base::RunLoop().RunUntilIdle();

  // Worker should handle the sudden shutdown as detach.
  ASSERT_EQ(3u, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  EXPECT_EQ(DETACHED, events_[2].type);
  EXPECT_EQ(EmbeddedWorkerStatus::STARTING, events_[2].status);
}

class StoreMessageInstanceClient
    : public EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient {
 public:
  explicit StoreMessageInstanceClient(
      base::WeakPtr<EmbeddedWorkerTestHelper> helper)
      : EmbeddedWorkerTestHelper::MockEmbeddedWorkerInstanceClient(helper) {}

  const std::vector<std::pair<blink::WebConsoleMessage::Level, std::string>>&
  message() {
    return messages_;
  }

 private:
  void AddMessageToConsole(blink::WebConsoleMessage::Level level,
                           const std::string& message) override {
    messages_.push_back(std::make_pair(level, message));
  }

  std::vector<std::pair<blink::WebConsoleMessage::Level, std::string>>
      messages_;
};

TEST_F(EmbeddedWorkerInstanceTest, AddMessageToConsole) {
  const GURL pattern("http://example.com/");
  const GURL url("http://example.com/worker.js");
  std::unique_ptr<StoreMessageInstanceClient> instance_client =
      std::make_unique<StoreMessageInstanceClient>(helper_->AsWeakPtr());
  StoreMessageInstanceClient* instance_client_rawptr = instance_client.get();
  helper_->RegisterMockInstanceClient(std::move(instance_client));
  ASSERT_EQ(mock_instance_clients()->size(), 1UL);

  RegistrationAndVersionPair pair = PrepareRegistrationAndVersion(pattern, url);
  const int64_t version_id = pair.second->version_id();
  std::unique_ptr<EmbeddedWorkerInstance> worker =
      embedded_worker_registry()->CreateWorker(pair.second.get());
  worker->AddObserver(this);

  // Attempt to start the worker and immediate AddMessageToConsole should not
  // cause a crash.
  std::pair<blink::WebConsoleMessage::Level, std::string> test_message =
      std::make_pair(blink::WebConsoleMessage::kLevelVerbose, "");
  mojom::EmbeddedWorkerStartParamsPtr params =
      CreateStartParams(version_id, pattern, url);
  worker->Start(std::move(params), CreateProviderInfoGetter(),
                base::DoNothing());
  worker->AddMessageToConsole(test_message.first, test_message.second);
  base::RunLoop().RunUntilIdle();

  // Messages sent before sending StartWorker message won't be dispatched.
  ASSERT_EQ(0UL, instance_client_rawptr->message().size());
  ASSERT_EQ(3UL, events_.size());
  EXPECT_EQ(PROCESS_ALLOCATED, events_[0].type);
  EXPECT_EQ(START_WORKER_MESSAGE_SENT, events_[1].type);
  EXPECT_EQ(STARTED, events_[2].type);
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, worker->status());

  worker->AddMessageToConsole(test_message.first, test_message.second);
  base::RunLoop().RunUntilIdle();

  // Messages sent after sending StartWorker message should be reached to
  // the renderer.
  ASSERT_EQ(1UL, instance_client_rawptr->message().size());
  EXPECT_EQ(test_message, instance_client_rawptr->message()[0]);

  // Ensure the worker is stopped.
  worker->Stop();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(EmbeddedWorkerStatus::STOPPED, worker->status());
}

}  // namespace content
