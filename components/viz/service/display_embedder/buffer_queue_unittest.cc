// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/buffer_queue.h"

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <utility>

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "components/viz/common/gl_helper.h"
#include "components/viz/test/test_context_provider.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "components/viz/test/test_web_graphics_context_3d.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/display/types/display_snapshot.h"

using ::testing::_;
using ::testing::Expectation;
using ::testing::Ne;
using ::testing::Return;

namespace viz {

class StubGpuMemoryBufferImpl : public gfx::GpuMemoryBuffer {
 public:
  explicit StubGpuMemoryBufferImpl(size_t* set_color_space_count)
      : set_color_space_count_(set_color_space_count) {}

  // Overridden from gfx::GpuMemoryBuffer:
  bool Map() override { return false; }
  void* memory(size_t plane) override { return nullptr; }
  void Unmap() override {}
  gfx::Size GetSize() const override { return gfx::Size(); }
  gfx::BufferFormat GetFormat() const override {
    return gfx::BufferFormat::BGRX_8888;
  }
  int stride(size_t plane) const override { return 0; }
  gfx::GpuMemoryBufferId GetId() const override {
    return gfx::GpuMemoryBufferId(0);
  }
  void SetColorSpace(const gfx::ColorSpace& color_space) override {
    *set_color_space_count_ += 1;
  }
  gfx::GpuMemoryBufferHandle GetHandle() const override {
    return gfx::GpuMemoryBufferHandle();
  }
  ClientBuffer AsClientBuffer() override {
    return reinterpret_cast<ClientBuffer>(this);
  }

  size_t* set_color_space_count_;
};

class StubGpuMemoryBufferManager : public TestGpuMemoryBufferManager {
 public:
  StubGpuMemoryBufferManager() : allocate_succeeds_(true) {}

  size_t set_color_space_count() const { return set_color_space_count_; }

  void set_allocate_succeeds(bool value) { allocate_succeeds_ = value; }

  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override {
    if (!surface_handle) {
      return TestGpuMemoryBufferManager::CreateGpuMemoryBuffer(
          size, format, usage, surface_handle);
    }
    if (allocate_succeeds_)
      return base::WrapUnique<gfx::GpuMemoryBuffer>(
          new StubGpuMemoryBufferImpl(&set_color_space_count_));
    return nullptr;
  }

 private:
  bool allocate_succeeds_;
  size_t set_color_space_count_ = 0;
};

#if defined(OS_WIN)
const gpu::SurfaceHandle kFakeSurfaceHandle =
    reinterpret_cast<gpu::SurfaceHandle>(1);
#else
const gpu::SurfaceHandle kFakeSurfaceHandle = 1;
#endif

const unsigned int kBufferQueueInternalformat = GL_RGBA;
const gfx::BufferFormat kBufferQueueFormat = gfx::BufferFormat::RGBA_8888;

class MockBufferQueue : public BufferQueue {
 public:
  MockBufferQueue(gpu::gles2::GLES2Interface* gl,
                  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                  unsigned int target,
                  unsigned int internalformat)
      : BufferQueue(gl,
                    target,
                    kBufferQueueInternalformat,
                    kBufferQueueFormat,
                    nullptr,
                    gpu_memory_buffer_manager,
                    kFakeSurfaceHandle) {}
  MOCK_METHOD4(CopyBufferDamage,
               void(int, int, const gfx::Rect&, const gfx::Rect&));
};

class BufferQueueTest : public ::testing::Test {
 public:
  BufferQueueTest() : doublebuffering_(true), first_frame_(true) {}

  void SetUp() override { InitWithContext(TestWebGraphicsContext3D::Create()); }

  void InitWithContext(std::unique_ptr<TestWebGraphicsContext3D> context) {
    context_provider_ = TestContextProvider::Create(std::move(context));
    context_provider_->BindToCurrentThread();
    gpu_memory_buffer_manager_.reset(new StubGpuMemoryBufferManager);
    mock_output_surface_ = new MockBufferQueue(context_provider_->ContextGL(),
                                               gpu_memory_buffer_manager_.get(),
                                               GL_TEXTURE_2D,
                                               kBufferQueueInternalformat);
    output_surface_.reset(mock_output_surface_);
    output_surface_->Initialize();
  }

  unsigned current_surface() {
    return output_surface_->current_surface_
               ? output_surface_->current_surface_->image
               : 0;
  }
  const std::vector<std::unique_ptr<BufferQueue::AllocatedSurface>>&
  available_surfaces() {
    return output_surface_->available_surfaces_;
  }
  base::circular_deque<std::unique_ptr<BufferQueue::AllocatedSurface>>&
  in_flight_surfaces() {
    return output_surface_->in_flight_surfaces_;
  }

  const BufferQueue::AllocatedSurface* displayed_frame() {
    return output_surface_->displayed_surface_.get();
  }
  const BufferQueue::AllocatedSurface* current_frame() {
    return output_surface_->current_surface_.get();
  }
  const BufferQueue::AllocatedSurface* next_frame() {
    return output_surface_->available_surfaces_.back().get();
  }
  const gfx::Size size() { return output_surface_->size_; }

  int CountBuffers() {
    int n = available_surfaces().size() + in_flight_surfaces().size() +
            (displayed_frame() ? 1 : 0);
    if (current_surface())
      n++;
    return n;
  }

  // Check that each buffer is unique if present.
  void CheckUnique() {
    std::set<unsigned> buffers;
    EXPECT_TRUE(InsertUnique(&buffers, current_surface()));
    if (displayed_frame())
      EXPECT_TRUE(InsertUnique(&buffers, displayed_frame()->image));
    for (auto& surface : available_surfaces())
      EXPECT_TRUE(InsertUnique(&buffers, surface->image));
    for (auto& surface : in_flight_surfaces()) {
      if (surface)
        EXPECT_TRUE(InsertUnique(&buffers, surface->image));
    }
  }

  void SwapBuffers() {
    output_surface_->SwapBuffers(gfx::Rect(output_surface_->size_));
  }

  void SendDamagedFrame(const gfx::Rect& damage) {
    // We don't care about the GL-level implementation here, just how it uses
    // damage rects.
    output_surface_->BindFramebuffer();
    output_surface_->SwapBuffers(damage);
    if (doublebuffering_ || !first_frame_)
      output_surface_->PageFlipComplete();
    first_frame_ = false;
  }

  void SendFullFrame() { SendDamagedFrame(gfx::Rect(output_surface_->size_)); }

 protected:
  bool InsertUnique(std::set<unsigned>* set, unsigned value) {
    if (!value)
      return true;
    if (set->find(value) != set->end())
      return false;
    set->insert(value);
    return true;
  }

  scoped_refptr<TestContextProvider> context_provider_;
  std::unique_ptr<StubGpuMemoryBufferManager> gpu_memory_buffer_manager_;
  std::unique_ptr<BufferQueue> output_surface_;
  MockBufferQueue* mock_output_surface_;
  bool doublebuffering_;
  bool first_frame_;
};

namespace {
const gfx::Size screen_size = gfx::Size(30, 30);
const gfx::Rect screen_rect = gfx::Rect(screen_size);
const gfx::Rect small_damage = gfx::Rect(gfx::Size(10, 10));
const gfx::Rect large_damage = gfx::Rect(gfx::Size(20, 20));
const gfx::Rect overlapping_damage = gfx::Rect(gfx::Size(5, 20));

GLuint CreateImageDefault() {
  static GLuint id = 0;
  return ++id;
}

class MockedContext : public TestWebGraphicsContext3D {
 public:
  MockedContext() {
    ON_CALL(*this, createImageCHROMIUM(_, _, _, _))
        .WillByDefault(testing::InvokeWithoutArgs(&CreateImageDefault));
  }
  MOCK_METHOD2(bindFramebuffer, void(GLenum, GLuint));
  MOCK_METHOD2(bindTexture, void(GLenum, GLuint));
  MOCK_METHOD2(bindTexImage2DCHROMIUM, void(GLenum, GLint));
  MOCK_METHOD4(createImageCHROMIUM,
               GLuint(ClientBuffer, GLsizei, GLsizei, GLenum));
  MOCK_METHOD1(destroyImageCHROMIUM, void(GLuint));
  MOCK_METHOD5(framebufferTexture2D,
               void(GLenum, GLenum, GLenum, GLuint, GLint));
};

class BufferQueueMockedContextTest : public BufferQueueTest {
 public:
  void SetUp() override {
    context_ = new MockedContext();
    InitWithContext(std::unique_ptr<TestWebGraphicsContext3D>(context_));
  }

 protected:
  MockedContext* context_;
};

scoped_refptr<TestContextProvider> CreateMockedContextProvider(
    MockedContext** context) {
  std::unique_ptr<MockedContext> owned_context(new MockedContext);
  *context = owned_context.get();
  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create(std::move(owned_context));
  context_provider->BindToCurrentThread();
  return context_provider;
}

std::unique_ptr<BufferQueue> CreateBufferQueue(
    unsigned int target,
    gpu::gles2::GLES2Interface* gl,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager) {
  std::unique_ptr<BufferQueue> buffer_queue(new BufferQueue(
      gl, target, kBufferQueueInternalformat, kBufferQueueFormat, nullptr,
      gpu_memory_buffer_manager, kFakeSurfaceHandle));
  buffer_queue->Initialize();
  return buffer_queue;
}

TEST(BufferQueueStandaloneTest, FboInitialization) {
  MockedContext* context;
  scoped_refptr<TestContextProvider> context_provider =
      CreateMockedContextProvider(&context);
  std::unique_ptr<StubGpuMemoryBufferManager> gpu_memory_buffer_manager(
      new StubGpuMemoryBufferManager);
  std::unique_ptr<BufferQueue> output_surface =
      CreateBufferQueue(GL_TEXTURE_2D, context_provider->ContextGL(),
                        gpu_memory_buffer_manager.get());

  EXPECT_CALL(*context, bindFramebuffer(GL_FRAMEBUFFER, Ne(0U)));
  ON_CALL(*context, framebufferTexture2D(_, _, _, _, _))
      .WillByDefault(Return());

  output_surface->Reshape(gfx::Size(10, 20), 1.0f, gfx::ColorSpace(), false);
}

TEST(BufferQueueStandaloneTest, FboBinding) {
  GLenum targets[] = {GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_ARB};
  for (size_t i = 0; i < 2; ++i) {
    GLenum target = targets[i];

    MockedContext* context;
    scoped_refptr<TestContextProvider> context_provider =
        CreateMockedContextProvider(&context);
    std::unique_ptr<StubGpuMemoryBufferManager> gpu_memory_buffer_manager(
        new StubGpuMemoryBufferManager);
    std::unique_ptr<BufferQueue> output_surface = CreateBufferQueue(
        target, context_provider->ContextGL(), gpu_memory_buffer_manager.get());

    EXPECT_CALL(*context, bindTexture(target, Ne(0U)));
    EXPECT_CALL(*context, destroyImageCHROMIUM(1));
    Expectation image =
        EXPECT_CALL(*context,
                    createImageCHROMIUM(_, 0, 0, kBufferQueueInternalformat))
            .WillOnce(Return(1));
    Expectation fb =
        EXPECT_CALL(*context, bindFramebuffer(GL_FRAMEBUFFER, Ne(0U)));
    Expectation tex = EXPECT_CALL(*context, bindTexture(target, Ne(0U)));
    Expectation bind_tex =
        EXPECT_CALL(*context, bindTexImage2DCHROMIUM(target, 1))
            .After(tex, image);
    EXPECT_CALL(*context,
                framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     target, Ne(0U), _))
        .After(fb, bind_tex);

    output_surface->BindFramebuffer();
  }
}

TEST(BufferQueueStandaloneTest, CheckBoundFramebuffer) {
  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create();
  context_provider->BindToCurrentThread();
  std::unique_ptr<StubGpuMemoryBufferManager> gpu_memory_buffer_manager;
  std::unique_ptr<BufferQueue> output_surface;
  gpu_memory_buffer_manager.reset(new StubGpuMemoryBufferManager);

  std::unique_ptr<GLHelper> gl_helper;
  gl_helper.reset(new GLHelper(context_provider->ContextGL(),
                               context_provider->ContextSupport()));

  output_surface.reset(new BufferQueue(
      context_provider->ContextGL(), GL_TEXTURE_2D,
      kBufferQueueInternalformat, kBufferQueueFormat,
      gl_helper.get(),
      gpu_memory_buffer_manager.get(), kFakeSurfaceHandle));
  output_surface->Initialize();
  output_surface->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  // Trigger a sub-buffer copy to exercise all paths.
  output_surface->BindFramebuffer();
  output_surface->SwapBuffers(screen_rect);
  output_surface->PageFlipComplete();
  output_surface->BindFramebuffer();
  output_surface->SwapBuffers(small_damage);

  int current_fbo = 0;
  context_provider->ContextGL()->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                             &current_fbo);
  EXPECT_EQ(static_cast<int>(output_surface->fbo()), current_fbo);
}

TEST_F(BufferQueueTest, PartialSwapReuse) {
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  ASSERT_TRUE(doublebuffering_);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(_, _, small_damage, screen_rect))
      .Times(1);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(_, _, small_damage, small_damage))
      .Times(1);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(_, _, large_damage, small_damage))
      .Times(1);
  SendFullFrame();
  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);
  SendDamagedFrame(large_damage);
  // Verify that the damage has propagated.
  EXPECT_EQ(next_frame()->damage, large_damage);
}

TEST_F(BufferQueueTest, PartialSwapFullFrame) {
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  ASSERT_TRUE(doublebuffering_);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(_, _, small_damage, screen_rect))
      .Times(1);
  SendFullFrame();
  SendDamagedFrame(small_damage);
  SendFullFrame();
  SendFullFrame();
  EXPECT_EQ(next_frame()->damage, screen_rect);
}

TEST_F(BufferQueueTest, PartialSwapOverlapping) {
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  ASSERT_TRUE(doublebuffering_);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(_, _, small_damage, screen_rect))
      .Times(1);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(_, _, overlapping_damage, small_damage))
      .Times(1);

  SendFullFrame();
  SendDamagedFrame(small_damage);
  SendDamagedFrame(overlapping_damage);
  EXPECT_EQ(next_frame()->damage, overlapping_damage);
}

TEST_F(BufferQueueTest, MultipleBindCalls) {
  // Check that multiple bind calls do not create or change surfaces.
  output_surface_->BindFramebuffer();
  EXPECT_EQ(1, CountBuffers());
  unsigned int fb = current_surface();
  output_surface_->BindFramebuffer();
  EXPECT_EQ(1, CountBuffers());
  EXPECT_EQ(fb, current_surface());
}

TEST_F(BufferQueueTest, CheckDoubleBuffering) {
  // Check buffer flow through double buffering path.
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  EXPECT_EQ(0, CountBuffers());
  output_surface_->BindFramebuffer();
  EXPECT_EQ(1, CountBuffers());
  EXPECT_NE(0U, current_surface());
  EXPECT_FALSE(displayed_frame());
  SwapBuffers();
  EXPECT_EQ(1U, in_flight_surfaces().size());
  output_surface_->PageFlipComplete();
  EXPECT_EQ(0U, in_flight_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  output_surface_->BindFramebuffer();
  EXPECT_EQ(2, CountBuffers());
  CheckUnique();
  EXPECT_NE(0U, current_surface());
  EXPECT_EQ(0U, in_flight_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  SwapBuffers();
  CheckUnique();
  EXPECT_EQ(1U, in_flight_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  output_surface_->PageFlipComplete();
  CheckUnique();
  EXPECT_EQ(0U, in_flight_surfaces().size());
  EXPECT_EQ(1U, available_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  output_surface_->BindFramebuffer();
  EXPECT_EQ(2, CountBuffers());
  CheckUnique();
  EXPECT_TRUE(available_surfaces().empty());
}

TEST_F(BufferQueueTest, CheckTripleBuffering) {
  // Check buffer flow through triple buffering path.
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);

  // This bit is the same sequence tested in the doublebuffering case.
  output_surface_->BindFramebuffer();
  EXPECT_FALSE(displayed_frame());
  SwapBuffers();
  output_surface_->PageFlipComplete();
  output_surface_->BindFramebuffer();
  SwapBuffers();

  EXPECT_EQ(2, CountBuffers());
  CheckUnique();
  EXPECT_EQ(1U, in_flight_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  output_surface_->BindFramebuffer();
  EXPECT_EQ(3, CountBuffers());
  CheckUnique();
  EXPECT_NE(0U, current_surface());
  EXPECT_EQ(1U, in_flight_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  output_surface_->PageFlipComplete();
  EXPECT_EQ(3, CountBuffers());
  CheckUnique();
  EXPECT_NE(0U, current_surface());
  EXPECT_EQ(0U, in_flight_surfaces().size());
  EXPECT_TRUE(displayed_frame()->texture);
  EXPECT_EQ(1U, available_surfaces().size());
}

TEST_F(BufferQueueTest, CheckEmptySwap) {
  // Check empty swap flow, in which the damage is empty and BindFramebuffer
  // might not be called.
  EXPECT_EQ(0, CountBuffers());
  output_surface_->BindFramebuffer();
  EXPECT_EQ(1, CountBuffers());
  EXPECT_NE(0U, current_surface());
  EXPECT_FALSE(displayed_frame());

  // This is the texture to scanout.
  uint32_t texture_id = output_surface_->GetCurrentTextureId();
  SwapBuffers();
  // Make sure we won't be drawing to the texture we just sent for scanout.
  output_surface_->BindFramebuffer();
  EXPECT_NE(texture_id, output_surface_->GetCurrentTextureId());

  EXPECT_EQ(1U, in_flight_surfaces().size());
  output_surface_->PageFlipComplete();

  // Test swapbuffers without calling BindFramebuffer. DirectRenderer skips
  // BindFramebuffer if not necessary.
  SwapBuffers();
  SwapBuffers();
  EXPECT_EQ(2U, in_flight_surfaces().size());

  output_surface_->PageFlipComplete();
  EXPECT_EQ(1U, in_flight_surfaces().size());

  output_surface_->PageFlipComplete();
  EXPECT_EQ(0U, in_flight_surfaces().size());
}

TEST_F(BufferQueueTest, CheckCorrectBufferOrdering) {
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  const size_t kSwapCount = 3;
  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->BindFramebuffer();
    SwapBuffers();
  }

  EXPECT_EQ(kSwapCount, in_flight_surfaces().size());
  for (size_t i = 0; i < kSwapCount; ++i) {
    unsigned int next_texture_id = in_flight_surfaces().front()->texture;
    output_surface_->PageFlipComplete();
    EXPECT_EQ(displayed_frame()->texture, next_texture_id);
  }
}

TEST_F(BufferQueueTest, ReshapeWithInFlightSurfaces) {
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  const size_t kSwapCount = 3;
  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->BindFramebuffer();
    SwapBuffers();
  }

  output_surface_->Reshape(gfx::Size(10, 20), 1.0f, gfx::ColorSpace(), false);
  EXPECT_EQ(3u, in_flight_surfaces().size());

  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->PageFlipComplete();
    EXPECT_FALSE(displayed_frame());
  }

  // The dummy surfacess left should be discarded.
  EXPECT_EQ(0u, available_surfaces().size());
}

TEST_F(BufferQueueTest, SwapAfterReshape) {
  DCHECK_EQ(0u, gpu_memory_buffer_manager_->set_color_space_count());
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  const size_t kSwapCount = 3;
  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->BindFramebuffer();
    SwapBuffers();
  }
  DCHECK_EQ(kSwapCount, gpu_memory_buffer_manager_->set_color_space_count());

  output_surface_->Reshape(gfx::Size(10, 20), 1.0f, gfx::ColorSpace(), false);
  DCHECK_EQ(kSwapCount, gpu_memory_buffer_manager_->set_color_space_count());

  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->BindFramebuffer();
    SwapBuffers();
  }
  DCHECK_EQ(2 * kSwapCount,
            gpu_memory_buffer_manager_->set_color_space_count());
  EXPECT_EQ(2 * kSwapCount, in_flight_surfaces().size());

  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->PageFlipComplete();
    EXPECT_FALSE(displayed_frame());
  }

  CheckUnique();

  for (size_t i = 0; i < kSwapCount; ++i) {
    unsigned int next_texture_id = in_flight_surfaces().front()->texture;
    output_surface_->PageFlipComplete();
    EXPECT_EQ(displayed_frame()->texture, next_texture_id);
    EXPECT_TRUE(displayed_frame());
  }

  DCHECK_EQ(2 * kSwapCount,
            gpu_memory_buffer_manager_->set_color_space_count());
  for (size_t i = 0; i < kSwapCount; ++i) {
    output_surface_->BindFramebuffer();
    SwapBuffers();
    output_surface_->PageFlipComplete();
  }
  DCHECK_EQ(2 * kSwapCount,
            gpu_memory_buffer_manager_->set_color_space_count());
}

TEST_F(BufferQueueMockedContextTest, RecreateBuffers) {
  // This setup is to easily get one frame in each of:
  // - currently bound for drawing.
  // - in flight to GPU.
  // - currently displayed.
  // - free frame.
  // This tests buffers in all states.
  // Bind/swap pushes frames into the in flight list, then the PageFlipComplete
  // calls pull one frame into displayed and another into the free list.
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);
  output_surface_->BindFramebuffer();
  SwapBuffers();
  output_surface_->BindFramebuffer();
  SwapBuffers();
  output_surface_->BindFramebuffer();
  SwapBuffers();
  output_surface_->BindFramebuffer();
  output_surface_->PageFlipComplete();
  output_surface_->PageFlipComplete();
  // We should have one buffer in each possible state right now, including one
  // being drawn to.
  ASSERT_EQ(1U, in_flight_surfaces().size());
  ASSERT_EQ(1U, available_surfaces().size());
  EXPECT_TRUE(displayed_frame());
  EXPECT_TRUE(current_frame());

  auto* current = current_frame();
  auto* displayed = displayed_frame();
  auto* in_flight = in_flight_surfaces().front().get();
  auto* available = available_surfaces().front().get();

  // Expect all 4 images to be destroyed, 3 of the existing textures to be
  // copied from and 3 new images to be created.
  EXPECT_CALL(*context_, createImageCHROMIUM(_, screen_size.width(),
                                             screen_size.height(),
                                             kBufferQueueInternalformat))
      .Times(3);
  Expectation copy1 = EXPECT_CALL(*mock_output_surface_,
                                  CopyBufferDamage(_, displayed->texture, _, _))
                          .Times(1);
  Expectation copy2 = EXPECT_CALL(*mock_output_surface_,
                                  CopyBufferDamage(_, current->texture, _, _))
                          .Times(1);
  Expectation copy3 = EXPECT_CALL(*mock_output_surface_,
                                  CopyBufferDamage(_, in_flight->texture, _, _))
                          .Times(1);

  EXPECT_CALL(*context_, destroyImageCHROMIUM(displayed->image))
      .Times(1)
      .After(copy1);
  EXPECT_CALL(*context_, destroyImageCHROMIUM(current->image))
      .Times(1)
      .After(copy2);
  EXPECT_CALL(*context_, destroyImageCHROMIUM(in_flight->image))
      .Times(1)
      .After(copy3);
  EXPECT_CALL(*context_, destroyImageCHROMIUM(available->image)).Times(1);
  // After copying, we expect the framebuffer binding to be updated.
  EXPECT_CALL(*context_, bindFramebuffer(_, _))
      .After(copy1)
      .After(copy2)
      .After(copy3);
  EXPECT_CALL(*context_, framebufferTexture2D(_, _, _, _, _))
      .After(copy1)
      .After(copy2)
      .After(copy3);

  output_surface_->RecreateBuffers();
  testing::Mock::VerifyAndClearExpectations(context_);
  testing::Mock::VerifyAndClearExpectations(mock_output_surface_);

  // All free buffers should be destroyed, the remaining buffers should all
  // be replaced but still valid.
  EXPECT_EQ(1U, in_flight_surfaces().size());
  EXPECT_EQ(0U, available_surfaces().size());
  EXPECT_TRUE(displayed_frame());
  EXPECT_TRUE(current_frame());
}

TEST_F(BufferQueueTest, AllocateFails) {
  output_surface_->Reshape(screen_size, 1.0f, gfx::ColorSpace(), false);

  // Succeed in the two swaps.
  output_surface_->BindFramebuffer();
  EXPECT_TRUE(current_frame());
  output_surface_->SwapBuffers(screen_rect);

  // Fail the next surface allocation.
  gpu_memory_buffer_manager_->set_allocate_succeeds(false);
  output_surface_->BindFramebuffer();
  EXPECT_FALSE(current_frame());
  output_surface_->SwapBuffers(screen_rect);
  EXPECT_FALSE(current_frame());

  // Try another swap. It should copy the buffer damage from the back
  // surface.
  gpu_memory_buffer_manager_->set_allocate_succeeds(true);
  output_surface_->BindFramebuffer();
  unsigned int source_texture = in_flight_surfaces().front()->texture;
  unsigned int target_texture = current_frame()->texture;
  testing::Mock::VerifyAndClearExpectations(mock_output_surface_);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(target_texture, source_texture, small_damage, _))
      .Times(1);
  output_surface_->SwapBuffers(small_damage);
  testing::Mock::VerifyAndClearExpectations(mock_output_surface_);

  // Destroy the just-created buffer, and try another swap. The copy should
  // come from the displayed surface (because both in-flight surfaces are
  // gone now).
  output_surface_->PageFlipComplete();
  in_flight_surfaces().back().reset();
  EXPECT_EQ(2u, in_flight_surfaces().size());
  for (auto& surface : in_flight_surfaces())
    EXPECT_FALSE(surface);
  output_surface_->BindFramebuffer();
  source_texture = displayed_frame()->texture;
  EXPECT_TRUE(current_frame());
  EXPECT_TRUE(displayed_frame());
  target_texture = current_frame()->texture;
  testing::Mock::VerifyAndClearExpectations(mock_output_surface_);
  EXPECT_CALL(*mock_output_surface_,
              CopyBufferDamage(target_texture, source_texture, small_damage, _))
      .Times(1);
  output_surface_->SwapBuffers(small_damage);
  testing::Mock::VerifyAndClearExpectations(mock_output_surface_);
}

}  // namespace
}  // namespace viz
