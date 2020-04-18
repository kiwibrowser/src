// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/controller/oom_intervention_impl.h"

#include <unistd.h>

#include "base/files/file_util.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/common/oom_intervention/oom_intervention_types.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

namespace {

class MockOomInterventionHost : public mojom::blink::OomInterventionHost {
 public:
  MockOomInterventionHost(mojom::blink::OomInterventionHostRequest request)
      : binding_(this, std::move(request)) {}
  ~MockOomInterventionHost() override = default;

  void OnHighMemoryUsage(bool intervention_triggered) override {}

 private:
  mojo::Binding<mojom::blink::OomInterventionHost> binding_;
};

}  // namespace

class OomInterventionImplTest : public testing::Test {
 public:
  uint64_t MockMemoryWorkloadCalculator() { return memory_workload_; }

 protected:
  uint64_t memory_workload_ = 0;
  FrameTestHelpers::WebViewHelper web_view_helper_;
};

TEST_F(OomInterventionImplTest, DetectedAndDeclined) {
  WebViewImpl* web_view = web_view_helper_.InitializeAndLoad("about::blank");
  Page* page = web_view->MainFrameImpl()->GetFrame()->GetPage();
  EXPECT_FALSE(page->Paused());

  auto intervention = std::make_unique<OomInterventionImpl>(
      WTF::BindRepeating(&OomInterventionImplTest::MockMemoryWorkloadCalculator,
                         WTF::Unretained(this)));
  EXPECT_FALSE(page->Paused());

  // Assign an arbitrary threshold and report workload bigger than the
  // threshold.
  uint64_t threshold = 80;
  intervention->memory_workload_threshold_ = threshold;
  memory_workload_ = threshold + 1;

  mojom::blink::OomInterventionHostPtr host_ptr;
  MockOomInterventionHost mock_host(mojo::MakeRequest(&host_ptr));
  base::UnsafeSharedMemoryRegion shm =
      base::UnsafeSharedMemoryRegion::Create(sizeof(OomInterventionMetrics));
  intervention->StartDetection(std::move(host_ptr), std::move(shm), threshold,
                               true /*trigger_intervention*/);
  test::RunDelayedTasks(TimeDelta::FromSeconds(1));
  EXPECT_TRUE(page->Paused());

  intervention.reset();
  EXPECT_FALSE(page->Paused());
}

TEST_F(OomInterventionImplTest, CalculatePMFAndSwap) {
  const char kStatmFile[] = "100 40 25 0 0";
  const char kStatusFile[] =
      "First:  1\n Second: 2 kB\nVmSwap: 10 kB \n Third: 10 kB\n Last: 8";
  base::FilePath statm_path;
  EXPECT_TRUE(base::CreateTemporaryFile(&statm_path));
  EXPECT_EQ(static_cast<int>(sizeof(kStatmFile)),
            base::WriteFile(statm_path, kStatmFile, sizeof(kStatmFile)));
  base::File statm_file(statm_path,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  base::FilePath status_path;
  EXPECT_TRUE(base::CreateTemporaryFile(&status_path));
  EXPECT_EQ(static_cast<int>(sizeof(kStatusFile)),
            base::WriteFile(status_path, kStatusFile, sizeof(kStatusFile)));
  base::File status_file(status_path,
                         base::File::FLAG_OPEN | base::File::FLAG_READ);

  auto intervention = std::make_unique<OomInterventionImpl>(
      WTF::BindRepeating(&OomInterventionImplTest::MockMemoryWorkloadCalculator,
                         WTF::Unretained(this)));
  intervention->statm_fd_.reset(statm_file.TakePlatformFile());
  intervention->status_fd_.reset(status_file.TakePlatformFile());

  mojom::blink::OomInterventionHostPtr host_ptr;
  MockOomInterventionHost mock_host(mojo::MakeRequest(&host_ptr));
  base::UnsafeSharedMemoryRegion shm =
      base::UnsafeSharedMemoryRegion::Create(sizeof(OomInterventionMetrics));
  uint64_t threshold = 80;
  intervention->memory_workload_threshold_ = threshold;
  memory_workload_ = threshold - 1;
  intervention->StartDetection(std::move(host_ptr), std::move(shm), threshold,
                               false /*trigger_intervention*/);

  intervention->Check(nullptr);
  OomInterventionMetrics* metrics = static_cast<OomInterventionMetrics*>(
      intervention->shared_metrics_buffer_.memory());
  uint64_t swap_kb = 10;
  uint64_t pmf_kb = (40 - 25) * getpagesize() / 1024 + swap_kb;
  EXPECT_EQ(pmf_kb, metrics->current_private_footprint_kb);
  EXPECT_EQ(swap_kb, metrics->current_swap_kb);
}

}  // namespace blink
