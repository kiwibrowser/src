// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/skia_paint_canvas.h"
#include "components/services/pdf_compositor/pdf_compositor_service.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "components/services/pdf_compositor/public/interfaces/pdf_compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/src/utils/SkMultiPictureDocument.h"

namespace printing {

// In order to test PdfCompositorService, this class overrides PrepareToStart()
// to do nothing. So the test discardable memory allocator set up by
// PdfCompositorServiceTest will be used. Also checks for the service setup are
// skipped since we don't have those setups in unit tests.
class PdfCompositorTestService : public printing::PdfCompositorService {
 public:
  explicit PdfCompositorTestService(const std::string& creator)
      : PdfCompositorService(creator) {}
  ~PdfCompositorTestService() override {}

  // PdfCompositorService:
  void PrepareToStart() override {}
};

class PdfServiceTestClient : public service_manager::test::ServiceTestClient,
                             public service_manager::mojom::ServiceFactory {
 public:
  explicit PdfServiceTestClient(service_manager::test::ServiceTest* test)
      : service_manager::test::ServiceTestClient(test) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&PdfServiceTestClient::Create, base::Unretained(this)));
  }
  ~PdfServiceTestClient() override {}

  // service_manager::Service
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  // service_manager::mojom::ServiceFactory
  void CreateService(
      service_manager::mojom::ServiceRequest request,
      const std::string& name,
      service_manager::mojom::PIDReceiverPtr pid_receiver) override {
    if (!name.compare(mojom::kServiceName)) {
      service_context_ = std::make_unique<service_manager::ServiceContext>(
          std::make_unique<PdfCompositorTestService>("pdf_compositor_unittest"),
          std::move(request));
    }
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  std::unique_ptr<service_manager::ServiceContext> service_context_;
};

class PdfCompositorServiceTest : public service_manager::test::ServiceTest {
 public:
  PdfCompositorServiceTest() : ServiceTest("pdf_compositor_service_unittest") {}
  ~PdfCompositorServiceTest() override {}

  MOCK_METHOD1(CallbackOnCompositeSuccess,
               void(const base::ReadOnlySharedMemoryRegion&));
  MOCK_METHOD1(CallbackOnCompositeStatus, void(mojom::PdfCompositor::Status));
  void OnCompositeToPdfCallback(mojom::PdfCompositor::Status status,
                                base::ReadOnlySharedMemoryRegion region) {
    if (status == mojom::PdfCompositor::Status::SUCCESS)
      CallbackOnCompositeSuccess(region);
    else
      CallbackOnCompositeStatus(status);
    run_loop_->Quit();
  }

  MOCK_METHOD0(ConnectionClosed, void());

 protected:
  // service_manager::test::ServiceTest:
  void SetUp() override {
    base::DiscardableMemoryAllocator::SetInstance(
        &discardable_memory_allocator_);
    ServiceTest::SetUp();

    ASSERT_FALSE(compositor_);
    connector()->BindInterface(mojom::kServiceName, &compositor_);
    ASSERT_TRUE(compositor_);

    run_loop_ = std::make_unique<base::RunLoop>();
  }

  void TearDown() override {
    // Clean up
    compositor_.reset();
    base::DiscardableMemoryAllocator::SetInstance(nullptr);
  }

  std::unique_ptr<service_manager::Service> CreateService() override {
    return std::make_unique<PdfServiceTestClient>(this);
  }

  mojo::ScopedSharedBufferHandle CreateMSKP() {
    SkDynamicMemoryWStream stream;
    sk_sp<SkDocument> doc = SkMakeMultiPictureDocument(&stream);
    cc::SkiaPaintCanvas canvas(doc->beginPage(800, 600));
    SkRect rect = SkRect::MakeXYWH(10, 10, 250, 250);
    cc::PaintFlags flags;
    flags.setAntiAlias(false);
    flags.setColor(SK_ColorRED);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas.drawRect(rect, flags);
    doc->endPage();
    doc->close();

    size_t len = stream.bytesWritten();
    base::MappedReadOnlyRegion memory =
        base::ReadOnlySharedMemoryRegion::Create(len);
    CHECK(memory.IsValid());
    stream.copyTo(memory.mapping.memory());
    return mojo::WrapReadOnlySharedMemoryRegion(std::move(memory.region));
  }

  void CallCompositorWithSuccess(mojom::PdfCompositorPtr ptr) {
    static constexpr uint64_t kFrameGuid = 1234;
    auto handle = CreateMSKP();
    ASSERT_TRUE(handle->is_valid());
    EXPECT_CALL(*this, CallbackOnCompositeSuccess(testing::_)).Times(1);
    ptr->CompositeDocumentToPdf(
        kFrameGuid, std::move(handle), ContentToFrameMap(),
        base::BindOnce(&PdfCompositorServiceTest::OnCompositeToPdfCallback,
                       base::Unretained(this)));
    run_loop_->Run();
  }

  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::PdfCompositorPtr compositor_;
  base::TestDiscardableMemoryAllocator discardable_memory_allocator_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PdfCompositorServiceTest);
};

// Test callback function is called on error conditions in service.
TEST_F(PdfCompositorServiceTest, InvokeCallbackOnContentError) {
  EXPECT_CALL(*this, CallbackOnCompositeStatus(
                         mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR))
      .Times(1);
  compositor_->CompositeDocumentToPdf(
      5u, mojo::SharedBufferHandle::Create(10), ContentToFrameMap(),
      base::BindOnce(&PdfCompositorServiceTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  run_loop_->Run();
}

// Test callback function is called upon success.
TEST_F(PdfCompositorServiceTest, InvokeCallbackOnSuccess) {
  CallCompositorWithSuccess(std::move(compositor_));
}

// Test coexistence of multiple service instances.
TEST_F(PdfCompositorServiceTest, MultipleServiceInstances) {
  // One service can bind multiple interfaces.
  mojom::PdfCompositorPtr another_compositor;
  ASSERT_FALSE(another_compositor);
  connector()->BindInterface(mojom::kServiceName, &another_compositor);
  ASSERT_TRUE(another_compositor);
  ASSERT_NE(compositor_.get(), another_compositor.get());

  // Terminating one interface won't affect another.
  compositor_.reset();
  CallCompositorWithSuccess(std::move(another_compositor));
}

// Test data structures and content of multiple service instances
// are independent from each other.
TEST_F(PdfCompositorServiceTest, IndependentServiceInstances) {
  // Create a new connection 2.
  mojom::PdfCompositorPtr compositor2;
  ASSERT_FALSE(compositor2);
  connector()->BindInterface(mojom::kServiceName, &compositor2);
  ASSERT_TRUE(compositor2);

  // In original connection, add frame 4 with content 2 referring
  // to subframe 1.
  compositor_->AddSubframeContent(1u, CreateMSKP(), ContentToFrameMap());

  // Original connection can use this subframe 1.
  EXPECT_CALL(*this, CallbackOnCompositeSuccess(testing::_)).Times(1);
  ContentToFrameMap subframe_content_map;
  subframe_content_map[2u] = 1u;

  compositor_->CompositeDocumentToPdf(
      4u, CreateMSKP(), std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorServiceTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  run_loop_->Run();
  testing::Mock::VerifyAndClearExpectations(this);

  // Connection 2 doesn't know about subframe 1.
  subframe_content_map.clear();
  subframe_content_map[2u] = 1u;
  EXPECT_CALL(*this, CallbackOnCompositeSuccess(testing::_)).Times(0);
  compositor2->CompositeDocumentToPdf(
      4u, CreateMSKP(), std::move(subframe_content_map),
      base::BindOnce(&PdfCompositorServiceTest::OnCompositeToPdfCallback,
                     base::Unretained(this)));
  testing::Mock::VerifyAndClearExpectations(this);

  // Add info about subframe 1 to connection 2 so it can use it.
  EXPECT_CALL(*this, CallbackOnCompositeSuccess(testing::_)).Times(1);
  // Add subframe 1's content.
  // Now all content needed for previous request is ready.
  compositor2->AddSubframeContent(1u, CreateMSKP(), ContentToFrameMap());
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();
}

}  // namespace printing
