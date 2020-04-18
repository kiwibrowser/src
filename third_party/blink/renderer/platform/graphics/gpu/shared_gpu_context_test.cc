// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"

#include <memory>

#include "gpu/command_buffer/client/gles2_interface.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/canvas_2d_layer_bridge.h"
#include "third_party/blink/renderer/platform/graphics/canvas_resource_provider.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_gles2_interface.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_web_graphics_context_3d_provider.h"
#include "third_party/khronos/GLES2/gl2ext.h"

using testing::Test;

namespace blink {

namespace {

template <class GLES2InterfaceType>
class SharedGpuContextTestBase : public Test {
 public:
  void SetUp() override {
    auto factory = [](GLES2InterfaceType* gl, bool* gpu_compositing_disabled)
        -> std::unique_ptr<WebGraphicsContext3DProvider> {
      *gpu_compositing_disabled = false;
      gl->SetIsContextLost(false);
      return std::make_unique<FakeWebGraphicsContext3DProvider>(gl);
    };
    SharedGpuContext::SetContextProviderFactoryForTesting(
        WTF::BindRepeating(factory, WTF::Unretained(&gl_)));
  }

  void TearDown() override { SharedGpuContext::ResetForTesting(); }

  GLES2InterfaceType gl_;
};

class SharedGpuContextTest
    : public SharedGpuContextTestBase<FakeGLES2Interface> {};

class MailboxMockGLES2Interface : public FakeGLES2Interface {
 public:
  MOCK_METHOD1(GenMailboxCHROMIUM, void(GLbyte*));
  MOCK_METHOD1(GenSyncTokenCHROMIUM, void(GLbyte*));
  MOCK_METHOD1(GenUnverifiedSyncTokenCHROMIUM, void(GLbyte*));
};

class MailboxSharedGpuContextTest
    : public SharedGpuContextTestBase<MailboxMockGLES2Interface> {};

// Test fixure that simulate a graphics context creation failure, when using gpu
// compositing.
class BadSharedGpuContextTest : public Test {
 public:
  void SetUp() override {
    auto factory = [](bool* gpu_compositing_disabled)
        -> std::unique_ptr<WebGraphicsContext3DProvider> {
      *gpu_compositing_disabled = false;
      return nullptr;
    };
    SharedGpuContext::SetContextProviderFactoryForTesting(
        WTF::BindRepeating(factory));
  }

  void TearDown() override { SharedGpuContext::ResetForTesting(); }
};

// Test fixure that simulate not using gpu compositing.
class SoftwareCompositingTest : public Test {
 public:
  void SetUp() override {
    auto factory = [](FakeGLES2Interface* gl, bool* gpu_compositing_disabled)
        -> std::unique_ptr<WebGraphicsContext3DProvider> {
      *gpu_compositing_disabled = true;
      // Return a context anyway, to ensure that's not what the class checks
      // to determine compositing mode.
      gl->SetIsContextLost(false);
      return std::make_unique<FakeWebGraphicsContext3DProvider>(gl);
    };
    SharedGpuContext::SetContextProviderFactoryForTesting(
        WTF::BindRepeating(factory, WTF::Unretained(&gl_)));
  }

  void TearDown() override { SharedGpuContext::ResetForTesting(); }

  FakeGLES2Interface gl_;
};

TEST_F(SharedGpuContextTest, contextLossAutoRecovery) {
  EXPECT_NE(SharedGpuContext::ContextProviderWrapper(), nullptr);
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> context =
      SharedGpuContext::ContextProviderWrapper();
  gl_.SetIsContextLost(true);
  EXPECT_FALSE(SharedGpuContext::IsValidWithoutRestoring());
  EXPECT_TRUE(!!context);

  // Context recreation results in old provider being discarded.
  EXPECT_TRUE(!!SharedGpuContext::ContextProviderWrapper());
  EXPECT_FALSE(!!context);
}

TEST_F(SharedGpuContextTest, AccelerateImageBufferSurfaceAutoRecovery) {
  // Verifies that after a context loss, attempting to allocate an
  // AcceleratedImageBufferSurface will restore the context and succeed
  gl_.SetIsContextLost(true);
  EXPECT_FALSE(SharedGpuContext::IsValidWithoutRestoring());
  IntSize size(10, 10);
  std::unique_ptr<CanvasResourceProvider> resource_provider =
      CanvasResourceProvider::Create(
          size, CanvasResourceProvider::kAcceleratedResourceUsage,
          SharedGpuContext::ContextProviderWrapper());
  EXPECT_TRUE(resource_provider && resource_provider->IsValid());
  EXPECT_TRUE(SharedGpuContext::IsValidWithoutRestoring());
}

TEST_F(SharedGpuContextTest, Canvas2DLayerBridgeAutoRecovery) {
  // Verifies that after a context loss, attempting to allocate a
  // Canvas2DLayerBridge will restore the context and succeed.
  gl_.SetIsContextLost(true);
  EXPECT_FALSE(SharedGpuContext::IsValidWithoutRestoring());
  IntSize size(10, 10);
  CanvasColorParams color_params;
  std::unique_ptr<Canvas2DLayerBridge> bridge =
      std::make_unique<Canvas2DLayerBridge>(
          size, 0,
          /*msaa sample count*/ Canvas2DLayerBridge::kEnableAcceleration,
          color_params);
  EXPECT_TRUE(bridge->IsAccelerated());
  EXPECT_TRUE(SharedGpuContext::IsValidWithoutRestoring());
}

TEST_F(SharedGpuContextTest, IsValidWithoutRestoring) {
  EXPECT_NE(SharedGpuContext::ContextProviderWrapper(), nullptr);
  EXPECT_TRUE(SharedGpuContext::IsValidWithoutRestoring());
}

TEST_F(BadSharedGpuContextTest, IsValidWithoutRestoring) {
  EXPECT_FALSE(SharedGpuContext::IsValidWithoutRestoring());
}

TEST_F(BadSharedGpuContextTest, AllowSoftwareToAcceleratedCanvasUpgrade) {
  EXPECT_FALSE(SharedGpuContext::AllowSoftwareToAcceleratedCanvasUpgrade());
}

TEST_F(BadSharedGpuContextTest, AccelerateImageBufferSurfaceCreationFails) {
  // With a bad shared context, AccelerateImageBufferSurface creation should
  // fail gracefully
  IntSize size(10, 10);
  std::unique_ptr<CanvasResourceProvider> resource_provider =
      CanvasResourceProvider::Create(
          size, CanvasResourceProvider::kAcceleratedResourceUsage,
          SharedGpuContext::ContextProviderWrapper());
  EXPECT_FALSE(!resource_provider);
}

TEST_F(SharedGpuContextTest, CompositingMode) {
  EXPECT_TRUE(SharedGpuContext::IsGpuCompositingEnabled());
}

TEST_F(BadSharedGpuContextTest, CompositingMode) {
  EXPECT_TRUE(SharedGpuContext::IsGpuCompositingEnabled());
}

TEST_F(SoftwareCompositingTest, CompositingMode) {
  EXPECT_FALSE(SharedGpuContext::IsGpuCompositingEnabled());
}

class FakeMailboxGenerator {
 public:
  void GenMailbox(GLbyte* name) { *name = counter_++; }

  GLbyte counter_ = 1;
};

TEST_F(MailboxSharedGpuContextTest, MailboxCaching) {
  IntSize size(10, 10);
  std::unique_ptr<CanvasResourceProvider> resource_provider =
      CanvasResourceProvider::Create(
          size, CanvasResourceProvider::kAcceleratedResourceUsage,
          SharedGpuContext::ContextProviderWrapper());
  EXPECT_TRUE(resource_provider && resource_provider->IsValid());
  scoped_refptr<StaticBitmapImage> image = resource_provider->Snapshot();
  testing::Mock::VerifyAndClearExpectations(&gl_);

  FakeMailboxGenerator mailboxGenerator;
  gpu::Mailbox mailbox;
  mailbox.name[0] = 0;

  EXPECT_CALL(gl_, GenMailboxCHROMIUM(mailbox.name))
      .Times(1)
      .WillOnce(testing::Invoke(&mailboxGenerator,
                                &FakeMailboxGenerator::GenMailbox));

  SharedGpuContext::ContextProviderWrapper()->Utils()->GetMailboxForSkImage(
      mailbox, image->PaintImageForCurrentFrame().GetSkImage(), GL_NEAREST);

  EXPECT_EQ(mailbox.name[0], 1);

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_CALL(gl_, GenMailboxCHROMIUM(mailbox.name))
      .Times(0);  // GenMailboxCHROMIUM must not be called!

  mailbox.name[0] = 0;
  SharedGpuContext::ContextProviderWrapper()->Utils()->GetMailboxForSkImage(
      mailbox, image->PaintImageForCurrentFrame().GetSkImage(), GL_NEAREST);
  EXPECT_EQ(mailbox.name[0], 1);

  testing::Mock::VerifyAndClearExpectations(&gl_);
}

TEST_F(MailboxSharedGpuContextTest, MailboxCacheSurvivesSkiaRecycling) {
  IntSize size(10, 10);
  std::unique_ptr<CanvasResourceProvider> resource_provider =
      CanvasResourceProvider::Create(
          size, CanvasResourceProvider::kAcceleratedResourceUsage,
          SharedGpuContext::ContextProviderWrapper());
  EXPECT_TRUE(resource_provider && resource_provider->IsValid());
  scoped_refptr<StaticBitmapImage> image = resource_provider->Snapshot();
  testing::Mock::VerifyAndClearExpectations(&gl_);

  FakeMailboxGenerator mailboxGenerator;
  gpu::Mailbox mailbox;
  mailbox.name[0] = 0;

  EXPECT_CALL(gl_, GenMailboxCHROMIUM(mailbox.name))
      .Times(1)
      .WillOnce(testing::Invoke(&mailboxGenerator,
                                &FakeMailboxGenerator::GenMailbox));

  SharedGpuContext::ContextProviderWrapper()->Utils()->GetMailboxForSkImage(
      mailbox, image->PaintImageForCurrentFrame().GetSkImage(), GL_NEAREST);

  EXPECT_EQ(mailbox.name[0], 1);
  testing::Mock::VerifyAndClearExpectations(&gl_);

  // Destroy image and surface to return texture to recleable resource pool
  image = nullptr;
  resource_provider = nullptr;

  testing::Mock::VerifyAndClearExpectations(&gl_);

  // Re-creating surface should recycle the old GrTexture inside skia
  resource_provider = CanvasResourceProvider::Create(
      size, CanvasResourceProvider::kAcceleratedResourceUsage,
      SharedGpuContext::ContextProviderWrapper());
  EXPECT_TRUE(resource_provider && resource_provider->IsValid());
  image = resource_provider->Snapshot();

  testing::Mock::VerifyAndClearExpectations(&gl_);

  EXPECT_CALL(gl_, GenMailboxCHROMIUM(mailbox.name))
      .Times(0);  // GenMailboxCHROMIUM must not be called!

  mailbox.name[0] = 0;
  SharedGpuContext::ContextProviderWrapper()->Utils()->GetMailboxForSkImage(
      mailbox, image->PaintImageForCurrentFrame().GetSkImage(), GL_NEAREST);
  EXPECT_EQ(mailbox.name[0], 1);

  testing::Mock::VerifyAndClearExpectations(&gl_);
}

}  // unnamed namespace

}  // blink
