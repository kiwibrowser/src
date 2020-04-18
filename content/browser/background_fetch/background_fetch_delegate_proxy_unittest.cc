// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_delegate_proxy.h"

#include <set>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "content/browser/background_fetch/background_fetch_test_base.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "content/public/browser/background_fetch_description.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

const char kExampleUniqueId[] = "7e57ab1e-c0de-a150-ca75-1e75f005ba11";
const char kExampleUniqueId2[] = "17467386-60b4-4c5b-b66c-aabf793fd39b";
const int kIconDisplaySize = 192;

class FakeBackgroundFetchDelegate : public BackgroundFetchDelegate {
 public:
  FakeBackgroundFetchDelegate() {}

  // BackgroundFetchDelegate implementation:
  void GetIconDisplaySize(
      BackgroundFetchDelegate::GetIconDisplaySizeCallback callback) override {
    std::move(callback).Run(gfx::Size(kIconDisplaySize, kIconDisplaySize));
  }
  void CreateDownloadJob(
      std::unique_ptr<BackgroundFetchDescription> fetch_description) override {}
  void DownloadUrl(const std::string& job_unique_id,
                   const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override {
    if (!client())
      return;

    download_guid_to_job_id_map_[guid] = job_unique_id;

    auto response = std::make_unique<BackgroundFetchResponse>(
        std::vector<GURL>({url}),
        base::MakeRefCounted<net::HttpResponseHeaders>("200 OK"));

    client()->OnDownloadStarted(job_unique_id, guid, std::move(response));
    if (complete_downloads_) {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(&FakeBackgroundFetchDelegate::CompleteDownload,
                         base::Unretained(this), job_unique_id, guid));
    }
  }
  void Abort(const std::string& job_unique_id) override {
    aborted_jobs_.insert(job_unique_id);
  }

  void set_complete_downloads(bool complete_downloads) {
    complete_downloads_ = complete_downloads;
  }

 private:
  void CompleteDownload(const std::string& job_unique_id,
                        const std::string& guid) {
    if (!client())
      return;

    if (aborted_jobs_.count(download_guid_to_job_id_map_[guid]))
      return;

    client()->OnDownloadComplete(job_unique_id, guid,
                                 std::make_unique<BackgroundFetchResult>(
                                     base::Time::Now(), base::FilePath(), 10u));
  }

  std::set<std::string> aborted_jobs_;
  std::map<std::string, std::string> download_guid_to_job_id_map_;
  bool complete_downloads_ = true;
};

class FakeController : public BackgroundFetchDelegateProxy::Controller {
 public:
  FakeController() : weak_ptr_factory_(this) {}

  void DidStartRequest(
      const scoped_refptr<BackgroundFetchRequestInfo>& request) override {
    request_started_ = true;
  }

  void DidUpdateRequest(
      const scoped_refptr<BackgroundFetchRequestInfo>& request,
      uint64_t bytes_downloaded) override {}

  void DidCompleteRequest(
      const scoped_refptr<BackgroundFetchRequestInfo>& request) override {
    request_completed_ = true;
  }

  void Abort(BackgroundFetchReasonToAbort reason_to_abort) override {}

  bool request_started_ = false;
  bool request_completed_ = false;
  base::WeakPtrFactory<FakeController> weak_ptr_factory_;
};

class BackgroundFetchDelegateProxyTest : public BackgroundFetchTestBase {
 public:
  BackgroundFetchDelegateProxyTest() : delegate_proxy_(&delegate_) {}
  void DidGetIconDisplaySize(base::Closure quit_closure,
                             gfx::Size* out_display_size,
                             const gfx::Size& display_size) {
    DCHECK(out_display_size);
    *out_display_size = display_size;
    std::move(quit_closure).Run();
  }

 protected:
  FakeBackgroundFetchDelegate delegate_;
  BackgroundFetchDelegateProxy delegate_proxy_;
};

scoped_refptr<BackgroundFetchRequestInfo> CreateRequestInfo(
    int request_index,
    const ServiceWorkerFetchRequest& fetch_request) {
  auto request = base::MakeRefCounted<BackgroundFetchRequestInfo>(
      request_index, fetch_request);
  request->InitializeDownloadGuid();
  return request;
}

}  // namespace

TEST_F(BackgroundFetchDelegateProxyTest, SetDelegate) {
  EXPECT_TRUE(delegate_.client().get());
}

TEST_F(BackgroundFetchDelegateProxyTest, StartRequest) {
  FakeController controller;
  ServiceWorkerFetchRequest fetch_request;
  auto request = CreateRequestInfo(0 /* request_index */, fetch_request);

  EXPECT_FALSE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);

  auto fetch_description = std::make_unique<BackgroundFetchDescription>(
      kExampleUniqueId, "Job 1", url::Origin(), SkBitmap(),
      0 /* completed_parts */, 1 /* total_parts */,
      0 /* completed_parts_size */, 0 /* total_parts_size */,
      std::vector<std::string>());
  delegate_proxy_.CreateDownloadJob(controller.weak_ptr_factory_.GetWeakPtr(),
                                    std::move(fetch_description));

  delegate_proxy_.StartRequest(kExampleUniqueId, url::Origin(), request);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(controller.request_started_);
  EXPECT_TRUE(controller.request_completed_);
}

TEST_F(BackgroundFetchDelegateProxyTest, StartRequest_NotCompleted) {
  FakeController controller;
  ServiceWorkerFetchRequest fetch_request;
  auto request = CreateRequestInfo(0 /* request_index */, fetch_request);

  EXPECT_FALSE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);

  delegate_.set_complete_downloads(false);
  auto fetch_description = std::make_unique<BackgroundFetchDescription>(
      kExampleUniqueId, "Job 1", url::Origin(), SkBitmap(),
      0 /* completed_parts */, 1 /* total_parts */,
      0 /* completed_parts_size */, 0 /* total_parts_size */,
      std::vector<std::string>());
  delegate_proxy_.CreateDownloadJob(controller.weak_ptr_factory_.GetWeakPtr(),
                                    std::move(fetch_description));

  delegate_proxy_.StartRequest(kExampleUniqueId, url::Origin(), request);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);
}

TEST_F(BackgroundFetchDelegateProxyTest, Abort) {
  FakeController controller;
  FakeController controller2;
  ServiceWorkerFetchRequest fetch_request;
  ServiceWorkerFetchRequest fetch_request2;
  auto request = CreateRequestInfo(0 /* request_index */, fetch_request);
  auto request2 = CreateRequestInfo(1 /* request_index */, fetch_request2);

  EXPECT_FALSE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);
  EXPECT_FALSE(controller2.request_started_);
  EXPECT_FALSE(controller2.request_completed_);

  auto fetch_description1 = std::make_unique<BackgroundFetchDescription>(
      kExampleUniqueId, "Job 1", url::Origin(), SkBitmap(),
      0 /* completed_parts */, 1 /* total_parts */,
      0 /* completed_parts_size */, 0 /* total_parts_size */,
      std::vector<std::string>());
  delegate_proxy_.CreateDownloadJob(controller.weak_ptr_factory_.GetWeakPtr(),
                                    std::move(fetch_description1));

  auto fetch_description2 = std::make_unique<BackgroundFetchDescription>(
      kExampleUniqueId2, "Job 2", url::Origin(), SkBitmap(),
      0 /* completed_parts */, 1 /* total_parts */,
      0 /* completed_parts_size */, 0 /* total_parts_size */,
      std::vector<std::string>());
  delegate_proxy_.CreateDownloadJob(controller2.weak_ptr_factory_.GetWeakPtr(),
                                    std::move(fetch_description2));

  delegate_proxy_.StartRequest(kExampleUniqueId, url::Origin(), request);
  delegate_proxy_.StartRequest(kExampleUniqueId2, url::Origin(), request2);
  delegate_proxy_.Abort(kExampleUniqueId);
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(controller.request_started_) << "Aborted job started";
  EXPECT_FALSE(controller.request_completed_) << "Aborted job completed";
  EXPECT_TRUE(controller2.request_started_) << "Normal job did not start";
  EXPECT_TRUE(controller2.request_completed_) << "Normal job did not complete";
}

TEST_F(BackgroundFetchDelegateProxyTest, GetIconDisplaySize) {
  gfx::Size out_display_size;
  base::RunLoop run_loop;
  delegate_proxy_.GetIconDisplaySize(base::BindOnce(
      &BackgroundFetchDelegateProxyTest::DidGetIconDisplaySize,
      base::Unretained(this), run_loop.QuitClosure(), &out_display_size));
  run_loop.Run();
  EXPECT_EQ(out_display_size.width(), kIconDisplaySize);
  EXPECT_EQ(out_display_size.height(), kIconDisplaySize);
}

}  // namespace content
