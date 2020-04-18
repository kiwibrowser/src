/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/gpu/drawing_buffer.h"

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/common/resources/transferable_resource.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/client/gles2_interface_stub.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/graphics/canvas_color_params.h"
#include "third_party/blink/renderer/platform/graphics/gpu/drawing_buffer_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "v8/include/v8.h"

using testing::Test;
using testing::_;

namespace blink {

namespace {

class FakePlatformSupport : public TestingPlatformSupport {
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override {
    return &test_gpu_memory_buffer_manager_;
  }

 private:
  viz::TestGpuMemoryBufferManager test_gpu_memory_buffer_manager_;
};

}  // anonymous namespace

class DrawingBufferTest : public Test {
 protected:
  void SetUp() override { Init(kDisableMultisampling); }

  void Init(UseMultisampling use_multisampling) {
    IntSize initial_size(kInitialWidth, kInitialHeight);
    auto gl = std::make_unique<GLES2InterfaceForTests>();
    auto provider =
        std::make_unique<WebGraphicsContext3DProviderForTests>(std::move(gl));
    GLES2InterfaceForTests* gl_ =
        static_cast<GLES2InterfaceForTests*>(provider->ContextGL());
    bool gpu_compositing = true;
    drawing_buffer_ = DrawingBufferForTests::Create(
        std::move(provider), gpu_compositing, gl_, initial_size,
        DrawingBuffer::kPreserve, use_multisampling);
    CHECK(drawing_buffer_);
    SetAndSaveRestoreState(false);
  }

  // Initialize GL state with unusual values, to verify that they are restored.
  // The |invert| parameter will reverse all boolean parameters, so that all
  // values are tested.
  void SetAndSaveRestoreState(bool invert) {
    GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
    GLboolean scissor_enabled = !invert;
    GLfloat clear_color[4] = {0.1, 0.2, 0.3, 0.4};
    GLfloat clear_depth = 0.8;
    GLint clear_stencil = 37;
    GLboolean color_mask[4] = {invert, !invert, !invert, invert};
    GLboolean depth_mask = invert;
    GLboolean stencil_mask = invert;
    GLint pack_alignment = 7;
    GLuint active_texture2d_binding = 0xbeef1;
    GLuint renderbuffer_binding = 0xbeef2;
    GLuint draw_framebuffer_binding = 0xbeef3;
    GLuint read_framebuffer_binding = 0xbeef4;
    GLuint pixel_unpack_buffer_binding = 0xbeef5;

    if (scissor_enabled)
      gl_->Enable(GL_SCISSOR_TEST);
    else
      gl_->Disable(GL_SCISSOR_TEST);

    gl_->ClearColor(clear_color[0], clear_color[1], clear_color[2],
                    clear_color[3]);
    gl_->ClearDepthf(clear_depth);
    gl_->ClearStencil(clear_stencil);
    gl_->ColorMask(color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
    gl_->DepthMask(depth_mask);
    gl_->StencilMask(stencil_mask);
    gl_->PixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
    gl_->BindTexture(GL_TEXTURE_2D, active_texture2d_binding);
    gl_->BindRenderbuffer(GL_RENDERBUFFER, renderbuffer_binding);
    gl_->BindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_framebuffer_binding);
    gl_->BindFramebuffer(GL_READ_FRAMEBUFFER, read_framebuffer_binding);
    gl_->BindBuffer(GL_PIXEL_UNPACK_BUFFER, pixel_unpack_buffer_binding);

    gl_->SaveState();
  }

  void VerifyStateWasRestored() {
    GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
    gl_->VerifyStateHasNotChangedSinceSave();
  }

  scoped_refptr<DrawingBufferForTests> drawing_buffer_;
};

class DrawingBufferTestMultisample : public DrawingBufferTest {
 protected:
  void SetUp() override { Init(kEnableMultisampling); }
};

TEST_F(DrawingBufferTestMultisample, verifyMultisampleResolve) {
  // Initial state: already marked changed, multisampled
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->ExplicitResolveOfMultisampleData());

  // Resolve the multisample buffer
  drawing_buffer_->ResolveAndBindForReadAndDraw();

  // After resolve, acknowledge new content
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  // No new content
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());

  drawing_buffer_->BeginDestruction();
}

TEST_F(DrawingBufferTest, VerifyResizingProperlyAffectsResources) {
  GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
  VerifyStateWasRestored();
  viz::TransferableResource resource;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback;

  IntSize initial_size(kInitialWidth, kInitialHeight);
  IntSize alternate_size(kInitialWidth, kAlternateHeight);

  // Produce one resource at size 100x100.
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  VerifyStateWasRestored();
  EXPECT_EQ(initial_size, gl_->MostRecentlyProducedSize());

  // Resize to 100x50.
  drawing_buffer_->Resize(alternate_size);
  VerifyStateWasRestored();
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  VerifyStateWasRestored();

  // Produce a resource at this size.
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  EXPECT_EQ(alternate_size, gl_->MostRecentlyProducedSize());
  VerifyStateWasRestored();

  // Reset to initial size.
  drawing_buffer_->Resize(initial_size);
  VerifyStateWasRestored();
  SetAndSaveRestoreState(true);
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  VerifyStateWasRestored();

  // Prepare another resource and verify that it's the correct size.
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  EXPECT_EQ(initial_size, gl_->MostRecentlyProducedSize());
  VerifyStateWasRestored();

  // Prepare one final resource and verify that it's the correct size.
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  VerifyStateWasRestored();
  EXPECT_EQ(initial_size, gl_->MostRecentlyProducedSize());
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  drawing_buffer_->BeginDestruction();
}

TEST_F(DrawingBufferTest, VerifyDestructionCompleteAfterAllResourceReleased) {
  bool live = true;
  drawing_buffer_->live_ = &live;

  viz::TransferableResource resource1;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback1;
  viz::TransferableResource resource2;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback2;
  viz::TransferableResource resource3;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback3;

  IntSize initial_size(kInitialWidth, kInitialHeight);

  // Produce resources.
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  drawing_buffer_->ClearFramebuffers(GL_STENCIL_BUFFER_BIT);
  VerifyStateWasRestored();
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource1,
                                                           &release_callback1));
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  drawing_buffer_->ClearFramebuffers(GL_DEPTH_BUFFER_BIT);
  VerifyStateWasRestored();
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource2,
                                                           &release_callback2));
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  drawing_buffer_->ClearFramebuffers(GL_COLOR_BUFFER_BIT);
  VerifyStateWasRestored();
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource3,
                                                           &release_callback3));

  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  release_callback1->Run(gpu::SyncToken(), false /* lostResource */);

  drawing_buffer_->BeginDestruction();
  ASSERT_EQ(live, true);

  DrawingBufferForTests* raw_pointer = drawing_buffer_.get();
  drawing_buffer_ = nullptr;
  ASSERT_EQ(live, true);

  EXPECT_FALSE(raw_pointer->MarkContentsChanged());
  release_callback2->Run(gpu::SyncToken(), false /* lostResource */);
  ASSERT_EQ(live, true);

  EXPECT_FALSE(raw_pointer->MarkContentsChanged());
  release_callback3->Run(gpu::SyncToken(), false /* lostResource */);
  ASSERT_EQ(live, false);
}

TEST_F(DrawingBufferTest, verifyDrawingBufferStaysAliveIfResourcesAreLost) {
  bool live = true;
  drawing_buffer_->live_ = &live;

  viz::TransferableResource resource1;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback1;
  viz::TransferableResource resource2;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback2;
  viz::TransferableResource resource3;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback3;

  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource1,
                                                           &release_callback1));
  VerifyStateWasRestored();
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource2,
                                                           &release_callback2));
  VerifyStateWasRestored();
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource3,
                                                           &release_callback3));
  VerifyStateWasRestored();

  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  release_callback1->Run(gpu::SyncToken(), true /* lostResource */);
  EXPECT_EQ(live, true);

  drawing_buffer_->BeginDestruction();
  EXPECT_EQ(live, true);

  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  release_callback2->Run(gpu::SyncToken(), false /* lostResource */);
  EXPECT_EQ(live, true);

  DrawingBufferForTests* raw_ptr = drawing_buffer_.get();
  drawing_buffer_ = nullptr;
  EXPECT_EQ(live, true);

  EXPECT_FALSE(raw_ptr->MarkContentsChanged());
  release_callback3->Run(gpu::SyncToken(), true /* lostResource */);
  EXPECT_EQ(live, false);
}

TEST_F(DrawingBufferTest, VerifyOnlyOneRecycledResourceMustBeKept) {
  viz::TransferableResource resource1;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback1;
  viz::TransferableResource resource2;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback2;
  viz::TransferableResource resource3;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback3;

  // Produce resources.
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource1,
                                                           &release_callback1));
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource2,
                                                           &release_callback2));
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource3,
                                                           &release_callback3));

  // Release resources by specific order; 1, 3, 2.
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  release_callback1->Run(gpu::SyncToken(), false /* lostResource */);
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  release_callback3->Run(gpu::SyncToken(), false /* lostResource */);
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  release_callback2->Run(gpu::SyncToken(), false /* lostResource */);

  // The first recycled resource must be 2. 1 and 3 were deleted by FIFO order
  // because DrawingBuffer never keeps more than one resource.
  viz::TransferableResource recycled_resource1;
  std::unique_ptr<viz::SingleReleaseCallback> recycled_release_callback1;
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(
      nullptr, &recycled_resource1, &recycled_release_callback1));
  EXPECT_EQ(resource2.mailbox_holder.mailbox,
            recycled_resource1.mailbox_holder.mailbox);

  // The second recycled resource must be a new resource.
  viz::TransferableResource recycled_resource2;
  std::unique_ptr<viz::SingleReleaseCallback> recycled_release_callback2;
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(
      nullptr, &recycled_resource2, &recycled_release_callback2));
  EXPECT_NE(resource1.mailbox_holder.mailbox,
            recycled_resource2.mailbox_holder.mailbox);
  EXPECT_NE(resource2.mailbox_holder.mailbox,
            recycled_resource2.mailbox_holder.mailbox);
  EXPECT_NE(resource3.mailbox_holder.mailbox,
            recycled_resource2.mailbox_holder.mailbox);

  recycled_release_callback1->Run(gpu::SyncToken(), false /* lostResource */);
  recycled_release_callback2->Run(gpu::SyncToken(), false /* lostResource */);
  drawing_buffer_->BeginDestruction();
}

TEST_F(DrawingBufferTest, verifyInsertAndWaitSyncTokenCorrectly) {
  GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
  viz::TransferableResource resource;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback;

  // Produce resources.
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_EQ(gpu::SyncToken(), gl_->MostRecentlyWaitedSyncToken());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  // PrepareTransferableResource() does not wait for any sync point.
  EXPECT_EQ(gpu::SyncToken(), gl_->MostRecentlyWaitedSyncToken());

  gpu::SyncToken wait_sync_token;
  gl_->GenSyncTokenCHROMIUM(wait_sync_token.GetData());
  release_callback->Run(wait_sync_token, false /* lostResource */);
  // m_drawingBuffer will wait for the sync point when recycling.
  EXPECT_EQ(gpu::SyncToken(), gl_->MostRecentlyWaitedSyncToken());

  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  // m_drawingBuffer waits for the sync point when recycling in
  // PrepareTransferableResource().
  EXPECT_EQ(wait_sync_token, gl_->MostRecentlyWaitedSyncToken());

  drawing_buffer_->BeginDestruction();
  gl_->GenSyncTokenCHROMIUM(wait_sync_token.GetData());
  release_callback->Run(wait_sync_token, false /* lostResource */);
  // m_drawingBuffer waits for the sync point because the destruction is in
  // progress.
  EXPECT_EQ(wait_sync_token, gl_->MostRecentlyWaitedSyncToken());
}

class DrawingBufferImageChromiumTest : public DrawingBufferTest,
                                       private ScopedWebGLImageChromiumForTest {
 public:
  DrawingBufferImageChromiumTest() : ScopedWebGLImageChromiumForTest(true) {}

 protected:
  void SetUp() override {
    platform_.reset(new ScopedTestingPlatformSupport<FakePlatformSupport>);

    IntSize initial_size(kInitialWidth, kInitialHeight);
    auto gl = std::make_unique<GLES2InterfaceForTests>();
    auto provider =
        std::make_unique<WebGraphicsContext3DProviderForTests>(std::move(gl));
    GLES2InterfaceForTests* gl_ =
        static_cast<GLES2InterfaceForTests*>(provider->ContextGL());
    image_id0_ = gl_->NextImageIdToBeCreated();
    EXPECT_CALL(*gl_, BindTexImage2DMock(image_id0_)).Times(1);
    bool gpu_compositing = true;
    drawing_buffer_ = DrawingBufferForTests::Create(
        std::move(provider), gpu_compositing, gl_, initial_size,
        DrawingBuffer::kPreserve, kDisableMultisampling);
    CHECK(drawing_buffer_);
    SetAndSaveRestoreState(true);
    testing::Mock::VerifyAndClearExpectations(gl_);
  }

  void TearDown() override {
    platform_.reset();
  }

  GLuint image_id0_;
  std::unique_ptr<ScopedTestingPlatformSupport<FakePlatformSupport>> platform_;
};

TEST_F(DrawingBufferImageChromiumTest, VerifyResizingReallocatesImages) {
  GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
  viz::TransferableResource resource;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback;

  IntSize initial_size(kInitialWidth, kInitialHeight);
  IntSize alternate_size(kInitialWidth, kAlternateHeight);

  GLuint image_id1 = gl_->NextImageIdToBeCreated();
  EXPECT_CALL(*gl_, BindTexImage2DMock(image_id1)).Times(1);
  // Produce one resource at size 100x100.
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  EXPECT_EQ(initial_size, gl_->MostRecentlyProducedSize());
  EXPECT_TRUE(resource.is_overlay_candidate);
  EXPECT_EQ(static_cast<gfx::Size>(initial_size), resource.size);
  testing::Mock::VerifyAndClearExpectations(gl_);
  VerifyStateWasRestored();

  GLuint image_id2 = gl_->NextImageIdToBeCreated();
  EXPECT_CALL(*gl_, BindTexImage2DMock(image_id2)).Times(1);
  EXPECT_CALL(*gl_, DestroyImageMock(image_id0_)).Times(1);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(image_id0_)).Times(1);
  EXPECT_CALL(*gl_, DestroyImageMock(image_id1)).Times(1);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(image_id1)).Times(1);
  // Resize to 100x50.
  drawing_buffer_->Resize(alternate_size);
  VerifyStateWasRestored();
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  VerifyStateWasRestored();
  testing::Mock::VerifyAndClearExpectations(gl_);

  GLuint image_id3 = gl_->NextImageIdToBeCreated();
  EXPECT_CALL(*gl_, BindTexImage2DMock(image_id3)).Times(1);
  // Produce a resource at this size.
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  EXPECT_EQ(alternate_size, gl_->MostRecentlyProducedSize());
  EXPECT_TRUE(resource.is_overlay_candidate);
  EXPECT_EQ(static_cast<gfx::Size>(alternate_size), resource.size);
  testing::Mock::VerifyAndClearExpectations(gl_);

  GLuint image_id4 = gl_->NextImageIdToBeCreated();
  EXPECT_CALL(*gl_, BindTexImage2DMock(image_id4)).Times(1);
  EXPECT_CALL(*gl_, DestroyImageMock(image_id2)).Times(1);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(image_id2)).Times(1);
  EXPECT_CALL(*gl_, DestroyImageMock(image_id3)).Times(1);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(image_id3)).Times(1);
  // Reset to initial size.
  drawing_buffer_->Resize(initial_size);
  VerifyStateWasRestored();
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  VerifyStateWasRestored();
  testing::Mock::VerifyAndClearExpectations(gl_);

  GLuint image_id5 = gl_->NextImageIdToBeCreated();
  EXPECT_CALL(*gl_, BindTexImage2DMock(image_id5)).Times(1);
  // Prepare another resource and verify that it's the correct size.
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  EXPECT_EQ(initial_size, gl_->MostRecentlyProducedSize());
  EXPECT_TRUE(resource.is_overlay_candidate);
  EXPECT_EQ(static_cast<gfx::Size>(initial_size), resource.size);
  testing::Mock::VerifyAndClearExpectations(gl_);

  // Prepare one final resource and verify that it's the correct size.
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));
  EXPECT_EQ(initial_size, gl_->MostRecentlyProducedSize());
  EXPECT_TRUE(resource.is_overlay_candidate);
  EXPECT_EQ(static_cast<gfx::Size>(initial_size), resource.size);
  release_callback->Run(gpu::SyncToken(), false /* lostResource */);

  EXPECT_CALL(*gl_, DestroyImageMock(image_id5)).Times(1);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(image_id5)).Times(1);
  EXPECT_CALL(*gl_, DestroyImageMock(image_id4)).Times(1);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(image_id4)).Times(1);
  drawing_buffer_->BeginDestruction();
  testing::Mock::VerifyAndClearExpectations(gl_);
}

TEST_F(DrawingBufferImageChromiumTest, AllocationFailure) {
  GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
  viz::TransferableResource resource1;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback1;
  viz::TransferableResource resource2;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback2;
  viz::TransferableResource resource3;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback3;

  // Request a resource. An image should already be created. Everything works
  // as expected.
  EXPECT_CALL(*gl_, BindTexImage2DMock(_)).Times(1);
  IntSize initial_size(kInitialWidth, kInitialHeight);
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource1,
                                                           &release_callback1));
  EXPECT_TRUE(resource1.is_overlay_candidate);
  testing::Mock::VerifyAndClearExpectations(gl_);
  VerifyStateWasRestored();

  // Force image CHROMIUM creation failure. Request another resource. It should
  // still be provided, but this time with allowOverlay = false.
  gl_->SetCreateImageChromiumFail(true);
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource2,
                                                           &release_callback2));
  EXPECT_FALSE(resource2.is_overlay_candidate);
  VerifyStateWasRestored();

  // Check that if image CHROMIUM starts working again, resources are
  // correctly created with allowOverlay = true.
  EXPECT_CALL(*gl_, BindTexImage2DMock(_)).Times(1);
  gl_->SetCreateImageChromiumFail(false);
  EXPECT_TRUE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource3,
                                                           &release_callback3));
  EXPECT_TRUE(resource3.is_overlay_candidate);
  testing::Mock::VerifyAndClearExpectations(gl_);
  VerifyStateWasRestored();

  release_callback1->Run(gpu::SyncToken(), false /* lostResource */);
  release_callback2->Run(gpu::SyncToken(), false /* lostResource */);
  release_callback3->Run(gpu::SyncToken(), false /* lostResource */);

  EXPECT_CALL(*gl_, DestroyImageMock(_)).Times(3);
  EXPECT_CALL(*gl_, ReleaseTexImage2DMock(_)).Times(3);
  drawing_buffer_->BeginDestruction();
  testing::Mock::VerifyAndClearExpectations(gl_);
}

class DepthStencilTrackingGLES2Interface
    : public gpu::gles2::GLES2InterfaceStub {
 public:
  void FramebufferRenderbuffer(GLenum target,
                               GLenum attachment,
                               GLenum renderbuffertarget,
                               GLuint renderbuffer) override {
    switch (attachment) {
      case GL_DEPTH_ATTACHMENT:
        depth_attachment_ = renderbuffer;
        break;
      case GL_STENCIL_ATTACHMENT:
        stencil_attachment_ = renderbuffer;
        break;
      case GL_DEPTH_STENCIL_ATTACHMENT:
        depth_stencil_attachment_ = renderbuffer;
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  GLenum CheckFramebufferStatus(GLenum target) override {
    return GL_FRAMEBUFFER_COMPLETE;
  }

  void GetIntegerv(GLenum ptype, GLint* value) override {
    switch (ptype) {
      case GL_MAX_TEXTURE_SIZE:
        *value = 1024;
        return;
    }
  }

  const GLubyte* GetString(GLenum type) override {
    if (type == GL_EXTENSIONS)
      return reinterpret_cast<const GLubyte*>("GL_OES_packed_depth_stencil");
    return reinterpret_cast<const GLubyte*>("");
  }

  void GenRenderbuffers(GLsizei n, GLuint* renderbuffers) override {
    for (GLsizei i = 0; i < n; ++i)
      renderbuffers[i] = next_gen_renderbuffer_id_++;
  }

  GLuint StencilAttachment() const { return stencil_attachment_; }
  GLuint DepthAttachment() const { return depth_attachment_; }
  GLuint DepthStencilAttachment() const { return depth_stencil_attachment_; }
  size_t NumAllocatedRenderBuffer() const {
    return next_gen_renderbuffer_id_ - 1;
  }

 private:
  GLuint next_gen_renderbuffer_id_ = 1;
  GLuint depth_attachment_ = 0;
  GLuint stencil_attachment_ = 0;
  GLuint depth_stencil_attachment_ = 0;
};

struct DepthStencilTestCase {
  DepthStencilTestCase(bool request_stencil,
                       bool request_depth,
                       int expected_render_buffers,
                       const char* const test_case_name)
      : request_stencil(request_stencil),
        request_depth(request_depth),
        expected_render_buffers(expected_render_buffers),
        test_case_name(test_case_name) {}

  bool request_stencil;
  bool request_depth;
  size_t expected_render_buffers;
  const char* const test_case_name;
};

// This tests that, when the packed depth+stencil extension is supported, and
// either depth or stencil is requested, DrawingBuffer always allocates a single
// packed renderbuffer and properly computes the actual context attributes as
// defined by WebGL. We always allocate a packed buffer in this case since many
// desktop OpenGL drivers that support this extension do not consider a
// framebuffer with only a depth or a stencil buffer attached to be complete.
TEST(DrawingBufferDepthStencilTest, packedDepthStencilSupported) {
  DepthStencilTestCase cases[] = {
      DepthStencilTestCase(false, false, 0, "neither"),
      DepthStencilTestCase(true, false, 1, "stencil only"),
      DepthStencilTestCase(false, true, 1, "depth only"),
      DepthStencilTestCase(true, true, 1, "both"),
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(cases[i].test_case_name);
    auto gl = std::make_unique<DepthStencilTrackingGLES2Interface>();
    DepthStencilTrackingGLES2Interface* tracking_gl = gl.get();
    auto provider =
        std::make_unique<WebGraphicsContext3DProviderForTests>(std::move(gl));
    DrawingBuffer::PreserveDrawingBuffer preserve = DrawingBuffer::kPreserve;

    bool premultiplied_alpha = false;
    bool want_alpha_channel = true;
    bool want_depth_buffer = cases[i].request_depth;
    bool want_stencil_buffer = cases[i].request_stencil;
    bool want_antialiasing = false;
    bool gpu_compositing = true;
    scoped_refptr<DrawingBuffer> drawing_buffer = DrawingBuffer::Create(
        std::move(provider), gpu_compositing, nullptr, IntSize(10, 10),
        premultiplied_alpha, want_alpha_channel, want_depth_buffer,
        want_stencil_buffer, want_antialiasing, preserve,
        DrawingBuffer::kWebGL1, DrawingBuffer::kAllowChromiumImage,
        CanvasColorParams());

    // When we request a depth or a stencil buffer, we will get both.
    EXPECT_EQ(cases[i].request_depth || cases[i].request_stencil,
              drawing_buffer->HasDepthBuffer());
    EXPECT_EQ(cases[i].request_depth || cases[i].request_stencil,
              drawing_buffer->HasStencilBuffer());
    EXPECT_EQ(cases[i].expected_render_buffers,
              tracking_gl->NumAllocatedRenderBuffer());
    if (cases[i].request_depth || cases[i].request_stencil) {
      EXPECT_NE(0u, tracking_gl->DepthStencilAttachment());
      EXPECT_EQ(0u, tracking_gl->DepthAttachment());
      EXPECT_EQ(0u, tracking_gl->StencilAttachment());
    } else {
      EXPECT_EQ(0u, tracking_gl->DepthStencilAttachment());
      EXPECT_EQ(0u, tracking_gl->DepthAttachment());
      EXPECT_EQ(0u, tracking_gl->StencilAttachment());
    }

    drawing_buffer->Resize(IntSize(10, 20));
    EXPECT_EQ(cases[i].request_depth || cases[i].request_stencil,
              drawing_buffer->HasDepthBuffer());
    EXPECT_EQ(cases[i].request_depth || cases[i].request_stencil,
              drawing_buffer->HasStencilBuffer());
    EXPECT_EQ(cases[i].expected_render_buffers,
              tracking_gl->NumAllocatedRenderBuffer());
    if (cases[i].request_depth || cases[i].request_stencil) {
      EXPECT_NE(0u, tracking_gl->DepthStencilAttachment());
      EXPECT_EQ(0u, tracking_gl->DepthAttachment());
      EXPECT_EQ(0u, tracking_gl->StencilAttachment());
    } else {
      EXPECT_EQ(0u, tracking_gl->DepthStencilAttachment());
      EXPECT_EQ(0u, tracking_gl->DepthAttachment());
      EXPECT_EQ(0u, tracking_gl->StencilAttachment());
    }

    drawing_buffer->BeginDestruction();
  }
}

TEST_F(DrawingBufferTest, VerifySetIsHiddenProperlyAffectsMailboxes) {
  GLES2InterfaceForTests* gl_ = drawing_buffer_->ContextGLForTests();
  viz::TransferableResource resource;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback;

  // Produce resources.
  EXPECT_FALSE(drawing_buffer_->MarkContentsChanged());
  EXPECT_TRUE(drawing_buffer_->PrepareTransferableResource(nullptr, &resource,
                                                           &release_callback));

  gpu::SyncToken wait_sync_token;
  gl_->GenSyncTokenCHROMIUM(wait_sync_token.GetData());
  drawing_buffer_->SetIsHidden(true);
  release_callback->Run(wait_sync_token, false /* lostResource */);
  // m_drawingBuffer deletes resource immediately when hidden.

  EXPECT_EQ(wait_sync_token, gl_->MostRecentlyWaitedSyncToken());

  drawing_buffer_->BeginDestruction();
}

TEST_F(DrawingBufferTest,
       VerifyTooBigDrawingBufferExceedingV8MaxSizeFailsToCreate) {
  IntSize too_big_size(1, (v8::TypedArray::kMaxLength / 4) + 1);
  bool gpu_compositing = true;
  scoped_refptr<DrawingBuffer> too_big_drawing_buffer = DrawingBuffer::Create(
      nullptr, gpu_compositing, nullptr, too_big_size, false, false, false,
      false, false, DrawingBuffer::kDiscard, DrawingBuffer::kWebGL1,
      DrawingBuffer::kAllowChromiumImage, CanvasColorParams());
  EXPECT_EQ(too_big_drawing_buffer, nullptr);
  drawing_buffer_->BeginDestruction();
}

}  // namespace blink
