// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_scheduler.h"

#include <vector>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "content/browser/background_fetch/background_fetch_request_info.h"
#include "content/browser/background_fetch/background_fetch_test_base.h"
#include "content/public/browser/browser_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAre;

namespace content {

namespace {

const int kExampleServiceWorkerRegistrationId = 1;
const char kExampleDeveloperId1[] = "my-example-id";
const char kExampleDeveloperId2[] = "my-other-id";

class FakeController : public BackgroundFetchScheduler::Controller {
 public:
  FakeController(const BackgroundFetchRegistrationId& registration_id,
                 BackgroundFetchScheduler* scheduler,
                 const std::string& name,
                 std::vector<std::string>* controller_sequence_list,
                 int total_jobs)
      : BackgroundFetchScheduler::Controller(
            registration_id,
            base::BindOnce(&FakeController::OnJobFinished)),
        scheduler_(scheduler),
        controller_sequence_list_(controller_sequence_list),
        name_(name),
        total_jobs_(total_jobs) {}

  ~FakeController() override {}

  // BackgroundFetchScheduler::Controller implementation:
  bool HasMoreRequests() override { return jobs_started_ < total_jobs_; }
  void StartRequest(
      scoped_refptr<BackgroundFetchRequestInfo> request) override {
    DCHECK_LT(jobs_started_, total_jobs_);
    ++jobs_started_;
    controller_sequence_list_->push_back(name_ +
                                         base::IntToString(jobs_started_));
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&FakeController::OnDownloadCompleted,
                       base::Unretained(this), std::move(request)));
  }

 private:
  void OnDownloadCompleted(scoped_refptr<BackgroundFetchRequestInfo> request) {
    scheduler_->MarkRequestAsComplete(registration_id(), std::move(request));
  }

  static void OnJobFinished(const BackgroundFetchRegistrationId&,
                            BackgroundFetchReasonToAbort reason_to_abort) {}

  int jobs_started_ = 0;
  BackgroundFetchScheduler* scheduler_;
  std::vector<std::string>* controller_sequence_list_;
  std::string name_;
  int total_jobs_;
};

class BackgroundFetchSchedulerTest
    : public BackgroundFetchTestBase,
      public BackgroundFetchScheduler::RequestProvider {
 public:
  BackgroundFetchSchedulerTest() : scheduler_(this) {}

  // Posts itself as a task |number_of_barriers| times and on the last iteration
  // invokes the quit_closure.
  void PostQuitAfterRepeatingBarriers(base::Closure quit_closure,
                                      int number_of_barriers) {
    if (--number_of_barriers == 0) {
      std::move(quit_closure).Run();
      return;
    }

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &BackgroundFetchSchedulerTest::PostQuitAfterRepeatingBarriers,
            base::Unretained(this), std::move(quit_closure),
            number_of_barriers));
  }

  void PopNextRequest(
      const BackgroundFetchRegistrationId& registration_id,
      BackgroundFetchScheduler::NextRequestCallback callback) override {
    ServiceWorkerFetchRequest fetch_request(GURL(), "GET",
                                            ServiceWorkerHeaderMap(),
                                            Referrer(), false /* is_reload */);
    auto request = base::MakeRefCounted<BackgroundFetchRequestInfo>(
        0 /* request_count */, fetch_request);
    request->InitializeDownloadGuid();

    std::move(callback).Run(std::move(request));
  }

  void MarkRequestAsComplete(
      const BackgroundFetchRegistrationId& registration_id,
      BackgroundFetchRequestInfo* request,
      BackgroundFetchScheduler::MarkedCompleteCallback callback) override {
    std::move(callback).Run();
  }

 protected:
  BackgroundFetchScheduler scheduler_;
  std::vector<std::string> controller_sequence_list_;
};

}  // namespace

TEST_F(BackgroundFetchSchedulerTest, SingleController) {
  BackgroundFetchRegistrationId registration_id1(
      kExampleServiceWorkerRegistrationId, origin(), kExampleDeveloperId1,
      base::GenerateGUID());
  FakeController controller(registration_id1, &scheduler_, "A",
                            &controller_sequence_list_, 4);

  scheduler_.AddJobController(&controller);

  base::RunLoop().RunUntilIdle();

  EXPECT_THAT(controller_sequence_list_, ElementsAre("A1", "A2", "A3", "A4"));
}

TEST_F(BackgroundFetchSchedulerTest, TwoControllers) {
  BackgroundFetchRegistrationId registration_id1(
      kExampleServiceWorkerRegistrationId, origin(), kExampleDeveloperId1,
      base::GenerateGUID());
  BackgroundFetchRegistrationId registration_id2(
      kExampleServiceWorkerRegistrationId, origin(), kExampleDeveloperId2,
      base::GenerateGUID());
  FakeController controller1(registration_id1, &scheduler_, "A",
                             &controller_sequence_list_, 4);
  FakeController controller2(registration_id2, &scheduler_, "B",
                             &controller_sequence_list_, 4);

  scheduler_.AddJobController(&controller1);
  scheduler_.AddJobController(&controller2);

  {
    base::RunLoop run_loop;
    PostQuitAfterRepeatingBarriers(run_loop.QuitClosure(), 3);
    run_loop.Run();

    // Only one task is run at a time so after 3 barrier iterations, 3 tasks
    // should have been have run.
    EXPECT_THAT(controller_sequence_list_, ElementsAre("A1", "B1", "A2"));
  }

  base::RunLoop().RunUntilIdle();

  EXPECT_THAT(controller_sequence_list_,
              ElementsAre("A1", "B1", "A2", "B2", "A3", "B3", "A4", "B4"));
}

TEST_F(BackgroundFetchSchedulerTest, TwoControllers_TwoConcurrent) {
  BackgroundFetchRegistrationId registration_id1(
      kExampleServiceWorkerRegistrationId, origin(), kExampleDeveloperId1,
      base::GenerateGUID());
  BackgroundFetchRegistrationId registration_id2(
      kExampleServiceWorkerRegistrationId, origin(), kExampleDeveloperId2,
      base::GenerateGUID());
  FakeController controller1(registration_id1, &scheduler_, "A",
                             &controller_sequence_list_, 4);
  FakeController controller2(registration_id2, &scheduler_, "B",
                             &controller_sequence_list_, 4);

  scheduler_.set_max_concurrent_downloads(2);
  scheduler_.AddJobController(&controller1);
  scheduler_.AddJobController(&controller2);

  {
    base::RunLoop run_loop;
    PostQuitAfterRepeatingBarriers(run_loop.QuitClosure(), 3);
    run_loop.Run();

    // Two tasks are run at a time so after 3 barrier iterations, 6 tasks should
    // have run.
    EXPECT_THAT(controller_sequence_list_,
                ElementsAre("A1", "B1", "A2", "B2", "A3", "B3"));
  }

  base::RunLoop().RunUntilIdle();

  EXPECT_THAT(controller_sequence_list_,
              ElementsAre("A1", "B1", "A2", "B2", "A3", "B3", "A4", "B4"));
}

}  // namespace content
