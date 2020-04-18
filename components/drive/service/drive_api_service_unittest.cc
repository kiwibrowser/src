// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/service/drive_api_service.h"
#include "base/test/test_simple_task_runner.h"
#include "google_apis/drive/dummy_auth_service.h"
#include "google_apis/drive/request_sender.h"
#include "google_apis/drive/test_util.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace {
const char kTestUserAgent[] = "test-user-agent";
}

class TestAuthService : public google_apis::DummyAuthService {
 public:
  void StartAuthentication(
      const google_apis::AuthStatusCallback& callback) override {
    callback_ = callback;
  }

  bool HasAccessToken() const override { return false; }

  void SendHttpError() {
    ASSERT_FALSE(callback_.is_null());
    callback_.Run(google_apis::HTTP_UNAUTHORIZED, "");
  }

 private:
  google_apis::AuthStatusCallback callback_;
};

TEST(DriveAPIServiceTest, BatchRequestConfiguratorWithAuthFailure) {
  const GURL test_base_url("http://localhost/");
  google_apis::DriveApiUrlGenerator url_generator(test_base_url, test_base_url,
      google_apis::TEAM_DRIVES_INTEGRATION_DISABLED);
  scoped_refptr<base::TestSimpleTaskRunner> task_runner =
      new base::TestSimpleTaskRunner();
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(task_runner.get());
  google_apis::RequestSender sender(
      new TestAuthService, request_context_getter.get(), task_runner.get(),
      kTestUserAgent, TRAFFIC_ANNOTATION_FOR_TESTS);
  std::unique_ptr<google_apis::drive::BatchUploadRequest> request =
      std::make_unique<google_apis::drive::BatchUploadRequest>(&sender,
                                                               url_generator);
  google_apis::drive::BatchUploadRequest* request_ptr = request.get();
  sender.StartRequestWithAuthRetry(std::move(request));
  BatchRequestConfigurator configurator(
      request_ptr->GetWeakPtrAsBatchUploadRequest(), task_runner.get(),
      url_generator, google_apis::CancelCallback());
  static_cast<TestAuthService*>(sender.auth_service())->SendHttpError();

  {
    google_apis::DriveApiErrorCode error = google_apis::HTTP_SUCCESS;
    std::unique_ptr<google_apis::FileResource> file_resource;
    configurator.MultipartUploadNewFile(
        "text/plain", 10, "", "title",
        base::FilePath(FILE_PATH_LITERAL("/file")), UploadNewFileOptions(),
        google_apis::test_util::CreateCopyResultCallback(&error,
                                                         &file_resource),
        google_apis::ProgressCallback());
    EXPECT_EQ(google_apis::DRIVE_OTHER_ERROR, error);
  }
  {
    google_apis::DriveApiErrorCode error = google_apis::HTTP_SUCCESS;
    std::unique_ptr<google_apis::FileResource> file_resource;
    configurator.MultipartUploadExistingFile(
        "text/plain", 10, "resource_id",
        base::FilePath(FILE_PATH_LITERAL("/file")), UploadExistingFileOptions(),
        google_apis::test_util::CreateCopyResultCallback(&error,
                                                         &file_resource),
        google_apis::ProgressCallback());
    EXPECT_EQ(google_apis::DRIVE_OTHER_ERROR, error);
  }
}

}  // namespace drive
