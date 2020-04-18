// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/canvas_resource.h"

#include "base/run_loop.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_gles2_interface.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_web_graphics_context_3d_provider.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/skia/include/core/SkSurface.h"

using testing::_;
using testing::Pointee;
using testing::Return;
using testing::SetArrayArgument;
using testing::Test;

namespace blink {

class MockGLES2InterfaceWithMailboxSupport : public FakeGLES2Interface {
 public:
  MOCK_METHOD2(ProduceTextureDirectCHROMIUM, void(GLuint, const GLbyte*));
  MOCK_METHOD1(GenMailboxCHROMIUM, void(GLbyte*));
  MOCK_METHOD1(GenUnverifiedSyncTokenCHROMIUM, void(GLbyte*));
  MOCK_METHOD4(CreateImageCHROMIUM,
               GLuint(ClientBuffer, GLsizei, GLsizei, GLenum));
  MOCK_METHOD2(BindTexture, void(GLenum, GLuint));
};

class FakeCanvasResourcePlatformSupport : public TestingPlatformSupport {
 public:
  void RunUntilStop() const { base::RunLoop().Run(); }

  void StopRunning() const { base::RunLoop().Quit(); }

 private:
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override {
    return &test_gpu_memory_buffer_manager_;
  }

  viz::TestGpuMemoryBufferManager test_gpu_memory_buffer_manager_;
};

class CanvasResourceTest : public Test {
 public:
  void SetUp() override {
    // Install our mock GL context so that it gets served by SharedGpuContext.
    auto factory = [](FakeGLES2Interface* gl, bool* gpu_compositing_disabled)
        -> std::unique_ptr<WebGraphicsContext3DProvider> {
      *gpu_compositing_disabled = false;
      return std::make_unique<FakeWebGraphicsContext3DProvider>(gl);
    };
    SharedGpuContext::SetContextProviderFactoryForTesting(
        WTF::BindRepeating(factory, WTF::Unretained(&gl_)));
    context_provider_wrapper_ = SharedGpuContext::ContextProviderWrapper();
  }

  void TearDown() override { SharedGpuContext::ResetForTesting(); }

  GrContext* GetGrContext() {
    return context_provider_wrapper_->ContextProvider()->GetGrContext();
  }

 protected:
  MockGLES2InterfaceWithMailboxSupport gl_;
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
};

gpu::SyncToken GenTestSyncToken(int id) {
  gpu::SyncToken token;
  token.Set(gpu::CommandBufferNamespace::GPU_IO,
            gpu::CommandBufferId::FromUnsafeValue(id), 1);
  return token;
}

TEST_F(CanvasResourceTest, SkiaResourceNoMailboxLeak) {
  testing::InSequence s;
  SkImageInfo image_info =
      SkImageInfo::MakeN32(10, 10, kPremul_SkAlphaType, nullptr);
  sk_sp<SkSurface> surface =
      SkSurface::MakeRenderTarget(GetGrContext(), SkBudgeted::kYes, image_info);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_TRUE(!!context_provider_wrapper_);
  scoped_refptr<CanvasResource> resource = CanvasResourceBitmap::Create(
      StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                context_provider_wrapper_),
      nullptr, kLow_SkFilterQuality, CanvasColorParams());

  testing::Mock::VerifyAndClearExpectations(&gl_);

  gpu::Mailbox test_mailbox;
  test_mailbox.name[0] = 1;
  EXPECT_CALL(gl_, GenMailboxCHROMIUM(_))
      .WillOnce(SetArrayArgument<0>(
          test_mailbox.name, test_mailbox.name + GL_MAILBOX_SIZE_CHROMIUM));
  EXPECT_CALL(gl_, BindTexture(GL_TEXTURE_2D, _)).Times(2);
  EXPECT_CALL(gl_, ProduceTextureDirectCHROMIUM(_, _));
  EXPECT_CALL(gl_, GenUnverifiedSyncTokenCHROMIUM(_));
  resource->GetOrCreateGpuMailbox();

  testing::Mock::VerifyAndClearExpectations(&gl_);

  // No expected call to DeleteTextures becaus skia recycles
  // No expected call to ProduceTextureDirectCHROMIUM(0, *) because
  // mailbox is cached by GraphicsContext3DUtils and therefore does not need to
  // be orphaned.
  EXPECT_CALL(gl_, ProduceTextureDirectCHROMIUM(0, _)).Times(0);
  resource = nullptr;

  testing::Mock::VerifyAndClearExpectations(&gl_);

  // Purging skia's resource cache will finally delete the GrTexture, resulting
  // in the mailbox being orphaned via ProduceTextureDirectCHROMIUM.
  EXPECT_CALL(gl_,
              ProduceTextureDirectCHROMIUM(0, Pointee(test_mailbox.name[0])))
      .Times(0);
  GetGrContext()->performDeferredCleanup(std::chrono::milliseconds(0));

  testing::Mock::VerifyAndClearExpectations(&gl_);
}

TEST_F(CanvasResourceTest, GpuMemoryBufferSyncTokenRefresh) {
  testing::InSequence s;
  ScopedTestingPlatformSupport<FakeCanvasResourcePlatformSupport> platform;

  constexpr GLuint image_id = 1;
  EXPECT_CALL(gl_, CreateImageCHROMIUM(_, _, _, _)).WillOnce(Return(image_id));
  EXPECT_CALL(gl_, BindTexture(gpu::GetPlatformSpecificTextureTarget(), _));
  scoped_refptr<CanvasResource> resource =
      CanvasResourceGpuMemoryBuffer::Create(
          IntSize(10, 10), CanvasColorParams(),
          SharedGpuContext::ContextProviderWrapper(),
          nullptr,  // Resource provider
          kLow_SkFilterQuality);

  EXPECT_TRUE(bool(resource));

  testing::Mock::VerifyAndClearExpectations(&gl_);

  gpu::Mailbox test_mailbox;
  test_mailbox.name[0] = 1;
  EXPECT_CALL(gl_, GenMailboxCHROMIUM(_))
      .WillOnce(SetArrayArgument<0>(
          test_mailbox.name, test_mailbox.name + GL_MAILBOX_SIZE_CHROMIUM));
  EXPECT_CALL(gl_, ProduceTextureDirectCHROMIUM(_, _));
  resource->GetOrCreateGpuMailbox();

  testing::Mock::VerifyAndClearExpectations(&gl_);

  const gpu::SyncToken reference_token1 = GenTestSyncToken(1);
  EXPECT_CALL(gl_, GenUnverifiedSyncTokenCHROMIUM(_))
      .WillOnce(SetArrayArgument<0>(
          reinterpret_cast<const GLbyte*>(&reference_token1),
          reinterpret_cast<const GLbyte*>(&reference_token1 + 1)));
  gpu::SyncToken actual_token = resource->GetSyncToken();
  EXPECT_EQ(actual_token, reference_token1);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  // Grabbing the mailbox again without making any changes must not result in
  // a sync token refresh
  EXPECT_CALL(gl_, GenMailboxCHROMIUM(_)).Times(0);
  EXPECT_CALL(gl_, GenUnverifiedSyncTokenCHROMIUM(_)).Times(0);
  resource->GetOrCreateGpuMailbox();
  actual_token = resource->GetSyncToken();
  EXPECT_EQ(actual_token, reference_token1);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  // Grabbing the mailbox again after a content change must result in
  // a sync token refresh, but the mailbox gets re-used.
  EXPECT_CALL(gl_, GenMailboxCHROMIUM(_)).Times(0);
  const gpu::SyncToken reference_token2 = GenTestSyncToken(2);
  EXPECT_CALL(gl_, GenUnverifiedSyncTokenCHROMIUM(_))
      .WillOnce(SetArrayArgument<0>(
          reinterpret_cast<const GLbyte*>(&reference_token2),
          reinterpret_cast<const GLbyte*>(&reference_token2 + 1)));
  resource->CopyFromTexture(1,  // source texture id
                            GL_RGBA, GL_UNSIGNED_BYTE);
  resource->GetOrCreateGpuMailbox();
  actual_token = resource->GetSyncToken();
  EXPECT_EQ(actual_token, reference_token2);

  testing::Mock::VerifyAndClearExpectations(&gl_);
}

TEST_F(CanvasResourceTest, MakeAcceleratedFromAcceleratedResourceIsNoOp) {
  ScopedTestingPlatformSupport<FakeCanvasResourcePlatformSupport> platform;

  SkImageInfo image_info =
      SkImageInfo::MakeN32(10, 10, kPremul_SkAlphaType, nullptr);
  sk_sp<SkSurface> surface =
      SkSurface::MakeRenderTarget(GetGrContext(), SkBudgeted::kYes, image_info);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_TRUE(!!context_provider_wrapper_);
  scoped_refptr<CanvasResource> resource = CanvasResourceBitmap::Create(
      StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                context_provider_wrapper_),
      nullptr, kLow_SkFilterQuality, CanvasColorParams());

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_TRUE(resource->IsAccelerated());
  scoped_refptr<CanvasResource> new_resource =
      resource->MakeAccelerated(context_provider_wrapper_);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_EQ(new_resource.get(), resource.get());
  EXPECT_TRUE(new_resource->IsAccelerated());
}

TEST_F(CanvasResourceTest, MakeAcceleratedFromRasterResource) {
  ScopedTestingPlatformSupport<FakeCanvasResourcePlatformSupport> platform;

  SkImageInfo image_info =
      SkImageInfo::MakeN32(10, 10, kPremul_SkAlphaType, nullptr);
  sk_sp<SkSurface> surface = SkSurface::MakeRaster(image_info);

  EXPECT_TRUE(!!context_provider_wrapper_);
  scoped_refptr<CanvasResource> resource = CanvasResourceBitmap::Create(
      StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                context_provider_wrapper_),
      nullptr, kLow_SkFilterQuality, CanvasColorParams());

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_FALSE(resource->IsAccelerated());
  scoped_refptr<CanvasResource> new_resource =
      resource->MakeAccelerated(context_provider_wrapper_);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_NE(new_resource.get(), resource.get());
  EXPECT_FALSE(resource->IsAccelerated());
  EXPECT_TRUE(new_resource->IsAccelerated());
}

TEST_F(CanvasResourceTest, MakeUnacceleratedFromUnacceleratedResourceIsNoOp) {
  ScopedTestingPlatformSupport<FakeCanvasResourcePlatformSupport> platform;

  SkImageInfo image_info =
      SkImageInfo::MakeN32(10, 10, kPremul_SkAlphaType, nullptr);
  sk_sp<SkSurface> surface = SkSurface::MakeRaster(image_info);

  scoped_refptr<CanvasResource> resource = CanvasResourceBitmap::Create(
      StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                context_provider_wrapper_),
      nullptr, kLow_SkFilterQuality, CanvasColorParams());

  EXPECT_FALSE(resource->IsAccelerated());
  scoped_refptr<CanvasResource> new_resource = resource->MakeUnaccelerated();

  EXPECT_EQ(new_resource.get(), resource.get());
  EXPECT_FALSE(new_resource->IsAccelerated());
}

TEST_F(CanvasResourceTest, MakeUnacceleratedFromAcceleratedResource) {
  ScopedTestingPlatformSupport<FakeCanvasResourcePlatformSupport> platform;

  SkImageInfo image_info =
      SkImageInfo::MakeN32(10, 10, kPremul_SkAlphaType, nullptr);
  sk_sp<SkSurface> surface =
      SkSurface::MakeRenderTarget(GetGrContext(), SkBudgeted::kYes, image_info);

  EXPECT_TRUE(!!context_provider_wrapper_);
  scoped_refptr<CanvasResource> resource = CanvasResourceBitmap::Create(
      StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                context_provider_wrapper_),
      nullptr, kLow_SkFilterQuality, CanvasColorParams());

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_TRUE(resource->IsAccelerated());
  scoped_refptr<CanvasResource> new_resource = resource->MakeUnaccelerated();

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_NE(new_resource.get(), resource.get());
  EXPECT_TRUE(resource->IsAccelerated());
  EXPECT_FALSE(new_resource->IsAccelerated());
}

}  // namespace blink
