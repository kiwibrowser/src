// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_job_controller.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "components/download/public/common/download_item.h"
#include "content/browser/background_fetch/background_fetch_constants.h"
#include "content/browser/background_fetch/background_fetch_data_manager.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/browser/background_fetch/background_fetch_test_base.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/fake_download_item.h"
#include "content/public/test/mock_download_manager.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;

namespace content {
namespace {

const int64_t kExampleServiceWorkerRegistrationId = 1;
const char kExampleDeveloperId[] = "my-example-id";
const char kExampleResponseData[] = "My response data";

enum class JobCompletionStatus { kRunning, kCompleted, kAborted };

class FakeBackgroundFetchRequestManager : public BackgroundFetchRequestManager {
 public:
  void AddDownloadJob(const BackgroundFetchRegistrationId& registration_id,
                      const std::set<std::string>& download_guids) {
    DCHECK(!registration_status_map_.count(registration_id.unique_id()));
    registration_status_map_.emplace(registration_id.unique_id(),
                                     RegistrationState(download_guids));
  }

  // BackgroundFetchRequestManager implementation:
  void MarkRequestAsComplete(
      const BackgroundFetchRegistrationId& registration_id,
      scoped_refptr<BackgroundFetchRequestInfo> request) override {
    DCHECK(registration_status_map_.count(registration_id.unique_id()));
    auto& state = registration_status_map_[registration_id.unique_id()];

    DCHECK_EQ(state.status, JobCompletionStatus::kRunning);
    DCHECK(state.uncompleted_downloads.count(request->download_guid()));
    state.uncompleted_downloads.erase(request->download_guid());

    if (state.uncompleted_downloads.size() == 0) {
      state.status = JobCompletionStatus::kCompleted;
    }
  }

  void OnJobAborted(const BackgroundFetchRegistrationId& registration_id,
                    std::vector<std::string> aborted_guids) override {
    DCHECK(registration_status_map_.count(registration_id.unique_id()));
    auto& state = registration_status_map_[registration_id.unique_id()];
    DCHECK_EQ(state.status, JobCompletionStatus::kRunning);
    state.status = JobCompletionStatus::kAborted;
  }

  JobCompletionStatus GetCompletionStatus(
      const BackgroundFetchRegistrationId& registration_id) {
    DCHECK(registration_status_map_.count(registration_id.unique_id()));
    return registration_status_map_[registration_id.unique_id()].status;
  }

  struct RegistrationState {
    RegistrationState() = default;
    explicit RegistrationState(const std::set<std::string>& downloads)
        : uncompleted_downloads(downloads) {}
    JobCompletionStatus status = JobCompletionStatus::kRunning;
    std::set<std::string> uncompleted_downloads;
  };

  std::map<std::string, RegistrationState> registration_status_map_;
};

class BackgroundFetchJobControllerTest : public BackgroundFetchTestBase {
 public:
  BackgroundFetchJobControllerTest() = default;

  ~BackgroundFetchJobControllerTest() override = default;

  // Creates a new Background Fetch registration, whose id will be stored in the
  // |*registration_id|, and registers it with the DataManager for the included
  // |request_data|. If |auto_complete_requests| is true, the request will
  // immediately receive a successful response. Should be wrapped in
  // ASSERT_NO_FATAL_FAILURE().
  std::vector<scoped_refptr<BackgroundFetchRequestInfo>>
  CreateRegistrationForRequests(
      BackgroundFetchRegistrationId* registration_id,
      std::map<GURL, std::string /* method */> request_data,
      bool auto_complete_requests) {
    DCHECK(registration_id);

    // New |unique_id|, since this is a new Background Fetch registration.
    *registration_id = BackgroundFetchRegistrationId(
        kExampleServiceWorkerRegistrationId, origin(), kExampleDeveloperId,
        base::GenerateGUID());

    std::vector<scoped_refptr<BackgroundFetchRequestInfo>> request_infos;
    std::set<std::string> uncompleted_downloads_guids;
    int request_counter = 0;
    for (const auto& pair : request_data) {
      ServiceWorkerFetchRequest fetch_request(
          GURL(pair.first), pair.second, ServiceWorkerHeaderMap(), Referrer(),
          false /* is_reload */);
      auto request = base::MakeRefCounted<BackgroundFetchRequestInfo>(
          request_counter++, fetch_request);
      request->InitializeDownloadGuid();
      request_infos.push_back(request);
      uncompleted_downloads_guids.insert(request->download_guid());
    }

    request_manager_.AddDownloadJob(*registration_id,
                                    uncompleted_downloads_guids);

    if (auto_complete_requests) {
      // Provide fake responses for the given |request_data| pairs.
      for (const auto& pair : request_data) {
        CreateRequestWithProvidedResponse(
            pair.second /* method */, pair.first /* url */,
            TestResponseBuilder(200 /* response_code */)
                .SetResponseData(kExampleResponseData)
                .Build());
      }
    }

    return request_infos;
  }

  // Creates a new BackgroundFetchJobController instance.
  std::unique_ptr<BackgroundFetchJobController> CreateJobController(
      const BackgroundFetchRegistrationId& registration_id,
      int total_downloads) {
    delegate_ = browser_context()->GetBackgroundFetchDelegate();
    DCHECK(delegate_);
    delegate_proxy_ = std::make_unique<BackgroundFetchDelegateProxy>(delegate_);

    BackgroundFetchRegistration registration;
    registration.developer_id = registration_id.developer_id();
    registration.unique_id = registration_id.unique_id();

    auto controller = std::make_unique<BackgroundFetchJobController>(
        delegate_proxy_.get(), registration_id, BackgroundFetchOptions(),
        SkBitmap(), registration, &request_manager_,
        base::BindRepeating(
            &BackgroundFetchJobControllerTest::DidUpdateProgress,
            base::Unretained(this)),
        base::BindOnce(&BackgroundFetchJobControllerTest::OnJobFinished));

    controller->InitializeRequestStatus(0, total_downloads,
                                        std::vector<std::string>());
    return controller;
  }

 protected:
  FakeBackgroundFetchRequestManager request_manager_;

  uint64_t last_downloaded_ = 0;

  // Closure that will be invoked every time the JobController receives a
  // progress update from a download.
  base::RepeatingClosure job_progress_closure_;

  std::unique_ptr<BackgroundFetchDelegateProxy> delegate_proxy_;
  BackgroundFetchDelegate* delegate_;

 private:
  void DidUpdateProgress(const std::string& unique_id,
                         uint64_t download_total,
                         uint64_t downloaded) {
    last_downloaded_ = downloaded;

    if (job_progress_closure_)
      job_progress_closure_.Run();
  }

  static void OnJobFinished(const BackgroundFetchRegistrationId&,
                            BackgroundFetchReasonToAbort reason_to_abort) {}

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchJobControllerTest);
};

TEST_F(BackgroundFetchJobControllerTest, SingleRequestJob) {
  BackgroundFetchRegistrationId registration_id;

  auto requests = CreateRegistrationForRequests(
      &registration_id, {{GURL("https://example.com/funny_cat.png"), "GET"}},
      true /* auto_complete_requests */);

  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  std::unique_ptr<BackgroundFetchJobController> controller =
      CreateJobController(registration_id, requests.size());

  controller->StartRequest(requests[0]);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(JobCompletionStatus::kCompleted,
            request_manager_.GetCompletionStatus(registration_id));
}

TEST_F(BackgroundFetchJobControllerTest, MultipleRequestJob) {
  BackgroundFetchRegistrationId registration_id;

  auto requests = CreateRegistrationForRequests(
      &registration_id,
      {{GURL("https://example.com/funny_cat.png"), "GET"},
       {GURL("https://example.com/scary_cat.png"), "GET"},
       {GURL("https://example.com/crazy_cat.png"), "GET"}},
      true /* auto_complete_requests */);

  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  std::unique_ptr<BackgroundFetchJobController> controller =
      CreateJobController(registration_id, requests.size());

  controller->StartRequest(requests[0]);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  controller->StartRequest(requests[1]);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  controller->StartRequest(requests[2]);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(JobCompletionStatus::kCompleted,
            request_manager_.GetCompletionStatus(registration_id));
}

TEST_F(BackgroundFetchJobControllerTest, Abort) {
  BackgroundFetchRegistrationId registration_id;

  auto requests = CreateRegistrationForRequests(
      &registration_id, {{GURL("https://example.com/funny_cat.png"), "GET"}},
      true /* auto_complete_requests */);

  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  std::unique_ptr<BackgroundFetchJobController> controller =
      CreateJobController(registration_id, requests.size());

  controller->StartRequest(requests[0]);
  controller->Abort(BackgroundFetchReasonToAbort::CANCELLED_FROM_UI);
  // Tell the delegate to abort the job as well so it doesn't send completed
  // messages to the JobController.
  delegate_->Abort(registration_id.unique_id());

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(JobCompletionStatus::kAborted,
            request_manager_.GetCompletionStatus(registration_id));
}

TEST_F(BackgroundFetchJobControllerTest, Progress) {
  BackgroundFetchRegistrationId registration_id;

  auto requests = CreateRegistrationForRequests(
      &registration_id, {{GURL("https://example.com/funny_cat.png"), "GET"}},
      true /* auto_complete_requests */);

  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  std::unique_ptr<BackgroundFetchJobController> controller =
      CreateJobController(registration_id, requests.size());

  controller->StartRequest(requests[0]);

  {
    base::RunLoop run_loop;
    job_progress_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  EXPECT_GT(last_downloaded_, 0u);
  EXPECT_LT(last_downloaded_, strlen(kExampleResponseData));
  EXPECT_EQ(JobCompletionStatus::kRunning,
            request_manager_.GetCompletionStatus(registration_id));

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(JobCompletionStatus::kCompleted,
            request_manager_.GetCompletionStatus(registration_id));
  EXPECT_EQ(last_downloaded_, strlen(kExampleResponseData));
}

}  // namespace
}  // namespace content
