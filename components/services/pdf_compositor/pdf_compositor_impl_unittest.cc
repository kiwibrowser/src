// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/crash/core/common/crash_key.h"
#include "components/services/pdf_compositor/pdf_compositor_impl.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace printing {

class MockPdfCompositorImpl : public PdfCompositorImpl {
 public:
  MockPdfCompositorImpl() : PdfCompositorImpl("unittest", nullptr) {}
  ~MockPdfCompositorImpl() override {}

  MOCK_METHOD2(OnFulfillRequest, void(uint64_t, int));

 protected:
  void FulfillRequest(uint64_t frame_guid,
                      base::Optional<uint32_t> page_num,
                      std::unique_ptr<base::SharedMemory> serialized_content,
                      const ContentToFrameMap& subframe_content_map,
                      CompositeToPdfCallback callback) override {
    OnFulfillRequest(frame_guid, page_num.has_value() ? page_num.value() : -1);
  }
};

class PdfCompositorImplTest : public testing::Test {
 public:
  PdfCompositorImplTest()
      : task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        run_loop_(std::make_unique<base::RunLoop>()),
        is_ready_(false) {}

  void OnIsReadyToCompositeCallback(bool is_ready) {
    is_ready_ = is_ready;
    run_loop_->Quit();
  }

  bool ResultFromCallback() {
    run_loop_->Run();
    run_loop_ = std::make_unique<base::RunLoop>();
    return is_ready_;
  }

  void OnCompositeToPdfCallback(mojom::PdfCompositor::Status status,
                                base::ReadOnlySharedMemoryRegion region) {
    // A stub for testing, no implementation.
  }

 protected:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool is_ready_;
};

class PdfCompositorImplCrashKeyTest : public PdfCompositorImplTest {
 public:
  PdfCompositorImplCrashKeyTest() {}
  ~PdfCompositorImplCrashKeyTest() override {}

  void SetUp() override {
    crash_reporter::ResetCrashKeysForTesting();
    crash_reporter::InitializeCrashKeys();
  }

  void TearDown() override { crash_reporter::ResetCrashKeysForTesting(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(PdfCompositorImplCrashKeyTest);
};

TEST_F(PdfCompositorImplTest, IsReadyToComposite) {
  PdfCompositorImpl impl("unittest", nullptr);
  // Frame 2 and 3 are painted.
  impl.AddSubframeContent(2u, mojo::SharedBufferHandle::Create(10),
                          ContentToFrameMap());
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          ContentToFrameMap());

  // Frame 1 contains content 3 which corresponds to frame 2.
  // Frame 1 should be ready as frame 2 is ready.
  ContentToFrameMap subframe_content_map;
  subframe_content_map[3u] = 2u;
  base::flat_set<uint64_t> pending_subframes;
  bool is_ready = impl.IsReadyToComposite(1u, std::move(subframe_content_map),
                                          &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());

  // If another page of frame 1 needs content 2 which corresponds to frame 3.
  // This page is ready since frame 3 was painted also.
  subframe_content_map.clear();
  subframe_content_map[2u] = 3u;
  is_ready = impl.IsReadyToComposite(1u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());

  // Frame 1 with content 1, 2 and 3 should not be ready since content 1's
  // content in frame 4 is not painted yet.
  subframe_content_map.clear();
  subframe_content_map[1u] = 4u;
  subframe_content_map[2u] = 3u;
  subframe_content_map[3u] = 2u;
  is_ready = impl.IsReadyToComposite(1u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_FALSE(is_ready);
  ASSERT_EQ(pending_subframes.size(), 1u);
  EXPECT_EQ(*pending_subframes.begin(), 4u);

  // Add content of frame 4. Now it is ready for composition.
  subframe_content_map.clear();
  subframe_content_map[1u] = 4u;
  subframe_content_map[2u] = 3u;
  subframe_content_map[3u] = 2u;
  impl.AddSubframeContent(4u, mojo::SharedBufferHandle::Create(10),
                          ContentToFrameMap());
  is_ready = impl.IsReadyToComposite(1u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());
}

TEST_F(PdfCompositorImplTest, MultiLayerDependency) {
  PdfCompositorImpl impl("unittest", nullptr);
  // Frame 3 has content 1 which refers to subframe 1.
  ContentToFrameMap subframe_content_map;
  subframe_content_map[1u] = 1u;
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));

  // Frame 5 has content 3 which refers to subframe 3.
  // Although frame 3's content is added, its subframe 1's content is not added.
  // So frame 5 is not ready.
  subframe_content_map.clear();
  subframe_content_map[3u] = 3u;
  base::flat_set<uint64_t> pending_subframes;
  bool is_ready = impl.IsReadyToComposite(5u, std::move(subframe_content_map),
                                          &pending_subframes);
  EXPECT_FALSE(is_ready);
  ASSERT_EQ(pending_subframes.size(), 1u);
  EXPECT_EQ(*pending_subframes.begin(), 1u);

  // Frame 6 is not ready either since it needs frame 5 to be ready.
  subframe_content_map.clear();
  subframe_content_map[1u] = 5u;
  is_ready = impl.IsReadyToComposite(6u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_FALSE(is_ready);
  ASSERT_EQ(pending_subframes.size(), 1u);
  EXPECT_EQ(*pending_subframes.begin(), 5u);

  // When frame 1's content is added, frame 5 is ready.
  impl.AddSubframeContent(1u, mojo::SharedBufferHandle::Create(10),
                          ContentToFrameMap());
  subframe_content_map.clear();
  subframe_content_map[3u] = 3u;
  is_ready = impl.IsReadyToComposite(5u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());

  // Add frame 5's content.
  subframe_content_map.clear();
  subframe_content_map[3u] = 3u;
  impl.AddSubframeContent(5u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));

  // Frame 6 is ready too.
  subframe_content_map.clear();
  subframe_content_map[1u] = 5u;
  is_ready = impl.IsReadyToComposite(6u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());
}

TEST_F(PdfCompositorImplTest, DependencyLoop) {
  PdfCompositorImpl impl("unittest", nullptr);
  // Frame 3 has content 1, which refers to frame 1.
  // Frame 1 has content 3, which refers to frame 3.
  ContentToFrameMap subframe_content_map;
  subframe_content_map[3u] = 3u;
  impl.AddSubframeContent(1u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));

  subframe_content_map.clear();
  subframe_content_map[1u] = 1u;
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));

  // Both frame 1 and 3 are painted, frame 5 should be ready.
  base::flat_set<uint64_t> pending_subframes;
  subframe_content_map.clear();
  subframe_content_map[1u] = 3u;
  bool is_ready = impl.IsReadyToComposite(5u, std::move(subframe_content_map),
                                          &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());

  // Frame 6 has content 7, which refers to frame 7.
  subframe_content_map.clear();
  subframe_content_map[7u] = 7u;
  impl.AddSubframeContent(6, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));
  // Frame 7 should be ready since frame 6's own content is added and it only
  // depends on frame 7.
  subframe_content_map.clear();
  subframe_content_map[6u] = 6u;
  is_ready = impl.IsReadyToComposite(7u, std::move(subframe_content_map),
                                     &pending_subframes);
  EXPECT_TRUE(is_ready);
  EXPECT_TRUE(pending_subframes.empty());
}

TEST_F(PdfCompositorImplTest, MultiRequestsBasic) {
  MockPdfCompositorImpl impl;
  // Page 0 with frame 3 has content 1, which refers to frame 8.
  // When the content is not available, the request is not fulfilled.
  ContentToFrameMap subframe_content_map;
  subframe_content_map[1u] = 8u;
  EXPECT_CALL(impl, OnFulfillRequest(testing::_, testing::_)).Times(0);
  impl.CompositePageToPdf(
      3u, 0, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  testing::Mock::VerifyAndClearExpectations(&impl);

  // When frame 8's content is ready, the previous request should be fulfilled.
  EXPECT_CALL(impl, OnFulfillRequest(3u, 0)).Times(1);
  impl.AddSubframeContent(8u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));
  testing::Mock::VerifyAndClearExpectations(&impl);

  // The following requests which only depends on frame 8 should be
  // immediately fulfilled.
  EXPECT_CALL(impl, OnFulfillRequest(3u, 1)).Times(1);
  EXPECT_CALL(impl, OnFulfillRequest(3u, -1)).Times(1);
  subframe_content_map.clear();
  subframe_content_map[1u] = 8u;
  impl.CompositePageToPdf(
      3u, 1, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));

  subframe_content_map.clear();
  subframe_content_map[1u] = 8u;
  impl.CompositeDocumentToPdf(
      3u, mojo::SharedBufferHandle::Create(10), std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
}

TEST_F(PdfCompositorImplTest, MultiRequestsOrder) {
  MockPdfCompositorImpl impl;
  // Page 0 with frame  3 has content 1, which refers to frame 8.
  // When the content is not available, the request is not fulfilled.
  ContentToFrameMap subframe_content_map;
  subframe_content_map[1u] = 8u;
  EXPECT_CALL(impl, OnFulfillRequest(testing::_, testing::_)).Times(0);
  impl.CompositePageToPdf(
      3u, 0, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));

  // The following requests which only depends on frame 8 should be
  // immediately fulfilled.
  subframe_content_map.clear();
  subframe_content_map[1u] = 8u;
  impl.CompositePageToPdf(
      3u, 1, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));

  subframe_content_map.clear();
  subframe_content_map[1u] = 8u;
  impl.CompositeDocumentToPdf(
      3u, mojo::SharedBufferHandle::Create(10), std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  testing::Mock::VerifyAndClearExpectations(&impl);

  // When frame 8's content is ready, the previous request should be
  // fulfilled.
  EXPECT_CALL(impl, OnFulfillRequest(3u, 0)).Times(1);
  EXPECT_CALL(impl, OnFulfillRequest(3u, 1)).Times(1);
  EXPECT_CALL(impl, OnFulfillRequest(3u, -1)).Times(1);
  subframe_content_map.clear();
  impl.AddSubframeContent(8u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));
}

TEST_F(PdfCompositorImplTest, MultiRequestsDepOrder) {
  MockPdfCompositorImpl impl;
  // Page 0 with frame 1 has content 1, which refers to frame
  // 2. When the content is not available, the request is not
  // fulfilled.
  EXPECT_CALL(impl, OnFulfillRequest(testing::_, testing::_)).Times(0);
  ContentToFrameMap subframe_content_map;
  subframe_content_map[1u] = 2u;
  impl.CompositePageToPdf(
      1u, 0, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));

  // Page 1 with frame 1 has content 1, which refers to frame
  // 3. When the content is not available, the request is not
  // fulfilled either.
  subframe_content_map.clear();
  subframe_content_map[1u] = 3u;
  impl.CompositePageToPdf(
      1u, 1, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  testing::Mock::VerifyAndClearExpectations(&impl);

  // When frame 3 and 2 become available, the pending requests should be
  // satisfied, thus be fulfilled in order.
  testing::Sequence order;
  EXPECT_CALL(impl, OnFulfillRequest(1, 1)).Times(1).InSequence(order);
  EXPECT_CALL(impl, OnFulfillRequest(1, 0)).Times(1).InSequence(order);
  subframe_content_map.clear();
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));
  impl.AddSubframeContent(2u, mojo::SharedBufferHandle::Create(10),
                          std::move(subframe_content_map));
}

TEST_F(PdfCompositorImplTest, NotifyUnavailableSubframe) {
  MockPdfCompositorImpl impl;
  // Page 0 with frame 3 has content 1, which refers to frame 8.
  // When the content is not available, the request is not fulfilled.
  ContentToFrameMap subframe_content_map;
  subframe_content_map[1u] = 8u;
  EXPECT_CALL(impl, OnFulfillRequest(testing::_, testing::_)).Times(0);
  impl.CompositePageToPdf(
      3u, 0, mojo::SharedBufferHandle::Create(10),
      std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorImplTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  testing::Mock::VerifyAndClearExpectations(&impl);

  // Notifies that frame 8's unavailable, the previous request should be
  // fulfilled.
  EXPECT_CALL(impl, OnFulfillRequest(3u, 0)).Times(1);
  impl.NotifyUnavailableSubframe(8u);
  testing::Mock::VerifyAndClearExpectations(&impl);
}

TEST_F(PdfCompositorImplCrashKeyTest, SetCrashKey) {
  PdfCompositorImpl impl("unittest", nullptr);
  std::string url_str("https://www.example.com/");
  GURL url(url_str);
  impl.SetWebContentsURL(url);

  EXPECT_EQ(crash_reporter::GetCrashKeyValue("main-frame-url"), url_str);
}

}  // namespace printing
