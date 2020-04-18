// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_registration_notifier.h"

#include <stdint.h>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom.h"

namespace content {
namespace {

const char kPrimaryUniqueId[] = "7e57ab1e-c0de-a150-ca75-1e75f005ba11";
const char kSecondaryUniqueId[] = "bb48a9fb-c21f-4c2d-a9ae-58bd48a9fb53";

constexpr uint64_t kDownloadTotal = 1;
constexpr uint64_t kDownloaded = 2;

class TestRegistrationObserver
    : public blink::mojom::BackgroundFetchRegistrationObserver {
 public:
  struct ProgressUpdate {
    ProgressUpdate(uint64_t upload_total,
                   uint64_t uploaded,
                   uint64_t download_total,
                   uint64_t downloaded)
        : upload_total(upload_total),
          uploaded(uploaded),
          download_total(download_total),
          downloaded(downloaded) {}

    uint64_t upload_total = 0;
    uint64_t uploaded = 0;
    uint64_t download_total = 0;
    uint64_t downloaded = 0;
  };

  TestRegistrationObserver() : binding_(this) {}
  ~TestRegistrationObserver() override = default;

  // Closes the bindings associated with this observer.
  void Close() { binding_.Close(); }

  // Returns an InterfacePtr to this observer.
  blink::mojom::BackgroundFetchRegistrationObserverPtr GetPtr() {
    blink::mojom::BackgroundFetchRegistrationObserverPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // Returns the vector of progress updates received by this observer.
  const std::vector<ProgressUpdate>& progress_updates() const {
    return progress_updates_;
  }

  // blink::mojom::BackgroundFetchRegistrationObserver implementation.
  void OnProgress(uint64_t upload_total,
                  uint64_t uploaded,
                  uint64_t download_total,
                  uint64_t downloaded) override {
    progress_updates_.emplace_back(upload_total, uploaded, download_total,
                                   downloaded);
  }

 private:
  std::vector<ProgressUpdate> progress_updates_;
  mojo::Binding<blink::mojom::BackgroundFetchRegistrationObserver> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestRegistrationObserver);
};

class BackgroundFetchRegistrationNotifierTest : public ::testing::Test {
 public:
  BackgroundFetchRegistrationNotifierTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        handle_(task_runner_),
        notifier_(std::make_unique<BackgroundFetchRegistrationNotifier>()) {}

  ~BackgroundFetchRegistrationNotifierTest() override = default;

  // Notifies all observers for the |unique_id| of the made progress, and waits
  // until the task runner managing the Mojo connection has finished.
  void Notify(const std::string& unique_id,
              uint64_t download_total,
              uint64_t downloaded) {
    notifier_->Notify(unique_id, download_total, downloaded);
    task_runner_->RunUntilIdle();
  }

  void CollectGarbage() { garbage_collected_ = true; }

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle handle_;

  std::unique_ptr<BackgroundFetchRegistrationNotifier> notifier_;

  bool garbage_collected_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchRegistrationNotifierTest);
};

TEST_F(BackgroundFetchRegistrationNotifierTest, NotifySingleObserver) {
  auto observer = std::make_unique<TestRegistrationObserver>();

  notifier_->AddObserver(kPrimaryUniqueId, observer->GetPtr());
  ASSERT_EQ(observer->progress_updates().size(), 0u);

  Notify(kPrimaryUniqueId, kDownloadTotal, kDownloaded);

  ASSERT_EQ(observer->progress_updates().size(), 1u);

  auto& update = observer->progress_updates()[0];
  // TODO(crbug.com/774054): Uploads are not yet supported.
  EXPECT_EQ(update.upload_total, 0u);
  EXPECT_EQ(update.uploaded, 0u);
  EXPECT_EQ(update.download_total, kDownloadTotal);
  EXPECT_EQ(update.downloaded, kDownloaded);
}

TEST_F(BackgroundFetchRegistrationNotifierTest, NotifyMultipleObservers) {
  std::vector<std::unique_ptr<TestRegistrationObserver>> primary_observers;
  primary_observers.push_back(std::make_unique<TestRegistrationObserver>());
  primary_observers.push_back(std::make_unique<TestRegistrationObserver>());
  primary_observers.push_back(std::make_unique<TestRegistrationObserver>());

  auto secondary_observer = std::make_unique<TestRegistrationObserver>();

  for (auto& observer : primary_observers) {
    notifier_->AddObserver(kPrimaryUniqueId, observer->GetPtr());
    ASSERT_EQ(observer->progress_updates().size(), 0u);
  }

  notifier_->AddObserver(kSecondaryUniqueId, secondary_observer->GetPtr());
  ASSERT_EQ(secondary_observer->progress_updates().size(), 0u);

  // Notify the |kPrimaryUniqueId|.
  Notify(kPrimaryUniqueId, kDownloadTotal, kDownloaded);

  for (auto& observer : primary_observers) {
    ASSERT_EQ(observer->progress_updates().size(), 1u);

    auto& update = observer->progress_updates()[0];
    // TODO(crbug.com/774054): Uploads are not yet supported.
    EXPECT_EQ(update.upload_total, 0u);
    EXPECT_EQ(update.uploaded, 0u);
    EXPECT_EQ(update.download_total, kDownloadTotal);
    EXPECT_EQ(update.downloaded, kDownloaded);
  }

  // The observer for |kSecondaryUniqueId| should not have been notified.
  ASSERT_EQ(secondary_observer->progress_updates().size(), 0u);
}

TEST_F(BackgroundFetchRegistrationNotifierTest,
       NotifyFollowingObserverInitiatedRemoval) {
  auto observer = std::make_unique<TestRegistrationObserver>();

  notifier_->AddObserver(kPrimaryUniqueId, observer->GetPtr());
  ASSERT_EQ(observer->progress_updates().size(), 0u);

  Notify(kPrimaryUniqueId, kDownloadTotal, kDownloaded);

  ASSERT_EQ(observer->progress_updates().size(), 1u);

  // Closes the binding as would be done from the renderer process.
  observer->Close();

  Notify(kPrimaryUniqueId, kDownloadTotal, kDownloaded);

  // The observers for |kPrimaryUniqueId| were removed, so no second update
  // should have been received by the |observer|.
  ASSERT_EQ(observer->progress_updates().size(), 1u);
}

TEST_F(BackgroundFetchRegistrationNotifierTest, NotifyWithoutObservers) {
  auto observer = std::make_unique<TestRegistrationObserver>();

  notifier_->AddObserver(kPrimaryUniqueId, observer->GetPtr());
  ASSERT_EQ(observer->progress_updates().size(), 0u);

  Notify(kSecondaryUniqueId, kDownloadTotal, kDownloaded);

  // Because the notification was for |kSecondaryUniqueId|, no progress updates
  // should be received by the |observer|.
  EXPECT_EQ(observer->progress_updates().size(), 0u);
}

TEST_F(BackgroundFetchRegistrationNotifierTest,
       AddGarbageCollectionCallback_NoObservers) {
  notifier_->AddGarbageCollectionCallback(
      kPrimaryUniqueId,
      base::BindOnce(&BackgroundFetchRegistrationNotifierTest::CollectGarbage,
                     base::Unretained(this)));

  ASSERT_TRUE(garbage_collected_)
      << "Garbage should be collected when there are no observers";
}

TEST_F(BackgroundFetchRegistrationNotifierTest,
       AddGarbageCollectionCallback_OneObserver) {
  auto observer = std::make_unique<TestRegistrationObserver>();

  auto foo = observer->GetPtr();
  notifier_->AddObserver(kPrimaryUniqueId, std::move(foo));
  notifier_->AddGarbageCollectionCallback(
      kPrimaryUniqueId,
      base::BindOnce(&BackgroundFetchRegistrationNotifierTest::CollectGarbage,
                     base::Unretained(this)));

  ASSERT_FALSE(garbage_collected_)
      << "Garbage should not be collected when there is an observer";

  observer->Close();

  // TODO(crbug.com/777764): Add this back when non-exceptional binding closures
  // are detected properly.
  // ASSERT_TRUE(garbage_collected_) << "Garbage should be collected when the
  //                                    "observer is closed.";
}

}  // namespace
}  // namespace content
