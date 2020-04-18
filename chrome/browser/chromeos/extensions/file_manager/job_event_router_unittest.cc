// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/job_event_router.h"

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace file_manager {
namespace {

class JobEventRouterImpl : public JobEventRouter {
 public:
  JobEventRouterImpl() : JobEventRouter(base::TimeDelta::FromMilliseconds(0)) {
    listener_extension_ids_.insert("extension_a");
  }
  std::vector<std::unique_ptr<base::DictionaryValue>> events;

  void SetListenerExtensionIds(std::set<std::string> extension_ids) {
    listener_extension_ids_ = extension_ids;
  }

 protected:
  std::set<std::string> GetFileTransfersUpdateEventListenerExtensionIds()
      override {
    return listener_extension_ids_;
  }

  GURL ConvertDrivePathToFileSystemUrl(
      const base::FilePath& file_path,
      const std::string& extension_id) override {
    std::string url;
    url.append("filesystem:chrome-extension://");
    url.append(extension_id);
    url.append(file_path.value());
    return GURL(url);
  }

  void DispatchEventToExtension(
      const std::string& extension_id,
      extensions::events::HistogramValue histogram_value,
      const std::string& event_name,
      std::unique_ptr<base::ListValue> event_args) override {
    const base::DictionaryValue* event;
    event_args->GetDictionary(0, &event);
    events.push_back(base::WrapUnique(event->DeepCopy()));
  }

 private:
  std::set<std::string> listener_extension_ids_;

  DISALLOW_COPY_AND_ASSIGN(JobEventRouterImpl);
};

class JobEventRouterTest : public testing::Test {
 protected:
  void SetUp() override { job_event_router.reset(new JobEventRouterImpl()); }

  drive::JobInfo CreateJobInfo(drive::JobID id,
                               int64_t num_completed_bytes,
                               int64_t num_total_bytes,
                               const base::FilePath& file_path) {
    drive::JobInfo job(drive::TYPE_DOWNLOAD_FILE);
    job.job_id = id;
    job.num_total_bytes = num_total_bytes;
    job.num_completed_bytes = num_completed_bytes;
    job.file_path = file_path;
    return job;
  }

  std::string GetEventString(size_t index, const std::string& name) {
    std::string value;
    job_event_router->events[index]->GetString(name, &value);
    return value;
  }

  double GetEventDouble(size_t index, const std::string& name) {
    double value = NAN;
    job_event_router->events[index]->GetDouble(name, &value);
    return value;
  }

  std::unique_ptr<JobEventRouterImpl> job_event_router;

 private:
  base::MessageLoop message_loop_;
};

TEST_F(JobEventRouterTest, Basic) {
  // Add a job.
  job_event_router->OnJobAdded(
      CreateJobInfo(0, 0, 100, base::FilePath("/test/a")));
  // Event should be throttled.
  ASSERT_EQ(0u, job_event_router->events.size());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, job_event_router->events.size());
  EXPECT_EQ("in_progress", GetEventString(0, "transferState"));
  EXPECT_EQ(0.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "total"));
  job_event_router->events.clear();

  // Job is updated.
  job_event_router->OnJobUpdated(
      CreateJobInfo(0, 50, 100, base::FilePath("/test/a")));
  job_event_router->OnJobUpdated(
      CreateJobInfo(0, 100, 100, base::FilePath("/test/a")));
  // Event should be throttled.
  ASSERT_EQ(0u, job_event_router->events.size());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, job_event_router->events.size());
  EXPECT_EQ("in_progress", GetEventString(0, "transferState"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "total"));
  job_event_router->events.clear();

  // Complete first job.
  job_event_router->OnJobDone(
      CreateJobInfo(0, 100, 100, base::FilePath("/test/a")),
      drive::FILE_ERROR_OK);
  // Complete event should not be throttled.
  ASSERT_EQ(1u, job_event_router->events.size());
  EXPECT_EQ("completed", GetEventString(0, "transferState"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "total"));
  job_event_router->events.clear();
}

TEST_F(JobEventRouterTest, CompleteWithInvalidCompletedBytes) {
  job_event_router->OnJobDone(
      CreateJobInfo(0, 50, 100, base::FilePath("/test/a")),
      drive::FILE_ERROR_OK);
  ASSERT_EQ(1u, job_event_router->events.size());
  EXPECT_EQ("completed", GetEventString(0, "transferState"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "total"));
}

TEST_F(JobEventRouterTest, AnotherJobAddedBeforeComplete) {
  job_event_router->OnJobAdded(
      CreateJobInfo(0, 0, 100, base::FilePath("/test/a")));
  job_event_router->OnJobUpdated(
      CreateJobInfo(0, 50, 100, base::FilePath("/test/a")));
  job_event_router->OnJobAdded(
      CreateJobInfo(1, 0, 100, base::FilePath("/test/b")));

  // Event should be throttled.
  ASSERT_EQ(0u, job_event_router->events.size());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, job_event_router->events.size());
  EXPECT_EQ("in_progress", GetEventString(0, "transferState"));
  EXPECT_EQ(50.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(200.0f, GetEventDouble(0, "total"));
  job_event_router->events.clear();

  job_event_router->OnJobDone(
      CreateJobInfo(0, 100, 100, base::FilePath("/test/a")),
      drive::FILE_ERROR_OK);
  job_event_router->OnJobDone(
      CreateJobInfo(1, 100, 100, base::FilePath("/test/b")),
      drive::FILE_ERROR_OK);
  // Complete event should not be throttled.
  ASSERT_EQ(2u, job_event_router->events.size());
  EXPECT_EQ("completed", GetEventString(0, "transferState"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(200.0f, GetEventDouble(0, "total"));
  EXPECT_EQ("completed", GetEventString(1, "transferState"));
  EXPECT_EQ(200.0f, GetEventDouble(1, "processed"));
  EXPECT_EQ(200.0f, GetEventDouble(1, "total"));
}

TEST_F(JobEventRouterTest, AnotherJobAddedAfterComplete) {
  job_event_router->OnJobAdded(
      CreateJobInfo(0, 0, 100, base::FilePath("/test/a")));
  job_event_router->OnJobUpdated(
      CreateJobInfo(0, 50, 100, base::FilePath("/test/a")));
  job_event_router->OnJobDone(
      CreateJobInfo(0, 100, 100, base::FilePath("/test/a")),
      drive::FILE_ERROR_OK);
  job_event_router->OnJobAdded(
      CreateJobInfo(1, 0, 100, base::FilePath("/test/b")));
  job_event_router->OnJobDone(
      CreateJobInfo(1, 100, 100, base::FilePath("/test/b")),
      drive::FILE_ERROR_OK);

  // Complete event should not be throttled.
  ASSERT_EQ(2u, job_event_router->events.size());
  EXPECT_EQ("completed", GetEventString(0, "transferState"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "processed"));
  // Total byte shold be reset when all tasks complete.
  EXPECT_EQ(100.0f, GetEventDouble(0, "total"));
  EXPECT_EQ("completed", GetEventString(1, "transferState"));
  EXPECT_EQ(100.0f, GetEventDouble(1, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(1, "total"));
}

TEST_F(JobEventRouterTest, UpdateTotalSizeAfterAdded) {
  job_event_router->OnJobAdded(
      CreateJobInfo(0, 0, 0, base::FilePath("/test/a")));
  base::RunLoop().RunUntilIdle();
  job_event_router->OnJobUpdated(
      CreateJobInfo(0, 0, 100, base::FilePath("/test/a")));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(2u, job_event_router->events.size());

  EXPECT_EQ("in_progress", GetEventString(0, "transferState"));
  EXPECT_EQ(0.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(0.0f, GetEventDouble(0, "total"));

  EXPECT_EQ("in_progress", GetEventString(1, "transferState"));
  EXPECT_EQ(0.0f, GetEventDouble(1, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(1, "total"));
}

TEST_F(JobEventRouterTest, MultipleListenerExtensions) {
  std::set<std::string> extension_ids;
  extension_ids.insert("extension_a");
  extension_ids.insert("extension_b");
  job_event_router->SetListenerExtensionIds(extension_ids);

  // Add a job.
  job_event_router->OnJobAdded(
      CreateJobInfo(0, 0, 100, base::FilePath("/test/a")));
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(2u, job_event_router->events.size());

  // Check event for extension_a.
  EXPECT_EQ("in_progress", GetEventString(0, "transferState"));
  EXPECT_EQ(0.0f, GetEventDouble(0, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(0, "total"));
  EXPECT_EQ("filesystem:chrome-extension://extension_a/test/a",
            GetEventString(0, "fileUrl"));

  // Check event for extension_b.
  EXPECT_EQ("in_progress", GetEventString(1, "transferState"));
  EXPECT_EQ(0.0f, GetEventDouble(1, "processed"));
  EXPECT_EQ(100.0f, GetEventDouble(1, "total"));
  EXPECT_EQ("filesystem:chrome-extension://extension_b/test/a",
            GetEventString(1, "fileUrl"));
}

}  // namespace
}  // namespace file_manager
