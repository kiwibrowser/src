// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/display_resource_provider.h"
#include "cc/resources/layer_tree_resource_provider.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "cc/test/test_context_provider.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "cc/test/test_texture.h"
#include "cc/test/test_web_graphics_context_3d.h"
#include "components/viz/common/resources/buffer_to_texture_target_map.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/returned_resource.h"
#include "components/viz/common/resources/shared_bitmap_manager.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/gpu_memory_buffer.h"

using testing::InSequence;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;
using testing::_;

namespace cc {
namespace {

static const bool kUseGpuMemoryBufferResources = false;
static const bool kDelegatedSyncPointsRequired = true;

MATCHER_P(MatchesSyncToken, sync_token, "") {
  gpu::SyncToken other;
  memcpy(&other, arg, sizeof(other));
  return other == sync_token;
}

static void EmptyReleaseCallback(const gpu::SyncToken& sync_token,
                                 bool lost_resource) {}

static void ReleaseCallback(gpu::SyncToken* release_sync_token,
                            bool* release_lost_resource,
                            const gpu::SyncToken& sync_token,
                            bool lost_resource) {
  *release_sync_token = sync_token;
  *release_lost_resource = lost_resource;
}

static void SharedBitmapReleaseCallback(
    std::unique_ptr<viz::SharedBitmap> bitmap,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {}

static void ReleaseSharedBitmapCallback(
    std::unique_ptr<viz::SharedBitmap> shared_bitmap,
    bool* release_called,
    gpu::SyncToken* release_sync_token,
    bool* lost_resource_result,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  *release_called = true;
  *release_sync_token = sync_token;
  *lost_resource_result = lost_resource;
}

static std::unique_ptr<viz::SharedBitmap> CreateAndFillSharedBitmap(
    viz::SharedBitmapManager* manager,
    const gfx::Size& size,
    uint32_t value) {
  std::unique_ptr<viz::SharedBitmap> shared_bitmap =
      manager->AllocateSharedBitmap(size);
  CHECK(shared_bitmap);
  uint32_t* pixels = reinterpret_cast<uint32_t*>(shared_bitmap->pixels());
  CHECK(pixels);
  std::fill_n(pixels, size.GetArea(), value);
  return shared_bitmap;
}

static viz::ResourceSettings CreateResourceSettings(
    size_t texture_id_allocation_chunk_size = 1) {
  viz::ResourceSettings resource_settings;
  resource_settings.texture_id_allocation_chunk_size =
      texture_id_allocation_chunk_size;
  resource_settings.use_gpu_memory_buffer_resources =
      kUseGpuMemoryBufferResources;
  resource_settings.buffer_to_texture_target_map =
      viz::DefaultBufferToTextureTargetMapForTesting();
  return resource_settings;
}

class TextureStateTrackingContext : public TestWebGraphicsContext3D {
 public:
  MOCK_METHOD2(bindTexture, void(GLenum target, GLuint texture));
  MOCK_METHOD3(texParameteri, void(GLenum target, GLenum pname, GLint param));
  MOCK_METHOD1(waitSyncToken, void(const GLbyte* sync_token));
  MOCK_METHOD3(produceTextureDirectCHROMIUM,
               void(GLuint texture, GLenum target, const GLbyte* mailbox));
  MOCK_METHOD2(createAndConsumeTextureCHROMIUM,
               unsigned(GLenum target, const GLbyte* mailbox));

  // Force all textures to be consecutive numbers starting at "1",
  // so we easily can test for them.
  GLuint NextTextureId() override {
    base::AutoLock lock(namespace_->lock);
    return namespace_->next_texture_id++;
  }

  void RetireTextureId(GLuint) override {}

  GLuint64 insertFenceSync() override { return next_fence_sync_++; }

  void genSyncToken(GLuint64 fence_sync, GLbyte* sync_token) override {
    gpu::SyncToken sync_token_data(gpu::CommandBufferNamespace::GPU_IO, 0,
                                   gpu::CommandBufferId::FromUnsafeValue(0x123),
                                   fence_sync);
    sync_token_data.SetVerifyFlush();
    memcpy(sync_token, &sync_token_data, sizeof(sync_token_data));
  }

  GLuint64 GetNextFenceSync() const { return next_fence_sync_; }

  GLuint64 next_fence_sync_ = 1;
};

// Shared data between multiple ResourceProviderContext. This contains mailbox
// contents as well as information about sync points.
class ContextSharedData {
 public:
  static std::unique_ptr<ContextSharedData> Create() {
    return base::WrapUnique(new ContextSharedData());
  }

  uint32_t InsertFenceSync() { return next_fence_sync_++; }

  void GenMailbox(GLbyte* mailbox) {
    memset(mailbox, 0, GL_MAILBOX_SIZE_CHROMIUM);
    memcpy(mailbox, &next_mailbox_, sizeof(next_mailbox_));
    ++next_mailbox_;
  }

  void ProduceTexture(const GLbyte* mailbox_name,
                      const gpu::SyncToken& sync_token,
                      scoped_refptr<TestTexture> texture) {
    uint32_t sync_point = static_cast<uint32_t>(sync_token.release_count());

    unsigned mailbox = 0;
    memcpy(&mailbox, mailbox_name, sizeof(mailbox));
    ASSERT_TRUE(mailbox && mailbox < next_mailbox_);
    textures_[mailbox] = texture;
    ASSERT_LT(sync_point_for_mailbox_[mailbox], sync_point);
    sync_point_for_mailbox_[mailbox] = sync_point;
  }

  scoped_refptr<TestTexture> ConsumeTexture(const GLbyte* mailbox_name,
                                            const gpu::SyncToken& sync_token) {
    unsigned mailbox = 0;
    memcpy(&mailbox, mailbox_name, sizeof(mailbox));
    DCHECK(mailbox && mailbox < next_mailbox_);

    // If the latest sync point the context has waited on is before the sync
    // point for when the mailbox was set, pretend we never saw that
    // ProduceTexture.
    if (sync_point_for_mailbox_[mailbox] > sync_token.release_count()) {
      NOTREACHED();
      return scoped_refptr<TestTexture>();
    }
    return textures_[mailbox];
  }

 private:
  ContextSharedData() : next_fence_sync_(1), next_mailbox_(1) {}

  uint64_t next_fence_sync_;
  unsigned next_mailbox_;
  using TextureMap = std::unordered_map<unsigned, scoped_refptr<TestTexture>>;
  TextureMap textures_;
  std::unordered_map<unsigned, uint32_t> sync_point_for_mailbox_;
};

class ResourceProviderContext : public TestWebGraphicsContext3D {
 public:
  static std::unique_ptr<ResourceProviderContext> Create(
      ContextSharedData* shared_data) {
    return base::WrapUnique(new ResourceProviderContext(shared_data));
  }

  GLuint64 insertFenceSync() override {
    return shared_data_->InsertFenceSync();
  }

  void genSyncToken(GLuint64 fence_sync, GLbyte* sync_token) override {
    gpu::SyncToken sync_token_data(gpu::CommandBufferNamespace::GPU_IO, 0,
                                   gpu::CommandBufferId::FromUnsafeValue(0x123),
                                   fence_sync);
    sync_token_data.SetVerifyFlush();
    // Commit the produceTextureCHROMIUM calls at this point, so that
    // they're associated with the sync point.
    for (const std::unique_ptr<PendingProduceTexture>& pending_texture :
         pending_produce_textures_) {
      shared_data_->ProduceTexture(pending_texture->mailbox, sync_token_data,
                                   pending_texture->texture);
    }
    pending_produce_textures_.clear();
    memcpy(sync_token, &sync_token_data, sizeof(sync_token_data));
  }

  void waitSyncToken(const GLbyte* sync_token) override {
    gpu::SyncToken sync_token_data;
    if (sync_token)
      memcpy(&sync_token_data, sync_token, sizeof(sync_token_data));

    if (sync_token_data.release_count() >
        last_waited_sync_token_.release_count()) {
      last_waited_sync_token_ = sync_token_data;
    }
  }

  const gpu::SyncToken& last_waited_sync_token() const {
    return last_waited_sync_token_;
  }

  void texStorage2DEXT(GLenum target,
                       GLint levels,
                       GLuint internalformat,
                       GLint width,
                       GLint height) override {
    CheckTextureIsBound(target);
    ASSERT_EQ(static_cast<unsigned>(GL_TEXTURE_2D), target);
    ASSERT_EQ(1, levels);
    GLenum format = GL_RGBA;
    switch (internalformat) {
      case GL_RGBA8_OES:
        break;
      case GL_BGRA8_EXT:
        format = GL_BGRA_EXT;
        break;
      default:
        NOTREACHED();
    }
    AllocateTexture(gfx::Size(width, height), format);
  }

  void texImage2D(GLenum target,
                  GLint level,
                  GLenum internalformat,
                  GLsizei width,
                  GLsizei height,
                  GLint border,
                  GLenum format,
                  GLenum type,
                  const void* pixels) override {
    CheckTextureIsBound(target);
    ASSERT_EQ(static_cast<unsigned>(GL_TEXTURE_2D), target);
    ASSERT_FALSE(level);
    ASSERT_EQ(internalformat, format);
    ASSERT_FALSE(border);
    ASSERT_EQ(static_cast<unsigned>(GL_UNSIGNED_BYTE), type);
    AllocateTexture(gfx::Size(width, height), format);
    if (pixels)
      SetPixels(0, 0, width, height, pixels);
  }

  void texSubImage2D(GLenum target,
                     GLint level,
                     GLint xoffset,
                     GLint yoffset,
                     GLsizei width,
                     GLsizei height,
                     GLenum format,
                     GLenum type,
                     const void* pixels) override {
    CheckTextureIsBound(target);
    ASSERT_EQ(static_cast<unsigned>(GL_TEXTURE_2D), target);
    ASSERT_FALSE(level);
    ASSERT_EQ(static_cast<unsigned>(GL_UNSIGNED_BYTE), type);
    {
      base::AutoLock lock_for_texture_access(namespace_->lock);
      ASSERT_EQ(GLDataFormat(BoundTexture(target)->format), format);
    }
    ASSERT_TRUE(pixels);
    SetPixels(xoffset, yoffset, width, height, pixels);
  }

  void genMailboxCHROMIUM(GLbyte* mailbox) override {
    return shared_data_->GenMailbox(mailbox);
  }

  void produceTextureDirectCHROMIUM(GLuint texture,
                                    GLenum target,
                                    const GLbyte* mailbox) override {
    // Delay moving the texture into the mailbox until the next
    // sync token, so that it is not visible to other contexts that
    // haven't waited on that sync point.
    std::unique_ptr<PendingProduceTexture> pending(new PendingProduceTexture);
    memcpy(pending->mailbox, mailbox, sizeof(pending->mailbox));
    base::AutoLock lock_for_texture_access(namespace_->lock);
    pending->texture = UnboundTexture(texture);
    pending_produce_textures_.push_back(std::move(pending));
  }

  GLuint createAndConsumeTextureCHROMIUM(GLenum target,
                                         const GLbyte* mailbox) override {
    GLuint texture_id = createTexture();
    base::AutoLock lock_for_texture_access(namespace_->lock);
    scoped_refptr<TestTexture> texture =
        shared_data_->ConsumeTexture(mailbox, last_waited_sync_token_);
    namespace_->textures.Replace(texture_id, texture);
    return texture_id;
  }

  void GetPixels(const gfx::Size& size,
                 viz::ResourceFormat format,
                 uint8_t* pixels) {
    CheckTextureIsBound(GL_TEXTURE_2D);
    base::AutoLock lock_for_texture_access(namespace_->lock);
    scoped_refptr<TestTexture> texture = BoundTexture(GL_TEXTURE_2D);
    ASSERT_EQ(texture->size, size);
    ASSERT_EQ(texture->format, format);
    memcpy(pixels, texture->data.get(), TextureSizeBytes(size, format));
  }

 protected:
  explicit ResourceProviderContext(ContextSharedData* shared_data)
      : shared_data_(shared_data) {}

 private:
  void AllocateTexture(const gfx::Size& size, GLenum format) {
    CheckTextureIsBound(GL_TEXTURE_2D);
    viz::ResourceFormat texture_format = viz::RGBA_8888;
    switch (format) {
      case GL_RGBA:
        texture_format = viz::RGBA_8888;
        break;
      case GL_BGRA_EXT:
        texture_format = viz::BGRA_8888;
        break;
    }
    base::AutoLock lock_for_texture_access(namespace_->lock);
    BoundTexture(GL_TEXTURE_2D)->Reallocate(size, texture_format);
  }

  void SetPixels(int xoffset,
                 int yoffset,
                 int width,
                 int height,
                 const void* pixels) {
    CheckTextureIsBound(GL_TEXTURE_2D);
    base::AutoLock lock_for_texture_access(namespace_->lock);
    scoped_refptr<TestTexture> texture = BoundTexture(GL_TEXTURE_2D);
    ASSERT_TRUE(texture->data.get());
    ASSERT_TRUE(xoffset >= 0 && xoffset + width <= texture->size.width());
    ASSERT_TRUE(yoffset >= 0 && yoffset + height <= texture->size.height());
    ASSERT_TRUE(pixels);
    size_t in_pitch = TextureSizeBytes(gfx::Size(width, 1), texture->format);
    size_t out_pitch =
        TextureSizeBytes(gfx::Size(texture->size.width(), 1), texture->format);
    uint8_t* dest = texture->data.get() + yoffset * out_pitch +
                    TextureSizeBytes(gfx::Size(xoffset, 1), texture->format);
    const uint8_t* src = static_cast<const uint8_t*>(pixels);
    for (int i = 0; i < height; ++i) {
      memcpy(dest, src, in_pitch);
      dest += out_pitch;
      src += in_pitch;
    }
  }

  struct PendingProduceTexture {
    GLbyte mailbox[GL_MAILBOX_SIZE_CHROMIUM];
    scoped_refptr<TestTexture> texture;
  };
  ContextSharedData* shared_data_;
  gpu::SyncToken last_waited_sync_token_;
  std::vector<std::unique_ptr<PendingProduceTexture>> pending_produce_textures_;
};

void GetResourcePixels(DisplayResourceProvider* resource_provider,
                       ResourceProviderContext* context,
                       viz::ResourceId id,
                       const gfx::Size& size,
                       viz::ResourceFormat format,
                       uint8_t* pixels) {
  resource_provider->WaitSyncToken(id);
  switch (resource_provider->default_resource_type()) {
    case ResourceProvider::RESOURCE_TYPE_GPU_MEMORY_BUFFER:
    case ResourceProvider::RESOURCE_TYPE_GL_TEXTURE: {
      DisplayResourceProvider::ScopedReadLockGL lock_gl(resource_provider, id);
      ASSERT_NE(0U, lock_gl.texture_id());
      context->bindTexture(GL_TEXTURE_2D, lock_gl.texture_id());
      context->GetPixels(size, format, pixels);
      break;
    }
    case ResourceProvider::RESOURCE_TYPE_BITMAP: {
      DisplayResourceProvider::ScopedReadLockSoftware lock_software(
          resource_provider, id);
      memcpy(pixels, lock_software.sk_bitmap()->getPixels(),
             lock_software.sk_bitmap()->computeByteSize());
      break;
    }
  }
}

class ResourceProviderTest
    : public testing::TestWithParam<ResourceProvider::ResourceType> {
 public:
  explicit ResourceProviderTest(bool child_needs_sync_token)
      : shared_data_(ContextSharedData::Create()),
        context3d_(NULL),
        child_context_(NULL) {
    switch (GetParam()) {
      case ResourceProvider::RESOURCE_TYPE_GPU_MEMORY_BUFFER:
      case ResourceProvider::RESOURCE_TYPE_GL_TEXTURE: {
        auto context3d(ResourceProviderContext::Create(shared_data_.get()));
        context3d_ = context3d.get();
        context_provider_ = TestContextProvider::Create(std::move(context3d));
        context_provider_->BindToCurrentThread();

        auto child_context_owned =
            ResourceProviderContext::Create(shared_data_.get());
        child_context_ = child_context_owned.get();
        child_context_provider_ =
            TestContextProvider::Create(std::move(child_context_owned));
        child_context_provider_->BindToCurrentThread();
        break;
      }
      case ResourceProvider::RESOURCE_TYPE_BITMAP:
        break;
    }

    shared_bitmap_manager_.reset(new TestSharedBitmapManager);
    gpu_memory_buffer_manager_ =
        std::make_unique<viz::TestGpuMemoryBufferManager>();
    child_gpu_memory_buffer_manager_ =
        gpu_memory_buffer_manager_->CreateClientGpuMemoryBufferManager();

    viz::ResourceSettings resource_settings = CreateResourceSettings();
    resource_provider_ = std::make_unique<DisplayResourceProvider>(
        context_provider_.get(), shared_bitmap_manager_.get(),
        gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
        resource_settings);
    child_resource_provider_ = std::make_unique<LayerTreeResourceProvider>(
        child_context_provider_.get(), shared_bitmap_manager_.get(),
        child_gpu_memory_buffer_manager_.get(), child_needs_sync_token,
        resource_settings);
  }

  ResourceProviderTest() : ResourceProviderTest(true) {}

  static void CollectResources(
      std::vector<viz::ReturnedResource>* array,
      const std::vector<viz::ReturnedResource>& returned) {
    array->insert(array->end(), returned.begin(), returned.end());
  }

  static ReturnCallback GetReturnCallback(
      std::vector<viz::ReturnedResource>* array) {
    return base::Bind(&ResourceProviderTest::CollectResources, array);
  }

  static void SetResourceFilter(DisplayResourceProvider* resource_provider,
                                viz::ResourceId id,
                                GLenum filter) {
    DisplayResourceProvider::ScopedSamplerGL sampler(resource_provider, id,
                                                     GL_TEXTURE_2D, filter);
  }

  ResourceProviderContext* context() { return context3d_; }

  viz::ResourceId CreateChildMailbox(gpu::SyncToken* release_sync_token,
                                     bool* lost_resource,
                                     bool* release_called,
                                     gpu::SyncToken* sync_token) {
    if (GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE) {
      unsigned texture = child_context_->createTexture();
      gpu::Mailbox gpu_mailbox;
      child_context_->genMailboxCHROMIUM(gpu_mailbox.name);
      child_context_->produceTextureDirectCHROMIUM(texture, GL_TEXTURE_2D,
                                                   gpu_mailbox.name);
      child_context_->genSyncToken(child_context_->insertFenceSync(),
                                   sync_token->GetData());
      EXPECT_TRUE(sync_token->HasData());

      std::unique_ptr<viz::SharedBitmap> shared_bitmap;
      std::unique_ptr<viz::SingleReleaseCallback> callback =
          viz::SingleReleaseCallback::Create(base::Bind(
              ReleaseSharedBitmapCallback, base::Passed(&shared_bitmap),
              release_called, release_sync_token, lost_resource));
      return child_resource_provider_->CreateResourceFromTextureMailbox(
          viz::TextureMailbox(gpu_mailbox, *sync_token, GL_TEXTURE_2D),
          std::move(callback));
    } else {
      gfx::Size size(64, 64);
      std::unique_ptr<viz::SharedBitmap> shared_bitmap(
          CreateAndFillSharedBitmap(shared_bitmap_manager_.get(), size, 0));

      viz::SharedBitmap* shared_bitmap_ptr = shared_bitmap.get();
      std::unique_ptr<viz::SingleReleaseCallback> callback =
          viz::SingleReleaseCallback::Create(base::Bind(
              ReleaseSharedBitmapCallback, base::Passed(&shared_bitmap),
              release_called, release_sync_token, lost_resource));
      return child_resource_provider_->CreateResourceFromTextureMailbox(
          viz::TextureMailbox(shared_bitmap_ptr, size), std::move(callback));
    }
  }

 protected:
  std::unique_ptr<ContextSharedData> shared_data_;
  ResourceProviderContext* context3d_;
  ResourceProviderContext* child_context_;
  scoped_refptr<TestContextProvider> context_provider_;
  scoped_refptr<TestContextProvider> child_context_provider_;
  std::unique_ptr<viz::TestGpuMemoryBufferManager> gpu_memory_buffer_manager_;
  std::unique_ptr<DisplayResourceProvider> resource_provider_;
  std::unique_ptr<viz::TestGpuMemoryBufferManager>
      child_gpu_memory_buffer_manager_;
  std::unique_ptr<LayerTreeResourceProvider> child_resource_provider_;
  std::unique_ptr<TestSharedBitmapManager> shared_bitmap_manager_;
};

void CheckCreateResource(ResourceProvider::ResourceType expected_default_type,
                         DisplayResourceProvider* resource_provider,
                         ResourceProviderContext* context) {
  DCHECK_EQ(expected_default_type, resource_provider->default_resource_type());

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  EXPECT_EQ(1, static_cast<int>(resource_provider->num_resources()));
  if (expected_default_type == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    EXPECT_EQ(0u, context->NumTextures());

  uint8_t data[4] = { 1, 2, 3, 4 };
  resource_provider->CopyToResource(id, data, size);
  if (expected_default_type == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    EXPECT_EQ(1u, context->NumTextures());

  uint8_t result[4] = { 0 };
  GetResourcePixels(resource_provider, context, id, size, format, result);
  EXPECT_EQ(0, memcmp(data, result, pixel_size));

  resource_provider->DeleteResource(id);
  EXPECT_EQ(0, static_cast<int>(resource_provider->num_resources()));
  if (expected_default_type == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    EXPECT_EQ(0u, context->NumTextures());
}

TEST_P(ResourceProviderTest, Basic) {
  CheckCreateResource(GetParam(), resource_provider_.get(), context());
}

TEST_P(ResourceProviderTest, SimpleUpload) {
  gfx::Size size(2, 2);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(16U, pixel_size);

  viz::ResourceId id = resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());

  uint8_t image[16] = {0};
  resource_provider_->CopyToResource(id, image, size);
  {
    uint8_t result[16] = {0};
    uint8_t expected[16] = {0};
    GetResourcePixels(resource_provider_.get(), context(), id, size, format,
                      result);
    EXPECT_EQ(0, memcmp(expected, result, pixel_size));
  }

  for (uint8_t i = 0; i < pixel_size; ++i)
    image[i] = i;
  resource_provider_->CopyToResource(id, image, size);
  {
    uint8_t result[16] = {0};
    uint8_t expected[16] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    GetResourcePixels(resource_provider_.get(), context(), id, size, format,
                      result);
    EXPECT_EQ(0, memcmp(expected, result, pixel_size));
  }
}

TEST_P(ResourceProviderTest, TransferGLResources) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  gfx::ColorSpace color_space1 = gfx::ColorSpace::CreateSRGB();
  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, color_space1);
  uint8_t data1[4] = { 1, 2, 3, 4 };
  child_resource_provider_->CopyToResource(id1, data1, size);

  viz::ResourceId id2 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data2[4] = { 5, 5, 5, 5 };
  child_resource_provider_->CopyToResource(id2, data2, size);

  viz::ResourceId id3 = child_resource_provider_->CreateGpuMemoryBufferResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
      gfx::BufferUsage::GPU_READ_CPU_READ_WRITE, gfx::ColorSpace());
  {
    LayerTreeResourceProvider::ScopedWriteLockGpuMemoryBuffer lock(
        child_resource_provider_.get(), id3);
    EXPECT_TRUE(lock.GetGpuMemoryBuffer());
  }

  GLuint external_texture_id = child_context_->createExternalTexture();

  gpu::Mailbox external_mailbox;
  child_context_->genMailboxCHROMIUM(external_mailbox.name);
  child_context_->produceTextureDirectCHROMIUM(
      external_texture_id, GL_TEXTURE_EXTERNAL_OES, external_mailbox.name);
  gpu::SyncToken external_sync_token;
  child_context_->genSyncToken(child_context_->insertFenceSync(),
                               external_sync_token.GetData());
  EXPECT_TRUE(external_sync_token.HasData());

  gfx::ColorSpace color_space4 = gfx::ColorSpace::CreateXYZD50();
  viz::TextureMailbox id4_mailbox(external_mailbox, external_sync_token,
                                  GL_TEXTURE_EXTERNAL_OES);
  id4_mailbox.set_color_space(color_space4);
  viz::ResourceId id4 =
      child_resource_provider_->CreateResourceFromTextureMailbox(
          id4_mailbox, viz::SingleReleaseCallback::Create(
                           base::Bind(&EmptyReleaseCallback)));

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));

  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);
    resource_ids_to_transfer.push_back(id3);
    resource_ids_to_transfer.push_back(id4);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(4u, list.size());
    EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());
    EXPECT_TRUE(list[1].mailbox_holder.sync_token.HasData());
    EXPECT_EQ(list[0].mailbox_holder.sync_token,
              list[1].mailbox_holder.sync_token);
    EXPECT_TRUE(list[2].mailbox_holder.sync_token.HasData());
    EXPECT_EQ(list[0].mailbox_holder.sync_token,
              list[2].mailbox_holder.sync_token);
    EXPECT_EQ(external_sync_token, list[3].mailbox_holder.sync_token);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[0].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[1].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[2].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_EXTERNAL_OES),
              list[3].mailbox_holder.texture_target);
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id3));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id4));
    resource_provider_->ReceiveFromChild(child_id, list);
    EXPECT_NE(list[0].mailbox_holder.sync_token,
              context3d_->last_waited_sync_token());
    {
      resource_provider_->WaitSyncToken(list[0].id);
      DisplayResourceProvider::ScopedReadLockGL lock(resource_provider_.get(),
                                                     list[0].id);
    }
    EXPECT_EQ(list[0].mailbox_holder.sync_token,
              context3d_->last_waited_sync_token());
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_ids_to_receive.insert(id3);
    resource_ids_to_receive.insert(id4);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  EXPECT_EQ(4u, resource_provider_->num_resources());
  ResourceProvider::ResourceIdMap resource_map =
      resource_provider_->GetChildToParentMap(child_id);
  viz::ResourceId mapped_id1 = resource_map[id1];
  viz::ResourceId mapped_id2 = resource_map[id2];
  viz::ResourceId mapped_id3 = resource_map[id3];
  viz::ResourceId mapped_id4 = resource_map[id4];
  EXPECT_NE(0u, mapped_id1);
  EXPECT_NE(0u, mapped_id2);
  EXPECT_NE(0u, mapped_id3);
  EXPECT_NE(0u, mapped_id4);
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id1));
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id2));
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id3));
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id4));

  uint8_t result[4] = { 0 };
  GetResourcePixels(
      resource_provider_.get(), context(), mapped_id1, size, format, result);
  EXPECT_EQ(0, memcmp(data1, result, pixel_size));

  GetResourcePixels(
      resource_provider_.get(), context(), mapped_id2, size, format, result);
  EXPECT_EQ(0, memcmp(data2, result, pixel_size));

  EXPECT_FALSE(resource_provider_->IsOverlayCandidate(mapped_id1));
  EXPECT_FALSE(resource_provider_->IsOverlayCandidate(mapped_id2));
  EXPECT_TRUE(resource_provider_->IsOverlayCandidate(mapped_id3));
  EXPECT_FALSE(resource_provider_->IsOverlayCandidate(mapped_id4));

  {
    resource_provider_->WaitSyncToken(mapped_id1);
    DisplayResourceProvider::ScopedReadLockGL lock1(resource_provider_.get(),
                                                    mapped_id1);
    EXPECT_TRUE(lock1.color_space() == color_space1);

    resource_provider_->WaitSyncToken(mapped_id4);
    DisplayResourceProvider::ScopedReadLockGL lock4(resource_provider_.get(),
                                                    mapped_id4);
    EXPECT_TRUE(lock4.color_space() == color_space4);
  }

  {
    // Check that transfering again the same resource from the child to the
    // parent works.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);
    resource_ids_to_transfer.push_back(id3);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    EXPECT_EQ(3u, list.size());
    EXPECT_EQ(id1, list[0].id);
    EXPECT_EQ(id2, list[1].id);
    EXPECT_EQ(id3, list[2].id);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[0].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[1].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[2].mailbox_holder.texture_target);
    std::vector<viz::ReturnedResource> returned =
        viz::TransferableResource::ReturnResources(list);
    child_resource_provider_->ReceiveReturnsFromParent(returned);
    // ids were exported twice, we returned them only once, they should still
    // be in-use.
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id3));
  }
  {
    EXPECT_EQ(0u, returned_to_child.size());

    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);

    ASSERT_EQ(4u, returned_to_child.size());
    EXPECT_TRUE(returned_to_child[0].sync_token.HasData());
    EXPECT_TRUE(returned_to_child[1].sync_token.HasData());
    EXPECT_TRUE(returned_to_child[2].sync_token.HasData());
    EXPECT_TRUE(returned_to_child[3].sync_token.HasData());
    EXPECT_FALSE(returned_to_child[0].lost);
    EXPECT_FALSE(returned_to_child[1].lost);
    EXPECT_FALSE(returned_to_child[2].lost);
    EXPECT_FALSE(returned_to_child[3].lost);
    child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
    returned_to_child.clear();
  }
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id1));
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id2));
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id3));
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id4));

  {
    child_resource_provider_->WaitSyncToken(id1);
    ResourceProvider::ScopedWriteLockGL lock(child_resource_provider_.get(),
                                             id1);
    ASSERT_NE(0U, lock.GetTexture());
  }
  // Ensure copying to resource doesn't fail.
  child_resource_provider_->CopyToResource(id2, data2, size);
  {
    child_resource_provider_->WaitSyncToken(id3);
    ResourceProvider::ScopedWriteLockGL lock(child_resource_provider_.get(),
                                             id3);
    ASSERT_NE(0U, lock.GetTexture());
  }
  {
    // Transfer resources to the parent again.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);
    resource_ids_to_transfer.push_back(id3);
    resource_ids_to_transfer.push_back(id4);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(4u, list.size());
    EXPECT_EQ(id1, list[0].id);
    EXPECT_EQ(id2, list[1].id);
    EXPECT_EQ(id3, list[2].id);
    EXPECT_EQ(id4, list[3].id);
    EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());
    EXPECT_TRUE(list[1].mailbox_holder.sync_token.HasData());
    EXPECT_TRUE(list[2].mailbox_holder.sync_token.HasData());
    EXPECT_TRUE(list[3].mailbox_holder.sync_token.HasData());
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[0].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[1].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[2].mailbox_holder.texture_target);
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_EXTERNAL_OES),
              list[3].mailbox_holder.texture_target);
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id3));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id4));
    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_ids_to_receive.insert(id3);
    resource_ids_to_receive.insert(id4);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  EXPECT_EQ(0u, returned_to_child.size());

  EXPECT_EQ(4u, resource_provider_->num_resources());
  resource_provider_->DestroyChild(child_id);
  EXPECT_EQ(0u, resource_provider_->num_resources());

  ASSERT_EQ(4u, returned_to_child.size());
  EXPECT_TRUE(returned_to_child[0].sync_token.HasData());
  EXPECT_TRUE(returned_to_child[1].sync_token.HasData());
  EXPECT_TRUE(returned_to_child[2].sync_token.HasData());
  EXPECT_TRUE(returned_to_child[3].sync_token.HasData());
  EXPECT_FALSE(returned_to_child[0].lost);
  EXPECT_FALSE(returned_to_child[1].lost);
  EXPECT_FALSE(returned_to_child[2].lost);
  EXPECT_FALSE(returned_to_child[3].lost);
}

#if defined(OS_ANDROID)
TEST_P(ResourceProviderTest, OverlayPromotionHint) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  GLuint external_texture_id = child_context_->createExternalTexture();

  gpu::Mailbox external_mailbox;
  child_context_->genMailboxCHROMIUM(external_mailbox.name);
  child_context_->produceTextureDirectCHROMIUM(
      external_texture_id, GL_TEXTURE_EXTERNAL_OES, external_mailbox.name);
  gpu::SyncToken external_sync_token;
  child_context_->genSyncToken(child_context_->insertFenceSync(),
                               external_sync_token.GetData());
  EXPECT_TRUE(external_sync_token.HasData());

  viz::TextureMailbox id1_mailbox(external_mailbox, external_sync_token,
                                  GL_TEXTURE_EXTERNAL_OES);
  id1_mailbox.set_wants_promotion_hint(true);
  id1_mailbox.set_is_overlay_candidate(true);
  id1_mailbox.set_is_backed_by_surface_texture(true);
  viz::ResourceId id1 =
      child_resource_provider_->CreateResourceFromTextureMailbox(
          id1_mailbox, viz::SingleReleaseCallback::Create(
                           base::Bind(&EmptyReleaseCallback)));

  viz::TextureMailbox id2_mailbox(external_mailbox, external_sync_token,
                                  GL_TEXTURE_EXTERNAL_OES);
  id2_mailbox.set_wants_promotion_hint(false);
  id2_mailbox.set_is_overlay_candidate(true);
  id2_mailbox.set_is_backed_by_surface_texture(false);
  viz::ResourceId id2 =
      child_resource_provider_->CreateResourceFromTextureMailbox(
          id2_mailbox, viz::SingleReleaseCallback::Create(
                           base::Bind(&EmptyReleaseCallback)));

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));

  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(2u, list.size());
    resource_provider_->ReceiveFromChild(child_id, list);
    {
      resource_provider_->WaitSyncToken(list[0].id);
      DisplayResourceProvider::ScopedReadLockGL lock(resource_provider_.get(),
                                                     list[0].id);
    }

    EXPECT_EQ(list[0].mailbox_holder.sync_token,
              context3d_->last_waited_sync_token());
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  EXPECT_EQ(2u, resource_provider_->num_resources());
  ResourceProvider::ResourceIdMap resource_map =
      resource_provider_->GetChildToParentMap(child_id);
  viz::ResourceId mapped_id1 = resource_map[id1];
  viz::ResourceId mapped_id2 = resource_map[id2];
  EXPECT_NE(0u, mapped_id1);
  EXPECT_NE(0u, mapped_id2);

  // Make sure that the request for a promotion hint was noticed.
  EXPECT_TRUE(resource_provider_->IsOverlayCandidate(mapped_id1));
  EXPECT_TRUE(resource_provider_->IsBackedBySurfaceTexture(mapped_id1));
  EXPECT_TRUE(resource_provider_->WantsPromotionHint(mapped_id1));

  EXPECT_TRUE(resource_provider_->IsOverlayCandidate(mapped_id2));
  EXPECT_FALSE(resource_provider_->IsBackedBySurfaceTexture(mapped_id2));
  EXPECT_FALSE(resource_provider_->WantsPromotionHint(mapped_id2));

  EXPECT_EQ(1u, resource_provider_->CountPromotionHintRequestsForTesting());

  // ResourceProvider maintains a set of promotion hint requests that should be
  // cleared when resources are deleted.
  resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                    viz::ResourceIdSet());
  EXPECT_EQ(2u, returned_to_child.size());
  child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);

  EXPECT_EQ(0u, resource_provider_->CountPromotionHintRequestsForTesting());

  resource_provider_->DestroyChild(child_id);
}
#endif

class ResourceProviderTestNoSyncToken : public ResourceProviderTest {
 public:
  ResourceProviderTestNoSyncToken() : ResourceProviderTest(false) {
    EXPECT_EQ(ResourceProvider::RESOURCE_TYPE_GL_TEXTURE, GetParam());
  }
};

TEST_P(ResourceProviderTestNoSyncToken, TransferGLResources) {
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = {1, 2, 3, 4};
  child_resource_provider_->CopyToResource(id1, data1, size);

  viz::ResourceId id2 = child_resource_provider_->CreateGpuMemoryBufferResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
      gfx::BufferUsage::GPU_READ_CPU_READ_WRITE, gfx::ColorSpace());
  {
    // Ensure locking the memory buffer doesn't create an unnecessary sync
    // point.
    LayerTreeResourceProvider::ScopedWriteLockGpuMemoryBuffer lock(
        child_resource_provider_.get(), id2);
    EXPECT_TRUE(lock.GetGpuMemoryBuffer());
  }

  GLuint external_texture_id = child_context_->createExternalTexture();

  // A sync point is specified directly and should be used.
  gpu::Mailbox external_mailbox;
  child_context_->genMailboxCHROMIUM(external_mailbox.name);
  child_context_->produceTextureDirectCHROMIUM(
      external_texture_id, GL_TEXTURE_EXTERNAL_OES, external_mailbox.name);
  gpu::SyncToken external_sync_token;
  child_context_->genSyncToken(child_context_->insertFenceSync(),
                               external_sync_token.GetData());
  EXPECT_TRUE(external_sync_token.HasData());
  viz::ResourceId id3 =
      child_resource_provider_->CreateResourceFromTextureMailbox(
          viz::TextureMailbox(external_mailbox, external_sync_token,
                              GL_TEXTURE_EXTERNAL_OES),
          viz::SingleReleaseCallback::Create(
              base::Bind(&EmptyReleaseCallback)));

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  resource_provider_->SetChildNeedsSyncTokens(child_id, false);
  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);
    resource_ids_to_transfer.push_back(id3);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(3u, list.size());
    // Standard resources shouldn't require creating and sending a sync point.
    EXPECT_FALSE(list[0].mailbox_holder.sync_token.HasData());
    EXPECT_FALSE(list[1].mailbox_holder.sync_token.HasData());
    // A given sync point should be passed through.
    EXPECT_EQ(external_sync_token, list[2].mailbox_holder.sync_token);
    resource_provider_->ReceiveFromChild(child_id, list);

    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_ids_to_receive.insert(id3);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  {
    EXPECT_EQ(0u, returned_to_child.size());

    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);

    ASSERT_EQ(3u, returned_to_child.size());
    std::map<viz::ResourceId, gpu::SyncToken> returned_sync_tokens;
    for (const auto& returned : returned_to_child)
      returned_sync_tokens[returned.id] = returned.sync_token;

    ASSERT_TRUE(returned_sync_tokens.find(id1) != returned_sync_tokens.end());
    // No new sync point should be created transferring back.
    ASSERT_TRUE(returned_sync_tokens.find(id1) != returned_sync_tokens.end());
    EXPECT_FALSE(returned_sync_tokens[id1].HasData());
    ASSERT_TRUE(returned_sync_tokens.find(id2) != returned_sync_tokens.end());
    EXPECT_FALSE(returned_sync_tokens[id2].HasData());
    // Original sync point given should be returned.
    ASSERT_TRUE(returned_sync_tokens.find(id3) != returned_sync_tokens.end());
    EXPECT_EQ(external_sync_token, returned_sync_tokens[id3]);
    EXPECT_FALSE(returned_to_child[0].lost);
    EXPECT_FALSE(returned_to_child[1].lost);
    EXPECT_FALSE(returned_to_child[2].lost);
    child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
    returned_to_child.clear();
  }

  resource_provider_->DestroyChild(child_id);
}

INSTANTIATE_TEST_CASE_P(
    ResourceProviderTests,
    ResourceProviderTestNoSyncToken,
    ::testing::Values(ResourceProvider::RESOURCE_TYPE_GL_TEXTURE));

// Test that SetBatchReturnResources batching works.
TEST_P(ResourceProviderTest, SetBatchPreventsReturn) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  uint8_t data1[4] = {1, 2, 3, 4};
  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));

  // Transfer some resources to the parent.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  viz::ResourceId ids[2];
  for (size_t i = 0; i < arraysize(ids); i++) {
    ids[i] = child_resource_provider_->CreateResource(
        size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
        gfx::ColorSpace());
    child_resource_provider_->CopyToResource(ids[i], data1, size);
    resource_ids_to_transfer.push_back(ids[i]);
  }

  std::vector<viz::TransferableResource> list;
  child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                &list);
  ASSERT_EQ(2u, list.size());
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(ids[0]));
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(ids[1]));

  resource_provider_->ReceiveFromChild(child_id, list);

  std::vector<std::unique_ptr<DisplayResourceProvider::ScopedReadLockGL>>
      read_locks;
  for (auto& parent_resource : list) {
    resource_provider_->WaitSyncToken(parent_resource.id);
    read_locks.push_back(
        std::make_unique<DisplayResourceProvider::ScopedReadLockGL>(
            resource_provider_.get(), parent_resource.id));
  }

  resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                    viz::ResourceIdSet());
  std::unique_ptr<DisplayResourceProvider::ScopedBatchReturnResources>
      returner =
          std::make_unique<DisplayResourceProvider::ScopedBatchReturnResources>(
              resource_provider_.get());
  EXPECT_EQ(0u, returned_to_child.size());

  read_locks.clear();
  EXPECT_EQ(0u, returned_to_child.size());

  returner.reset();
  EXPECT_EQ(2u, returned_to_child.size());
  // All resources in a batch should share a sync token.
  EXPECT_EQ(returned_to_child[0].sync_token, returned_to_child[1].sync_token);

  child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
  child_resource_provider_->DeleteResource(ids[0]);
  child_resource_provider_->DeleteResource(ids[1]);
  EXPECT_EQ(0u, child_resource_provider_->num_resources());
}

TEST_P(ResourceProviderTest, ReadLockCountStopsReturnToChildOrDelete) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = {1, 2, 3, 4};
  child_resource_provider_->CopyToResource(id1, data1, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(1u, list.size());
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));

    resource_provider_->ReceiveFromChild(child_id, list);

    resource_provider_->WaitSyncToken(list[0].id);
    DisplayResourceProvider::ScopedReadLockGL lock(resource_provider_.get(),
                                                   list[0].id);

    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      viz::ResourceIdSet());
    EXPECT_EQ(0u, returned_to_child.size());
  }

  EXPECT_EQ(1u, returned_to_child.size());
  child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);

  child_resource_provider_->WaitSyncToken(id1);
  EXPECT_EQ(1u, child_resource_provider_->num_resources());
  child_resource_provider_->DeleteResource(id1);
  EXPECT_EQ(0u, child_resource_provider_->num_resources());

  resource_provider_->DestroyChild(child_id);
}

class TestFence : public ResourceProvider::Fence {
 public:
  TestFence() {}

  void Set() override {}
  bool HasPassed() override { return passed; }
  void Wait() override {}

  bool passed = false;

 private:
  ~TestFence() override {}
};

TEST_P(ResourceProviderTest, ReadLockFenceStopsReturnToChildOrDelete) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = {1, 2, 3, 4};
  child_resource_provider_->CopyToResource(id1, data1, size);
  child_resource_provider_->EnableReadLockFencesForTesting(id1);
  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));

  // Transfer some resources to the parent.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  resource_ids_to_transfer.push_back(id1);

  std::vector<viz::TransferableResource> list;
  child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                &list);
  ASSERT_EQ(1u, list.size());
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
  EXPECT_TRUE(list[0].read_lock_fences_enabled);

  resource_provider_->ReceiveFromChild(child_id, list);

  scoped_refptr<TestFence> fence(new TestFence);
  resource_provider_->SetReadLockFence(fence.get());
  {
    unsigned parent_id = list.front().id;
    resource_provider_->WaitSyncToken(parent_id);
    DisplayResourceProvider::ScopedReadLockGL lock(resource_provider_.get(),
                                                   parent_id);
  }
  resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                    viz::ResourceIdSet());
  EXPECT_EQ(0u, returned_to_child.size());

  resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                    viz::ResourceIdSet());
  EXPECT_EQ(0u, returned_to_child.size());
  fence->passed = true;

  resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                    viz::ResourceIdSet());
  EXPECT_EQ(1u, returned_to_child.size());

  child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
  child_resource_provider_->DeleteResource(id1);
  EXPECT_EQ(0u, child_resource_provider_->num_resources());
}

TEST_P(ResourceProviderTest, ReadLockFenceDestroyChild) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data[4] = {1, 2, 3, 4};
  child_resource_provider_->CopyToResource(id1, data, size);
  child_resource_provider_->EnableReadLockFencesForTesting(id1);

  viz::ResourceId id2 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  child_resource_provider_->CopyToResource(id2, data, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));

  // Transfer resources to the parent.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  resource_ids_to_transfer.push_back(id1);
  resource_ids_to_transfer.push_back(id2);

  std::vector<viz::TransferableResource> list;
  child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                &list);
  ASSERT_EQ(2u, list.size());
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));

  resource_provider_->ReceiveFromChild(child_id, list);

  scoped_refptr<TestFence> fence(new TestFence);
  resource_provider_->SetReadLockFence(fence.get());
  {
    for (size_t i = 0; i < list.size(); i++) {
      unsigned parent_id = list[i].id;
      resource_provider_->WaitSyncToken(parent_id);
      DisplayResourceProvider::ScopedReadLockGL lock(resource_provider_.get(),
                                                     parent_id);
    }
  }
  EXPECT_EQ(0u, returned_to_child.size());

  EXPECT_EQ(2u, resource_provider_->num_resources());

  resource_provider_->DestroyChild(child_id);

  EXPECT_EQ(0u, resource_provider_->num_resources());
  EXPECT_EQ(2u, returned_to_child.size());

  // id1 should be lost and id2 should not.
  EXPECT_EQ(returned_to_child[0].lost, returned_to_child[0].id == id1);
  EXPECT_EQ(returned_to_child[1].lost, returned_to_child[1].id == id1);

  child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
  child_resource_provider_->DeleteResource(id1);
  child_resource_provider_->DeleteResource(id2);
  EXPECT_EQ(0u, child_resource_provider_->num_resources());
}

TEST_P(ResourceProviderTest, ReadLockFenceContextLost) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data[4] = {1, 2, 3, 4};
  child_resource_provider_->CopyToResource(id1, data, size);
  child_resource_provider_->EnableReadLockFencesForTesting(id1);

  viz::ResourceId id2 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  child_resource_provider_->CopyToResource(id2, data, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));

  // Transfer resources to the parent.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  resource_ids_to_transfer.push_back(id1);
  resource_ids_to_transfer.push_back(id2);

  std::vector<viz::TransferableResource> list;
  child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                &list);
  ASSERT_EQ(2u, list.size());
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
  EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));

  resource_provider_->ReceiveFromChild(child_id, list);

  scoped_refptr<TestFence> fence(new TestFence);
  resource_provider_->SetReadLockFence(fence.get());
  {
    for (size_t i = 0; i < list.size(); i++) {
      unsigned parent_id = list[i].id;
      resource_provider_->WaitSyncToken(parent_id);
      DisplayResourceProvider::ScopedReadLockGL lock(resource_provider_.get(),
                                                     parent_id);
    }
  }
  EXPECT_EQ(0u, returned_to_child.size());

  EXPECT_EQ(2u, resource_provider_->num_resources());
  resource_provider_->DidLoseVulkanContextProvider();
  resource_provider_ = nullptr;

  EXPECT_EQ(2u, returned_to_child.size());

  EXPECT_TRUE(returned_to_child[0].lost);
  EXPECT_TRUE(returned_to_child[1].lost);
}

TEST_P(ResourceProviderTest, TransferSoftwareResources) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_BITMAP)
    return;

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = { 1, 2, 3, 4 };
  child_resource_provider_->CopyToResource(id1, data1, size);

  viz::ResourceId id2 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data2[4] = { 5, 5, 5, 5 };
  child_resource_provider_->CopyToResource(id2, data2, size);

  std::unique_ptr<viz::SharedBitmap> shared_bitmap(CreateAndFillSharedBitmap(
      shared_bitmap_manager_.get(), gfx::Size(1, 1), 0));
  viz::SharedBitmap* shared_bitmap_ptr = shared_bitmap.get();
  viz::ResourceId id3 =
      child_resource_provider_->CreateResourceFromTextureMailbox(
          viz::TextureMailbox(shared_bitmap_ptr, gfx::Size(1, 1)),
          viz::SingleReleaseCallback::Create(base::Bind(
              &SharedBitmapReleaseCallback, base::Passed(&shared_bitmap))));

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);
    resource_ids_to_transfer.push_back(id3);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(3u, list.size());
    EXPECT_FALSE(list[0].mailbox_holder.sync_token.HasData());
    EXPECT_FALSE(list[1].mailbox_holder.sync_token.HasData());
    EXPECT_FALSE(list[2].mailbox_holder.sync_token.HasData());
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id3));
    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_ids_to_receive.insert(id3);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  EXPECT_EQ(3u, resource_provider_->num_resources());
  ResourceProvider::ResourceIdMap resource_map =
      resource_provider_->GetChildToParentMap(child_id);
  viz::ResourceId mapped_id1 = resource_map[id1];
  viz::ResourceId mapped_id2 = resource_map[id2];
  viz::ResourceId mapped_id3 = resource_map[id3];
  EXPECT_NE(0u, mapped_id1);
  EXPECT_NE(0u, mapped_id2);
  EXPECT_NE(0u, mapped_id3);
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id1));
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id2));
  EXPECT_FALSE(resource_provider_->InUseByConsumer(id3));

  uint8_t result[4] = { 0 };
  GetResourcePixels(
      resource_provider_.get(), context(), mapped_id1, size, format, result);
  EXPECT_EQ(0, memcmp(data1, result, pixel_size));

  GetResourcePixels(
      resource_provider_.get(), context(), mapped_id2, size, format, result);
  EXPECT_EQ(0, memcmp(data2, result, pixel_size));

  {
    // Check that transfering again the same resource from the child to the
    // parent works.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    EXPECT_EQ(2u, list.size());
    EXPECT_EQ(id1, list[0].id);
    EXPECT_EQ(id2, list[1].id);
    std::vector<viz::ReturnedResource> returned =
        viz::TransferableResource::ReturnResources(list);
    child_resource_provider_->ReceiveReturnsFromParent(returned);
    // ids were exported twice, we returned them only once, they should still
    // be in-use.
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
  }
  {
    EXPECT_EQ(0u, returned_to_child.size());

    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);

    ASSERT_EQ(3u, returned_to_child.size());
    EXPECT_FALSE(returned_to_child[0].sync_token.HasData());
    EXPECT_FALSE(returned_to_child[1].sync_token.HasData());
    EXPECT_FALSE(returned_to_child[2].sync_token.HasData());
    std::set<viz::ResourceId> expected_ids;
    expected_ids.insert(id1);
    expected_ids.insert(id2);
    expected_ids.insert(id3);
    std::set<viz::ResourceId> returned_ids;
    for (unsigned i = 0; i < 3; i++)
      returned_ids.insert(returned_to_child[i].id);
    EXPECT_EQ(expected_ids, returned_ids);
    EXPECT_FALSE(returned_to_child[0].lost);
    EXPECT_FALSE(returned_to_child[1].lost);
    EXPECT_FALSE(returned_to_child[2].lost);
    child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
    returned_to_child.clear();
  }
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id1));
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id2));
  EXPECT_FALSE(child_resource_provider_->InUseByConsumer(id3));

  {
    ResourceProvider::ScopedWriteLockSoftware lock(
        child_resource_provider_.get(), id1);
    const SkBitmap sk_bitmap = lock.sk_bitmap();
    EXPECT_EQ(sk_bitmap.width(), size.width());
    EXPECT_EQ(sk_bitmap.height(), size.height());
    EXPECT_EQ(0, memcmp(data1, sk_bitmap.getPixels(), pixel_size));
  }
  {
    ResourceProvider::ScopedWriteLockSoftware lock(
        child_resource_provider_.get(), id2);
    const SkBitmap sk_bitmap = lock.sk_bitmap();
    EXPECT_EQ(sk_bitmap.width(), size.width());
    EXPECT_EQ(sk_bitmap.height(), size.height());
    EXPECT_EQ(0, memcmp(data2, sk_bitmap.getPixels(), pixel_size));
  }
  {
    // Transfer resources to the parent again.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);
    resource_ids_to_transfer.push_back(id3);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(3u, list.size());
    EXPECT_EQ(id1, list[0].id);
    EXPECT_EQ(id2, list[1].id);
    EXPECT_EQ(id3, list[2].id);
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id3));
    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_ids_to_receive.insert(id3);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  EXPECT_EQ(0u, returned_to_child.size());

  EXPECT_EQ(3u, resource_provider_->num_resources());
  resource_provider_->DestroyChild(child_id);
  EXPECT_EQ(0u, resource_provider_->num_resources());

  ASSERT_EQ(3u, returned_to_child.size());
  EXPECT_FALSE(returned_to_child[0].sync_token.HasData());
  EXPECT_FALSE(returned_to_child[1].sync_token.HasData());
  EXPECT_FALSE(returned_to_child[2].sync_token.HasData());
  std::set<viz::ResourceId> expected_ids;
  expected_ids.insert(id1);
  expected_ids.insert(id2);
  expected_ids.insert(id3);
  std::set<viz::ResourceId> returned_ids;
  for (unsigned i = 0; i < 3; i++)
    returned_ids.insert(returned_to_child[i].id);
  EXPECT_EQ(expected_ids, returned_ids);
  EXPECT_FALSE(returned_to_child[0].lost);
  EXPECT_FALSE(returned_to_child[1].lost);
  EXPECT_FALSE(returned_to_child[2].lost);
}

TEST_P(ResourceProviderTest, TransferGLToSoftware) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_BITMAP)
    return;

  scoped_refptr<TestContextProvider> child_context_provider =
      TestContextProvider::Create(
          ResourceProviderContext::Create(shared_data_.get()));
  child_context_provider->BindToCurrentThread();

  auto child_resource_provider(std::make_unique<LayerTreeResourceProvider>(
      child_context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id1 = child_resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = { 1, 2, 3, 4 };
  child_resource_provider->CopyToResource(id1, data1, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);

    std::vector<viz::TransferableResource> list;
    child_resource_provider->PrepareSendToParent(resource_ids_to_transfer,
                                                 &list);
    ASSERT_EQ(1u, list.size());
    EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());
    EXPECT_EQ(static_cast<GLenum>(GL_TEXTURE_2D),
              list[0].mailbox_holder.texture_target);
    EXPECT_TRUE(child_resource_provider->InUseByConsumer(id1));
    resource_provider_->ReceiveFromChild(child_id, list);
  }

  EXPECT_EQ(0u, resource_provider_->num_resources());
  ASSERT_EQ(1u, returned_to_child.size());
  EXPECT_EQ(returned_to_child[0].id, id1);
  ResourceProvider::ResourceIdMap resource_map =
      resource_provider_->GetChildToParentMap(child_id);
  viz::ResourceId mapped_id1 = resource_map[id1];
  EXPECT_EQ(0u, mapped_id1);

  resource_provider_->DestroyChild(child_id);
  EXPECT_EQ(0u, resource_provider_->num_resources());

  ASSERT_EQ(1u, returned_to_child.size());
  EXPECT_FALSE(returned_to_child[0].lost);
}

TEST_P(ResourceProviderTest, TransferInvalidSoftware) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_BITMAP)
    return;

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = { 1, 2, 3, 4 };
  child_resource_provider_->CopyToResource(id1, data1, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(1u, list.size());
    // Make invalid.
    list[0].mailbox_holder.mailbox.name[1] ^= 0xff;
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    resource_provider_->ReceiveFromChild(child_id, list);
  }

  EXPECT_EQ(1u, resource_provider_->num_resources());
  EXPECT_EQ(0u, returned_to_child.size());

  ResourceProvider::ResourceIdMap resource_map =
      resource_provider_->GetChildToParentMap(child_id);
  viz::ResourceId mapped_id1 = resource_map[id1];
  EXPECT_NE(0u, mapped_id1);
  {
    DisplayResourceProvider::ScopedReadLockSoftware lock(
        resource_provider_.get(), mapped_id1);
    EXPECT_FALSE(lock.valid());
  }

  resource_provider_->DestroyChild(child_id);
  EXPECT_EQ(0u, resource_provider_->num_resources());

  ASSERT_EQ(1u, returned_to_child.size());
  EXPECT_FALSE(returned_to_child[0].lost);
}

TEST_P(ResourceProviderTest, DeleteExportedResources) {
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = { 1, 2, 3, 4 };
  child_resource_provider_->CopyToResource(id1, data1, size);

  viz::ResourceId id2 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data2[4] = {5, 5, 5, 5};
  child_resource_provider_->CopyToResource(id2, data2, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(2u, list.size());
    if (GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE) {
      EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());
      EXPECT_TRUE(list[1].mailbox_holder.sync_token.HasData());
    }
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);

    std::vector<viz::ReturnedResource> returned =
        viz::TransferableResource::ReturnResources(list);
    child_resource_provider_->ReceiveReturnsFromParent(returned);
    EXPECT_EQ(0u, returned_to_child.size());
    EXPECT_EQ(2u, resource_provider_->num_resources());

    // Return the resources from the parent, it should be returned at this
    // point.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);
    EXPECT_EQ(2u, returned_to_child.size());
    EXPECT_EQ(0u, resource_provider_->num_resources());
  }
}

TEST_P(ResourceProviderTest, DestroyChildWithExportedResources) {
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id1 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data1[4] = {1, 2, 3, 4};
  child_resource_provider_->CopyToResource(id1, data1, size);

  viz::ResourceId id2 = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data2[4] = {5, 5, 5, 5};
  child_resource_provider_->CopyToResource(id2, data2, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer some resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id1);
    resource_ids_to_transfer.push_back(id2);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(2u, list.size());
    if (GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE) {
      EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());
      EXPECT_TRUE(list[1].mailbox_holder.sync_token.HasData());
    }
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id1));
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id2));
    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id1);
    resource_ids_to_receive.insert(id2);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);

    EXPECT_EQ(0u, returned_to_child.size());
    EXPECT_EQ(2u, resource_provider_->num_resources());
    // Destroy the child, the resources should be returned.
    resource_provider_->DestroyChild(child_id);
    EXPECT_EQ(2u, returned_to_child.size());
    EXPECT_EQ(0u, resource_provider_->num_resources());
  }
}

TEST_P(ResourceProviderTest, DeleteTransferredResources) {
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  size_t pixel_size = TextureSizeBytes(size, format);
  ASSERT_EQ(4U, pixel_size);

  viz::ResourceId id = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  uint8_t data[4] = { 1, 2, 3, 4 };
  child_resource_provider_->CopyToResource(id, data, size);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer some resource to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(id);

    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(1u, list.size());
    if (GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
      EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());
    EXPECT_TRUE(child_resource_provider_->InUseByConsumer(id));
    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(id);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  // Delete textures in the child, while they are transfered.
  child_resource_provider_->DeleteResource(id);
  EXPECT_EQ(1u, child_resource_provider_->num_resources());
  {
    EXPECT_EQ(0u, returned_to_child.size());

    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);

    ASSERT_EQ(1u, returned_to_child.size());
    if (GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
      EXPECT_TRUE(returned_to_child[0].sync_token.HasData());
    child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
  }
  EXPECT_EQ(0u, child_resource_provider_->num_resources());
}

class ResourceProviderTestTextureFilters : public ResourceProviderTest {
 public:
  static void RunTest(GLenum child_filter, GLenum parent_filter) {
    auto child_context_owned(std::make_unique<TextureStateTrackingContext>());
    TextureStateTrackingContext* child_context = child_context_owned.get();

    auto child_context_provider =
        TestContextProvider::Create(std::move(child_context_owned));
    child_context_provider->BindToCurrentThread();
    auto shared_bitmap_manager = std::make_unique<TestSharedBitmapManager>();

    viz::ResourceSettings resource_settings = CreateResourceSettings();
    auto child_resource_provider(std::make_unique<LayerTreeResourceProvider>(
        child_context_provider.get(), shared_bitmap_manager.get(), nullptr,
        kDelegatedSyncPointsRequired, resource_settings));

    auto parent_context_owned(std::make_unique<TextureStateTrackingContext>());
    TextureStateTrackingContext* parent_context = parent_context_owned.get();

    auto parent_context_provider =
        TestContextProvider::Create(std::move(parent_context_owned));
    parent_context_provider->BindToCurrentThread();

    auto parent_resource_provider(std::make_unique<DisplayResourceProvider>(
        parent_context_provider.get(), shared_bitmap_manager.get(), nullptr,
        kDelegatedSyncPointsRequired, resource_settings));

    gfx::Size size(1, 1);
    viz::ResourceFormat format = viz::RGBA_8888;
    int child_texture_id = 1;
    int parent_texture_id = 2;

    size_t pixel_size = TextureSizeBytes(size, format);
    ASSERT_EQ(4U, pixel_size);

    viz::ResourceId id = child_resource_provider->CreateResource(
        size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
        gfx::ColorSpace());

    // The new texture is created with GL_LINEAR.
    EXPECT_CALL(*child_context, bindTexture(GL_TEXTURE_2D, child_texture_id))
        .Times(2);  // Once to create and once to allocate.
    EXPECT_CALL(*child_context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(*child_context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    EXPECT_CALL(
        *child_context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    EXPECT_CALL(
        *child_context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    child_resource_provider->AllocateForTesting(id);
    Mock::VerifyAndClearExpectations(child_context);

    uint8_t data[4] = { 1, 2, 3, 4 };

    EXPECT_CALL(*child_context, bindTexture(GL_TEXTURE_2D, child_texture_id));
    child_resource_provider->CopyToResource(id, data, size);
    Mock::VerifyAndClearExpectations(child_context);

    std::vector<viz::ReturnedResource> returned_to_child;
    int child_id = parent_resource_provider->CreateChild(
        GetReturnCallback(&returned_to_child));
    {
      // Transfer some resource to the parent.
      ResourceProvider::ResourceIdArray resource_ids_to_transfer;
      resource_ids_to_transfer.push_back(id);
      std::vector<viz::TransferableResource> list;

      EXPECT_CALL(*child_context,
                  produceTextureDirectCHROMIUM(_, GL_TEXTURE_2D, _));

      child_resource_provider->PrepareSendToParent(resource_ids_to_transfer,
                                                   &list);
      Mock::VerifyAndClearExpectations(child_context);

      ASSERT_EQ(1u, list.size());
      EXPECT_EQ(static_cast<unsigned>(child_filter), list[0].filter);

      EXPECT_CALL(*parent_context,
                  createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, _))
          .WillOnce(Return(parent_texture_id));

      parent_resource_provider->ReceiveFromChild(child_id, list);
      {
        parent_resource_provider->WaitSyncToken(list[0].id);
        DisplayResourceProvider::ScopedReadLockGL lock(
            parent_resource_provider.get(), list[0].id);
      }
      Mock::VerifyAndClearExpectations(parent_context);

      viz::ResourceIdSet resource_ids_to_receive;
      resource_ids_to_receive.insert(id);
      parent_resource_provider->DeclareUsedResourcesFromChild(
          child_id, resource_ids_to_receive);
      Mock::VerifyAndClearExpectations(parent_context);
    }
    ResourceProvider::ResourceIdMap resource_map =
        parent_resource_provider->GetChildToParentMap(child_id);
    viz::ResourceId mapped_id = resource_map[id];
    EXPECT_NE(0u, mapped_id);

    // The texture is set to |parent_filter| in the parent.
    EXPECT_CALL(*parent_context, bindTexture(GL_TEXTURE_2D, parent_texture_id));
    EXPECT_CALL(
        *parent_context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, parent_filter));
    EXPECT_CALL(
        *parent_context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, parent_filter));
    SetResourceFilter(parent_resource_provider.get(), mapped_id, parent_filter);
    Mock::VerifyAndClearExpectations(parent_context);

    // The texture should be reset to |child_filter| in the parent when it is
    // returned, since that is how it was received.
    EXPECT_CALL(*parent_context, bindTexture(GL_TEXTURE_2D, parent_texture_id));
    EXPECT_CALL(*parent_context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(*parent_context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    {
      EXPECT_EQ(0u, returned_to_child.size());

      // Transfer resources back from the parent to the child. Set no resources
      // as being in use.
      viz::ResourceIdSet no_resources;
      parent_resource_provider->DeclareUsedResourcesFromChild(child_id,
                                                              no_resources);
      Mock::VerifyAndClearExpectations(parent_context);

      ASSERT_EQ(1u, returned_to_child.size());
      child_resource_provider->ReceiveReturnsFromParent(returned_to_child);
    }
  }
};

TEST_P(ResourceProviderTest, TextureFilters_ChildLinearParentNearest) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  ResourceProviderTestTextureFilters::RunTest(GL_LINEAR, GL_NEAREST);
}

TEST_P(ResourceProviderTest, TransferMailboxResources) {
  // Other mailbox transfers tested elsewhere.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  unsigned texture = context()->createTexture();
  context()->bindTexture(GL_TEXTURE_2D, texture);
  uint8_t data[4] = { 1, 2, 3, 4 };
  context()->texImage2D(
      GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  gpu::Mailbox mailbox;
  context()->genMailboxCHROMIUM(mailbox.name);
  context()->produceTextureDirectCHROMIUM(texture, GL_TEXTURE_2D, mailbox.name);
  gpu::SyncToken sync_token;
  context()->genSyncToken(context()->insertFenceSync(), sync_token.GetData());
  EXPECT_TRUE(sync_token.HasData());

  // All the logic below assumes that the sync token releases are all positive.
  EXPECT_LT(0u, sync_token.release_count());

  gpu::SyncToken release_sync_token;
  bool lost_resource = false;
  viz::ReleaseCallback callback =
      base::Bind(ReleaseCallback, &release_sync_token, &lost_resource);
  viz::ResourceId resource =
      child_resource_provider_->CreateResourceFromTextureMailbox(
          viz::TextureMailbox(mailbox, sync_token, GL_TEXTURE_2D),
          viz::SingleReleaseCallback::Create(callback));
  EXPECT_EQ(1u, context()->NumTextures());
  EXPECT_FALSE(release_sync_token.HasData());
  {
    // Transfer the resource, expect the sync points to be consistent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(resource);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(1u, list.size());
    EXPECT_LE(sync_token.release_count(),
              list[0].mailbox_holder.sync_token.release_count());
    EXPECT_EQ(0,
              memcmp(mailbox.name,
                     list[0].mailbox_holder.mailbox.name,
                     sizeof(mailbox.name)));
    EXPECT_FALSE(release_sync_token.HasData());

    context()->waitSyncToken(list[0].mailbox_holder.sync_token.GetConstData());
    unsigned other_texture =
        context()->createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, mailbox.name);
    uint8_t test_data[4] = { 0 };
    context()->GetPixels(gfx::Size(1, 1), viz::RGBA_8888, test_data);
    EXPECT_EQ(0, memcmp(data, test_data, sizeof(data)));

    context()->produceTextureDirectCHROMIUM(other_texture, GL_TEXTURE_2D,
                                            mailbox.name);
    context()->deleteTexture(other_texture);
    context()->genSyncToken(context()->insertFenceSync(),
                            list[0].mailbox_holder.sync_token.GetData());
    EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());

    // Receive the resource, then delete it, expect the sync points to be
    // consistent.
    std::vector<viz::ReturnedResource> returned =
        viz::TransferableResource::ReturnResources(list);
    child_resource_provider_->ReceiveReturnsFromParent(returned);
    EXPECT_EQ(1u, context()->NumTextures());
    EXPECT_FALSE(release_sync_token.HasData());

    child_resource_provider_->DeleteResource(resource);
    EXPECT_LE(list[0].mailbox_holder.sync_token.release_count(),
              release_sync_token.release_count());
    EXPECT_FALSE(lost_resource);
  }

  // We're going to do the same thing as above, but testing the case where we
  // delete the resource before we receive it back.
  sync_token = release_sync_token;
  EXPECT_LT(0u, sync_token.release_count());
  release_sync_token.Clear();
  resource = child_resource_provider_->CreateResourceFromTextureMailbox(
      viz::TextureMailbox(mailbox, sync_token, GL_TEXTURE_2D),
      viz::SingleReleaseCallback::Create(callback));
  EXPECT_EQ(1u, context()->NumTextures());
  EXPECT_FALSE(release_sync_token.HasData());
  {
    // Transfer the resource, expect the sync points to be consistent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(resource);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    ASSERT_EQ(1u, list.size());
    EXPECT_LE(sync_token.release_count(),
              list[0].mailbox_holder.sync_token.release_count());
    EXPECT_EQ(0,
              memcmp(mailbox.name,
                     list[0].mailbox_holder.mailbox.name,
                     sizeof(mailbox.name)));
    EXPECT_FALSE(release_sync_token.HasData());

    context()->waitSyncToken(list[0].mailbox_holder.sync_token.GetConstData());
    unsigned other_texture =
        context()->createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, mailbox.name);
    uint8_t test_data[4] = { 0 };
    context()->GetPixels(gfx::Size(1, 1), viz::RGBA_8888, test_data);
    EXPECT_EQ(0, memcmp(data, test_data, sizeof(data)));

    context()->produceTextureDirectCHROMIUM(other_texture, GL_TEXTURE_2D,
                                            mailbox.name);
    context()->deleteTexture(other_texture);
    context()->genSyncToken(context()->insertFenceSync(),
                            list[0].mailbox_holder.sync_token.GetData());
    EXPECT_TRUE(list[0].mailbox_holder.sync_token.HasData());

    // Delete the resource, which shouldn't do anything.
    child_resource_provider_->DeleteResource(resource);
    EXPECT_EQ(1u, context()->NumTextures());
    EXPECT_FALSE(release_sync_token.HasData());

    // Then receive the resource which should release the mailbox, expect the
    // sync points to be consistent.
    std::vector<viz::ReturnedResource> returned =
        viz::TransferableResource::ReturnResources(list);
    child_resource_provider_->ReceiveReturnsFromParent(returned);
    EXPECT_LE(list[0].mailbox_holder.sync_token.release_count(),
              release_sync_token.release_count());
    EXPECT_FALSE(lost_resource);
  }

  context()->waitSyncToken(release_sync_token.GetConstData());
  texture =
      context()->createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, mailbox.name);
  context()->deleteTexture(texture);
}

TEST_P(ResourceProviderTest, LostResourceInParent) {
  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  viz::ResourceId resource = child_resource_provider_->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  child_resource_provider_->AllocateForTesting(resource);
  // Expect a GL resource to be lost.
  bool should_lose_resource =
      GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE;

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer the resource to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(resource);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    EXPECT_EQ(1u, list.size());

    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(resource);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  // Lose the output surface in the parent.
  resource_provider_->DidLoseVulkanContextProvider();

  {
    EXPECT_EQ(0u, returned_to_child.size());

    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);

    // Expect a GL resource to be lost.
    ASSERT_EQ(1u, returned_to_child.size());
    EXPECT_EQ(should_lose_resource, returned_to_child[0].lost);
    child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
    returned_to_child.clear();
  }

  // A GL resource should be lost.
  EXPECT_EQ(should_lose_resource, child_resource_provider_->IsLost(resource));

  // Lost resources stay in use in the parent forever.
  EXPECT_EQ(should_lose_resource,
            child_resource_provider_->InUseByConsumer(resource));
}


TEST_P(ResourceProviderTest, LostMailboxInParent) {
  gpu::SyncToken release_sync_token;
  bool lost_resource = false;
  bool release_called = false;
  gpu::SyncToken sync_token;
  viz::ResourceId resource = CreateChildMailbox(
      &release_sync_token, &lost_resource, &release_called, &sync_token);

  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id =
      resource_provider_->CreateChild(GetReturnCallback(&returned_to_child));
  {
    // Transfer the resource to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(resource);
    std::vector<viz::TransferableResource> list;
    child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                  &list);
    EXPECT_EQ(1u, list.size());

    resource_provider_->ReceiveFromChild(child_id, list);
    viz::ResourceIdSet resource_ids_to_receive;
    resource_ids_to_receive.insert(resource);
    resource_provider_->DeclareUsedResourcesFromChild(child_id,
                                                      resource_ids_to_receive);
  }

  // Lose the output surface in the parent.
  resource_provider_->DidLoseVulkanContextProvider();

  {
    EXPECT_EQ(0u, returned_to_child.size());

    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    viz::ResourceIdSet no_resources;
    resource_provider_->DeclareUsedResourcesFromChild(child_id, no_resources);

    ASSERT_EQ(1u, returned_to_child.size());
    // Losing an output surface only loses hardware resources.
    EXPECT_EQ(returned_to_child[0].lost,
              GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE);
    child_resource_provider_->ReceiveReturnsFromParent(returned_to_child);
    returned_to_child.clear();
  }

  // Delete the resource in the child. Expect the resource to be lost if it's
  // a GL texture.
  child_resource_provider_->DeleteResource(resource);
  EXPECT_EQ(lost_resource,
            GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE);
}

TEST_P(ResourceProviderTest, Shutdown) {
  gpu::SyncToken release_sync_token;
  bool lost_resource = false;
  bool release_called = false;
  gpu::SyncToken sync_token;
  CreateChildMailbox(&release_sync_token, &lost_resource, &release_called,
                     &sync_token);

  EXPECT_FALSE(release_sync_token.HasData());
  EXPECT_FALSE(lost_resource);

  child_resource_provider_ = nullptr;

  if (GetParam() == ResourceProvider::RESOURCE_TYPE_GL_TEXTURE) {
    EXPECT_LE(sync_token.release_count(), release_sync_token.release_count());
  }
  EXPECT_TRUE(release_called);
  EXPECT_FALSE(lost_resource);
}

TEST_P(ResourceProviderTest, ShutdownWithExportedResource) {
  gpu::SyncToken release_sync_token;
  bool lost_resource = false;
  bool release_called = false;
  gpu::SyncToken sync_token;
  viz::ResourceId resource = CreateChildMailbox(
      &release_sync_token, &lost_resource, &release_called, &sync_token);

  // Transfer the resource, so we can't release it properly on shutdown.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  resource_ids_to_transfer.push_back(resource);
  std::vector<viz::TransferableResource> list;
  child_resource_provider_->PrepareSendToParent(resource_ids_to_transfer,
                                                &list);

  EXPECT_FALSE(release_sync_token.HasData());
  EXPECT_FALSE(lost_resource);

  child_resource_provider_ = nullptr;

  // Since the resource is in the parent, the child considers it lost.
  EXPECT_TRUE(lost_resource);
}

TEST_P(ResourceProviderTest, LostContext) {
  // viz::TextureMailbox callbacks only exist for GL textures for now.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  unsigned texture = context()->createTexture();
  context()->bindTexture(GL_TEXTURE_2D, texture);
  gpu::Mailbox mailbox;
  context()->genMailboxCHROMIUM(mailbox.name);
  context()->produceTextureDirectCHROMIUM(texture, GL_TEXTURE_2D, mailbox.name);
  gpu::SyncToken sync_token;
  context()->genSyncToken(context()->insertFenceSync(), sync_token.GetData());

  EXPECT_TRUE(sync_token.HasData());

  gpu::SyncToken release_sync_token;
  bool lost_resource = false;
  std::unique_ptr<viz::SingleReleaseCallback> callback =
      viz::SingleReleaseCallback::Create(
          base::Bind(&ReleaseCallback, &release_sync_token, &lost_resource));
  child_resource_provider_->CreateResourceFromTextureMailbox(
      viz::TextureMailbox(mailbox, sync_token, GL_TEXTURE_2D),
      std::move(callback));

  EXPECT_FALSE(release_sync_token.HasData());
  EXPECT_FALSE(lost_resource);

  child_resource_provider_->DidLoseVulkanContextProvider();
  child_resource_provider_ = nullptr;

  EXPECT_LE(sync_token.release_count(), release_sync_token.release_count());
  EXPECT_TRUE(lost_resource);
}

TEST_P(ResourceProviderTest, ScopedSampler) {
  // Sampling is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<DisplayResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  int texture_id = 1;

  viz::ResourceId id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());

  // Check that the texture gets created with the right sampler settings.
  EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id))
      .Times(2);  // Once to create and once to allocate.
  EXPECT_CALL(*context,
              texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  EXPECT_CALL(*context,
              texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  EXPECT_CALL(
      *context,
      texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  EXPECT_CALL(
      *context,
      texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  resource_provider->AllocateForTesting(id);
  Mock::VerifyAndClearExpectations(context);

  // Creating a sampler with the default filter should not change any texture
  // parameters.
  {
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id));
    DisplayResourceProvider::ScopedSamplerGL sampler(
        resource_provider.get(), id, GL_TEXTURE_2D, GL_LINEAR);
    Mock::VerifyAndClearExpectations(context);
  }

  // Using a different filter should be reflected in the texture parameters.
  {
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id));
    EXPECT_CALL(
        *context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    EXPECT_CALL(
        *context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    DisplayResourceProvider::ScopedSamplerGL sampler(
        resource_provider.get(), id, GL_TEXTURE_2D, GL_NEAREST);
    Mock::VerifyAndClearExpectations(context);
  }

  // Test resetting to the default filter.
  {
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id));
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    DisplayResourceProvider::ScopedSamplerGL sampler(
        resource_provider.get(), id, GL_TEXTURE_2D, GL_LINEAR);
    Mock::VerifyAndClearExpectations(context);
  }
}

TEST_P(ResourceProviderTest, ManagedResource) {
  // Sampling is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;
  int texture_id = 1;

  // Check that the texture gets created with the right sampler settings.
  viz::ResourceId id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());
  EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id));
  EXPECT_CALL(*context,
              texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  EXPECT_CALL(*context,
              texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  EXPECT_CALL(
      *context,
      texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  EXPECT_CALL(
      *context,
      texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  resource_provider->CreateForTesting(id);
  EXPECT_NE(0u, id);

  Mock::VerifyAndClearExpectations(context);
}

TEST_P(ResourceProviderTest, TextureWrapMode) {
  // Sampling is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  for (int texture_id = 1; texture_id <= 2; ++texture_id) {
    // Check that the texture gets created with the right sampler settings.
    viz::ResourceId id = resource_provider->CreateResource(
        size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
        gfx::ColorSpace());
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id));
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    EXPECT_CALL(*context, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                        GL_CLAMP_TO_EDGE));
    EXPECT_CALL(*context, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                        GL_CLAMP_TO_EDGE));
    resource_provider->CreateForTesting(id);
    EXPECT_NE(0u, id);

    Mock::VerifyAndClearExpectations(context);
  }
}

TEST_P(ResourceProviderTest, TextureHint) {
  // Sampling is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  context->set_support_texture_storage(true);
  context->set_support_texture_usage(true);
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  const ResourceProvider::TextureHint hints[] = {
      ResourceProvider::TEXTURE_HINT_DEFAULT,
      ResourceProvider::TEXTURE_HINT_FRAMEBUFFER,
  };
  for (GLuint texture_id = 1; texture_id <= arraysize(hints); ++texture_id) {
    // Check that the texture gets created with the right sampler settings.
    viz::ResourceId id = resource_provider->CreateResource(
        size, hints[texture_id - 1], format, gfx::ColorSpace());
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id));
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    EXPECT_CALL(
        *context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    EXPECT_CALL(
        *context,
        texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    // Check that GL_TEXTURE_USAGE_ANGLE is set iff the TEXTURE_HINT_FRAMEBUFFER
    // hint is used.
    bool is_framebuffer_hint =
        hints[texture_id - 1] & ResourceProvider::TEXTURE_HINT_FRAMEBUFFER;
    EXPECT_CALL(*context,
                texParameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_USAGE_ANGLE,
                              GL_FRAMEBUFFER_ATTACHMENT_ANGLE))
        .Times(is_framebuffer_hint ? 1 : 0);
    resource_provider->CreateForTesting(id);
    EXPECT_NE(0u, id);

    Mock::VerifyAndClearExpectations(context);
  }
}

TEST_P(ResourceProviderTest, TextureMailbox_SharedMemory) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_BITMAP)
    return;

  gfx::Size size(64, 64);
  const uint32_t kBadBeef = 0xbadbeef;
  std::unique_ptr<viz::SharedBitmap> shared_bitmap(
      CreateAndFillSharedBitmap(shared_bitmap_manager_.get(), size, kBadBeef));

  auto resource_provider(std::make_unique<DisplayResourceProvider>(
      nullptr, shared_bitmap_manager_.get(), gpu_memory_buffer_manager_.get(),
      kDelegatedSyncPointsRequired, CreateResourceSettings()));

  auto child_resource_provider(std::make_unique<LayerTreeResourceProvider>(
      nullptr, shared_bitmap_manager_.get(), gpu_memory_buffer_manager_.get(),
      kDelegatedSyncPointsRequired, CreateResourceSettings()));

  gpu::SyncToken release_sync_token;
  bool lost_resource = false;
  std::unique_ptr<viz::SingleReleaseCallback> callback =
      viz::SingleReleaseCallback::Create(
          base::Bind(&ReleaseCallback, &release_sync_token, &lost_resource));
  viz::TextureMailbox mailbox(shared_bitmap.get(), size);

  viz::ResourceId resource_id =
      child_resource_provider->CreateResourceFromTextureMailbox(
          mailbox, std::move(callback));
  EXPECT_NE(0u, resource_id);

  // Transfer resources to the parent.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  resource_ids_to_transfer.push_back(resource_id);

  std::vector<viz::TransferableResource> send_to_parent;
  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id = resource_provider->CreateChild(
      base::Bind(&CollectResources, &returned_to_child));
  child_resource_provider->PrepareSendToParent(resource_ids_to_transfer,
                                               &send_to_parent);
  resource_provider->ReceiveFromChild(child_id, send_to_parent);

  // In DisplayResourceProvider's namespace, use the mapped resource id.
  ResourceProvider::ResourceIdMap resource_map =
      resource_provider->GetChildToParentMap(child_id);
  viz::ResourceId mapped_resource_id = resource_map[resource_id];

  {
    DisplayResourceProvider::ScopedReadLockSoftware lock(
        resource_provider.get(), mapped_resource_id);
    const SkBitmap* sk_bitmap = lock.sk_bitmap();
    EXPECT_EQ(sk_bitmap->width(), size.width());
    EXPECT_EQ(sk_bitmap->height(), size.height());
    EXPECT_EQ(*sk_bitmap->getAddr32(16, 16), kBadBeef);
  }

  EXPECT_EQ(0u, returned_to_child.size());
  // Transfer resources back from the parent to the child. Set no resources as
  // being in use.
  resource_provider->DeclareUsedResourcesFromChild(child_id,
                                                   viz::ResourceIdSet());
  EXPECT_EQ(1u, returned_to_child.size());
  child_resource_provider->ReceiveReturnsFromParent(returned_to_child);

  child_resource_provider->DeleteResource(resource_id);
  EXPECT_FALSE(release_sync_token.HasData());
  EXPECT_FALSE(lost_resource);
}

class ResourceProviderTestTextureMailboxGLFilters
    : public ResourceProviderTest {
 public:
  static void RunTest(
      TestSharedBitmapManager* shared_bitmap_manager,
      viz::TestGpuMemoryBufferManager* gpu_memory_buffer_manager,
      bool mailbox_nearest_neighbor,
      GLenum sampler_filter) {
    auto context_owned(std::make_unique<TextureStateTrackingContext>());
    TextureStateTrackingContext* context = context_owned.get();
    auto context_provider =
        TestContextProvider::Create(std::move(context_owned));
    context_provider->BindToCurrentThread();

    auto resource_provider(std::make_unique<DisplayResourceProvider>(
        context_provider.get(), shared_bitmap_manager,
        gpu_memory_buffer_manager, kDelegatedSyncPointsRequired,
        CreateResourceSettings()));

    auto child_context_owned(std::make_unique<TextureStateTrackingContext>());
    TextureStateTrackingContext* child_context = child_context_owned.get();
    auto child_context_provider =
        TestContextProvider::Create(std::move(child_context_owned));
    child_context_provider->BindToCurrentThread();

    auto child_resource_provider(std::make_unique<LayerTreeResourceProvider>(
        child_context_provider.get(), shared_bitmap_manager,
        gpu_memory_buffer_manager, kDelegatedSyncPointsRequired,
        CreateResourceSettings()));

    unsigned texture_id = 1;
    gpu::SyncToken sync_token(gpu::CommandBufferNamespace::GPU_IO, 0,
                              gpu::CommandBufferId::FromUnsafeValue(0x12),
                              0x34);
    unsigned target = GL_TEXTURE_2D;
    const GLuint64 current_fence_sync = child_context->GetNextFenceSync();

    EXPECT_CALL(*child_context, bindTexture(_, _)).Times(0);
    EXPECT_CALL(*child_context, waitSyncToken(_)).Times(0);
    EXPECT_CALL(*child_context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);
    EXPECT_CALL(*child_context, createAndConsumeTextureCHROMIUM(_, _)).Times(0);

    gpu::Mailbox gpu_mailbox;
    memcpy(gpu_mailbox.name, "Hello world", strlen("Hello world") + 1);
    gpu::SyncToken release_sync_token;
    bool lost_resource = false;
    std::unique_ptr<viz::SingleReleaseCallback> callback =
        viz::SingleReleaseCallback::Create(
            base::Bind(&ReleaseCallback, &release_sync_token, &lost_resource));

    viz::TextureMailbox mailbox(gpu_mailbox, sync_token, target);
    mailbox.set_nearest_neighbor(mailbox_nearest_neighbor);

    viz::ResourceId resource_id =
        child_resource_provider->CreateResourceFromTextureMailbox(
            mailbox, std::move(callback));
    EXPECT_NE(0u, resource_id);
    EXPECT_EQ(current_fence_sync, child_context->GetNextFenceSync());

    Mock::VerifyAndClearExpectations(child_context);

    // Transfer resources to the parent.
    ResourceProvider::ResourceIdArray resource_ids_to_transfer;
    resource_ids_to_transfer.push_back(resource_id);

    std::vector<viz::TransferableResource> send_to_parent;
    std::vector<viz::ReturnedResource> returned_to_child;
    int child_id = resource_provider->CreateChild(
        base::Bind(&CollectResources, &returned_to_child));
    child_resource_provider->PrepareSendToParent(resource_ids_to_transfer,
                                                 &send_to_parent);
    resource_provider->ReceiveFromChild(child_id, send_to_parent);

    // In DisplayResourceProvider's namespace, use the mapped resource id.
    ResourceProvider::ResourceIdMap resource_map =
        resource_provider->GetChildToParentMap(child_id);
    viz::ResourceId mapped_resource_id = resource_map[resource_id];
    {
      // The verified flush flag will be set by
      // LayerTreeResourceProvider::PrepareSendToParent. Before checking if
      // the gpu::SyncToken matches, set this flag first.
      sync_token.SetVerifyFlush();

      // Mailbox sync point WaitSyncToken before using the texture.
      EXPECT_CALL(*context, waitSyncToken(MatchesSyncToken(sync_token)));
      resource_provider->WaitSyncToken(mapped_resource_id);
      Mock::VerifyAndClearExpectations(context);

      EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(target, _))
          .WillOnce(Return(texture_id));
      EXPECT_CALL(*context, bindTexture(target, texture_id));

      EXPECT_CALL(*context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);

      // The sampler will reset these if |mailbox_nearest_neighbor| does not
      // match |sampler_filter|.
      if (mailbox_nearest_neighbor != (sampler_filter == GL_NEAREST)) {
        EXPECT_CALL(*context, texParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler_filter));
        EXPECT_CALL(*context, texParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler_filter));
      }

      DisplayResourceProvider::ScopedSamplerGL lock(
          resource_provider.get(), mapped_resource_id, sampler_filter);
      Mock::VerifyAndClearExpectations(context);
      EXPECT_EQ(current_fence_sync, context->GetNextFenceSync());

      // When done with it, a sync point should be inserted, but no produce is
      // necessary.
      EXPECT_CALL(*child_context, bindTexture(_, _)).Times(0);
      EXPECT_CALL(*child_context, produceTextureDirectCHROMIUM(_, _, _))
          .Times(0);

      EXPECT_CALL(*child_context, waitSyncToken(_)).Times(0);
      EXPECT_CALL(*child_context, createAndConsumeTextureCHROMIUM(_, _))
          .Times(0);
    }

    EXPECT_EQ(0u, returned_to_child.size());
    // Transfer resources back from the parent to the child. Set no resources as
    // being in use.
    resource_provider->DeclareUsedResourcesFromChild(child_id,
                                                     viz::ResourceIdSet());
    EXPECT_EQ(1u, returned_to_child.size());
    child_resource_provider->ReceiveReturnsFromParent(returned_to_child);

    child_resource_provider->DeleteResource(resource_id);
    EXPECT_TRUE(release_sync_token.HasData());
    EXPECT_FALSE(lost_resource);
  }
};

TEST_P(ResourceProviderTest, TextureMailbox_GLTexture2D_LinearToLinear) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  ResourceProviderTestTextureMailboxGLFilters::RunTest(
      shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(),
      false,
      GL_LINEAR);
}

TEST_P(ResourceProviderTest, TextureMailbox_GLTexture2D_NearestToNearest) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  ResourceProviderTestTextureMailboxGLFilters::RunTest(
      shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(),
      true,
      GL_NEAREST);
}

TEST_P(ResourceProviderTest, TextureMailbox_GLTexture2D_NearestToLinear) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  ResourceProviderTestTextureMailboxGLFilters::RunTest(
      shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(),
      true,
      GL_LINEAR);
}

TEST_P(ResourceProviderTest, TextureMailbox_GLTexture2D_LinearToNearest) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  ResourceProviderTestTextureMailboxGLFilters::RunTest(
      shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(),
      false,
      GL_NEAREST);
}

TEST_P(ResourceProviderTest, TextureMailbox_GLTextureExternalOES) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<DisplayResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  auto child_context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* child_context = child_context_owned.get();
  auto child_context_provider =
      TestContextProvider::Create(std::move(child_context_owned));
  child_context_provider->BindToCurrentThread();

  auto child_resource_provider(std::make_unique<LayerTreeResourceProvider>(
      child_context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gpu::SyncToken sync_token(gpu::CommandBufferNamespace::GPU_IO, 0,
                            gpu::CommandBufferId::FromUnsafeValue(0x12), 0x34);
  const GLuint64 current_fence_sync = child_context->GetNextFenceSync();
  unsigned target = GL_TEXTURE_EXTERNAL_OES;

  EXPECT_CALL(*child_context, bindTexture(_, _)).Times(0);
  EXPECT_CALL(*child_context, waitSyncToken(_)).Times(0);
  EXPECT_CALL(*child_context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);
  EXPECT_CALL(*child_context, createAndConsumeTextureCHROMIUM(_, _)).Times(0);

  gpu::Mailbox gpu_mailbox;
  memcpy(gpu_mailbox.name, "Hello world", strlen("Hello world") + 1);
  std::unique_ptr<viz::SingleReleaseCallback> callback =
      viz::SingleReleaseCallback::Create(base::Bind(&EmptyReleaseCallback));

  viz::TextureMailbox mailbox(gpu_mailbox, sync_token, target);

  viz::ResourceId resource_id =
      child_resource_provider->CreateResourceFromTextureMailbox(
          mailbox, std::move(callback));
  EXPECT_NE(0u, resource_id);
  EXPECT_EQ(current_fence_sync, child_context->GetNextFenceSync());

  Mock::VerifyAndClearExpectations(child_context);

  // Transfer resources to the parent.
  ResourceProvider::ResourceIdArray resource_ids_to_transfer;
  resource_ids_to_transfer.push_back(resource_id);

  std::vector<viz::TransferableResource> send_to_parent;
  std::vector<viz::ReturnedResource> returned_to_child;
  int child_id = resource_provider->CreateChild(
      base::Bind(&CollectResources, &returned_to_child));
  child_resource_provider->PrepareSendToParent(resource_ids_to_transfer,
                                               &send_to_parent);
  resource_provider->ReceiveFromChild(child_id, send_to_parent);

  // Before create DrawQuad in DisplayResourceProvider's namespace, get the
  // mapped resource id first.
  ResourceProvider::ResourceIdMap resource_map =
      resource_provider->GetChildToParentMap(child_id);
  viz::ResourceId mapped_resource_id = resource_map[resource_id];
  {
    // The verified flush flag will be set by
    // LayerTreeResourceProvider::PrepareSendToParent. Before checking if
    // the gpu::SyncToken matches, set this flag first.
    sync_token.SetVerifyFlush();

    // Mailbox sync point WaitSyncToken before using the texture.
    EXPECT_CALL(*context, waitSyncToken(MatchesSyncToken(sync_token)));
    resource_provider->WaitSyncToken(mapped_resource_id);
    Mock::VerifyAndClearExpectations(context);

    unsigned texture_id = 1;

    EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(target, _))
        .WillOnce(Return(texture_id));

    EXPECT_CALL(*context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);

    DisplayResourceProvider::ScopedReadLockGL lock(resource_provider.get(),
                                                   mapped_resource_id);
    Mock::VerifyAndClearExpectations(context);

    // When done with it, a sync point should be inserted, but no produce is
    // necessary.
    EXPECT_CALL(*context, bindTexture(_, _)).Times(0);
    EXPECT_CALL(*context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);

    EXPECT_CALL(*context, waitSyncToken(_)).Times(0);
    EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(_, _)).Times(0);
    Mock::VerifyAndClearExpectations(context);
  }
  EXPECT_EQ(0u, returned_to_child.size());
  // Transfer resources back from the parent to the child. Set no resources as
  // being in use.
  resource_provider->DeclareUsedResourcesFromChild(child_id,
                                                   viz::ResourceIdSet());
  EXPECT_EQ(1u, returned_to_child.size());
  child_resource_provider->ReceiveReturnsFromParent(returned_to_child);

  child_resource_provider->DeleteResource(resource_id);
}

TEST_P(ResourceProviderTest,
       TextureMailbox_WaitSyncTokenIfNeeded_WithSyncToken) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gpu::SyncToken sync_token(gpu::CommandBufferNamespace::GPU_IO, 0,
                            gpu::CommandBufferId::FromUnsafeValue(0x12), 0x34);
  const GLuint64 current_fence_sync = context->GetNextFenceSync();
  unsigned target = GL_TEXTURE_2D;

  EXPECT_CALL(*context, bindTexture(_, _)).Times(0);
  EXPECT_CALL(*context, waitSyncToken(_)).Times(0);
  EXPECT_CALL(*context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);
  EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(_, _)).Times(0);

  gpu::Mailbox gpu_mailbox;
  memcpy(gpu_mailbox.name, "Hello world", strlen("Hello world") + 1);
  std::unique_ptr<viz::SingleReleaseCallback> callback =
      viz::SingleReleaseCallback::Create(base::Bind(&EmptyReleaseCallback));

  viz::TextureMailbox mailbox(gpu_mailbox, sync_token, target);

  viz::ResourceId id = resource_provider->CreateResourceFromTextureMailbox(
      mailbox, std::move(callback));
  EXPECT_NE(0u, id);
  EXPECT_EQ(current_fence_sync, context->GetNextFenceSync());

  Mock::VerifyAndClearExpectations(context);

  {
    // First call to WaitSyncToken should call waitSyncToken.
    EXPECT_CALL(*context, waitSyncToken(MatchesSyncToken(sync_token)));
    resource_provider->WaitSyncToken(id);
    Mock::VerifyAndClearExpectations(context);

    // Subsequent calls to WaitSyncToken shouldn't call waitSyncToken.
    EXPECT_CALL(*context, waitSyncToken(_)).Times(0);
    resource_provider->WaitSyncToken(id);
    Mock::VerifyAndClearExpectations(context);
  }
}

TEST_P(ResourceProviderTest, TextureMailbox_WaitSyncTokenIfNeeded_NoSyncToken) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gpu::SyncToken sync_token;
  const GLuint64 current_fence_sync = context->GetNextFenceSync();
  unsigned target = GL_TEXTURE_2D;

  EXPECT_CALL(*context, bindTexture(_, _)).Times(0);
  EXPECT_CALL(*context, waitSyncToken(_)).Times(0);
  EXPECT_CALL(*context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);
  EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(_, _)).Times(0);

  gpu::Mailbox gpu_mailbox;
  memcpy(gpu_mailbox.name, "Hello world", strlen("Hello world") + 1);
  std::unique_ptr<viz::SingleReleaseCallback> callback =
      viz::SingleReleaseCallback::Create(base::Bind(&EmptyReleaseCallback));

  viz::TextureMailbox mailbox(gpu_mailbox, sync_token, target);

  viz::ResourceId id = resource_provider->CreateResourceFromTextureMailbox(
      mailbox, std::move(callback));
  EXPECT_NE(0u, id);
  EXPECT_EQ(current_fence_sync, context->GetNextFenceSync());

  Mock::VerifyAndClearExpectations(context);

  {
    // WaitSyncToken with empty sync_token shouldn't call waitSyncToken.
    EXPECT_CALL(*context, waitSyncToken(_)).Times(0);
    resource_provider->WaitSyncToken(id);
    Mock::VerifyAndClearExpectations(context);
  }
}

TEST_P(ResourceProviderTest, TextureMailbox_PrepareSendToParent_NoSyncToken) {
  // Mailboxing is only supported for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context_owned(std::make_unique<TextureStateTrackingContext>());
  TextureStateTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  EXPECT_CALL(*context, bindTexture(_, _)).Times(0);
  EXPECT_CALL(*context, waitSyncToken(_)).Times(0);
  EXPECT_CALL(*context, produceTextureDirectCHROMIUM(_, _, _)).Times(0);
  EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(_, _)).Times(0);

  viz::TextureMailbox mailbox(gpu::Mailbox::Generate(), gpu::SyncToken(),
                              GL_TEXTURE_2D);

  std::unique_ptr<viz::SingleReleaseCallback> callback =
      viz::SingleReleaseCallback::Create(base::Bind(&EmptyReleaseCallback));

  viz::ResourceId id = resource_provider->CreateResourceFromTextureMailbox(
      mailbox, std::move(callback));
  EXPECT_NE(0u, id);
  Mock::VerifyAndClearExpectations(context);

  ResourceProvider::ResourceIdArray resource_ids_to_transfer{id};
  std::vector<viz::TransferableResource> list;
  resource_provider->PrepareSendToParent(resource_ids_to_transfer, &list);
  ASSERT_EQ(1u, list.size());
  EXPECT_FALSE(list[0].mailbox_holder.sync_token.HasData());
  EXPECT_TRUE(list[0].mailbox_holder.sync_token.verified_flush());
  Mock::VerifyAndClearExpectations(context);
}

class AllocationTrackingContext3D : public TextureStateTrackingContext {
 public:
  MOCK_METHOD0(NextTextureId, GLuint());
  MOCK_METHOD1(RetireTextureId, void(GLuint id));
  MOCK_METHOD5(texStorage2DEXT,
               void(GLenum target,
                    GLint levels,
                    GLuint internalformat,
                    GLint width,
                    GLint height));
  MOCK_METHOD9(texImage2D,
               void(GLenum target,
                    GLint level,
                    GLenum internalformat,
                    GLsizei width,
                    GLsizei height,
                    GLint border,
                    GLenum format,
                    GLenum type,
                    const void* pixels));
  MOCK_METHOD9(texSubImage2D,
               void(GLenum target,
                    GLint level,
                    GLint xoffset,
                    GLint yoffset,
                    GLsizei width,
                    GLsizei height,
                    GLenum format,
                    GLenum type,
                    const void* pixels));
  MOCK_METHOD9(asyncTexImage2DCHROMIUM,
               void(GLenum target,
                    GLint level,
                    GLenum internalformat,
                    GLsizei width,
                    GLsizei height,
                    GLint border,
                    GLenum format,
                    GLenum type,
                    const void* pixels));
  MOCK_METHOD9(asyncTexSubImage2DCHROMIUM,
               void(GLenum target,
                    GLint level,
                    GLint xoffset,
                    GLint yoffset,
                    GLsizei width,
                    GLsizei height,
                    GLenum format,
                    GLenum type,
                    const void* pixels));
  MOCK_METHOD8(compressedTexImage2D,
               void(GLenum target,
                    GLint level,
                    GLenum internalformat,
                    GLsizei width,
                    GLsizei height,
                    GLint border,
                    GLsizei image_size,
                    const void* data));
  MOCK_METHOD1(waitAsyncTexImage2DCHROMIUM, void(GLenum));
  MOCK_METHOD4(createImageCHROMIUM,
               GLuint(ClientBuffer, GLsizei, GLsizei, GLenum));
  MOCK_METHOD1(destroyImageCHROMIUM, void(GLuint));
  MOCK_METHOD2(bindTexImage2DCHROMIUM, void(GLenum, GLint));
  MOCK_METHOD2(releaseTexImage2DCHROMIUM, void(GLenum, GLint));

  // We're mocking bindTexture, so we override
  // TestWebGraphicsContext3D::texParameteri to avoid assertions related to the
  // currently bound texture.
  void texParameteri(GLenum target, GLenum pname, GLint param) override {}
};

TEST_P(ResourceProviderTest, TextureAllocation) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new StrictMock<AllocationTrackingContext3D>);
  AllocationTrackingContext3D* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  gfx::Size size(2, 2);
  gfx::Vector2d offset(0, 0);
  viz::ResourceFormat format = viz::RGBA_8888;
  viz::ResourceId id = 0;
  uint8_t pixels[16] = { 0 };
  int texture_id = 123;

  // Lazy allocation. Don't allocate when creating the resource.
  id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());

  EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(texture_id));
  EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id)).Times(1);
  resource_provider->CreateForTesting(id);

  EXPECT_CALL(*context, RetireTextureId(texture_id)).Times(1);
  resource_provider->DeleteResource(id);

  Mock::VerifyAndClearExpectations(context);

  // Do allocate when we set the pixels.
  id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, gfx::ColorSpace());

  EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(texture_id));
  EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id)).Times(3);
  EXPECT_CALL(*context, texImage2D(_, _, _, 2, 2, _, _, _, _)).Times(1);
  EXPECT_CALL(*context, texSubImage2D(_, _, _, _, 2, 2, _, _, _)).Times(1);
  resource_provider->CopyToResource(id, pixels, size);

  EXPECT_CALL(*context, RetireTextureId(texture_id)).Times(1);
  resource_provider->DeleteResource(id);

  Mock::VerifyAndClearExpectations(context);
}

static void TestTextureAllocationHint(
    AllocationTrackingContext3D* context,
    LayerTreeResourceProvider* resource_provider) {
  gfx::Size size(2, 2);

  const viz::ResourceFormat formats[3] = {viz::RGBA_8888, viz::BGRA_8888,
                                          viz::RGBA_F16};
  const ResourceProvider::TextureHint hints[2] = {
      ResourceProvider::TEXTURE_HINT_DEFAULT,
      ResourceProvider::TEXTURE_HINT_FRAMEBUFFER,
  };
  for (size_t i = 0; i < arraysize(formats); ++i) {
    for (GLuint texture_id = 1; texture_id <= arraysize(hints); ++texture_id) {
      // Lazy allocation. Don't allocate when creating the resource.
      viz::ResourceId id = resource_provider->CreateResource(
          size, hints[texture_id - 1], formats[i], gfx::ColorSpace());

      EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(texture_id));
      EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id)).Times(2);
      // Resources are immutable when possible, so texStorage2DEXT should happen
      // instead of texImage2D when the context support or format allows.
      bool support_immutable_texture =
          context->test_capabilities().texture_format_bgra8888 ||
          formats[i] != viz::BGRA_8888;
      EXPECT_CALL(*context, texStorage2DEXT(_, _, _, 2, 2))
          .Times(support_immutable_texture ? 1 : 0);
      EXPECT_CALL(*context, texImage2D(_, _, _, 2, 2, _, _, _, _))
          .Times(support_immutable_texture ? 0 : 1);
      resource_provider->AllocateForTesting(id);

      EXPECT_CALL(*context, RetireTextureId(texture_id)).Times(1);
      resource_provider->DeleteResource(id);

      Mock::VerifyAndClearExpectations(context);
    }
  }
}

TEST_P(ResourceProviderTest, TextureAllocationHint) {
  // Only for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context = base::MakeUnique<StrictMock<AllocationTrackingContext3D>>();
  auto* context_ptr = context.get();

  // BGRA storage is not supported.
  context->set_support_texture_format_bgra8888(false);

  context->set_support_texture_storage(true);
  context->set_support_texture_usage(true);
  auto context_provider = TestContextProvider::Create(std::move(context));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  TestTextureAllocationHint(context_ptr, resource_provider.get());
}

TEST_P(ResourceProviderTest, TextureAllocationHint_BGRA) {
  // Only for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  auto context = base::MakeUnique<StrictMock<AllocationTrackingContext3D>>();
  auto* context_ptr = context.get();

  // BGRA storage is supported.
  context->set_support_texture_format_bgra8888(true);

  context->set_support_texture_storage(true);
  context->set_support_texture_usage(true);
  auto context_provider = TestContextProvider::Create(std::move(context));
  context_provider->BindToCurrentThread();

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  TestTextureAllocationHint(context_ptr, resource_provider.get());
}

TEST_P(ResourceProviderTest, Image_GLTexture) {
  // Only for GL textures.
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new StrictMock<AllocationTrackingContext3D>);
  AllocationTrackingContext3D* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  const int kWidth = 2;
  const int kHeight = 2;
  viz::ResourceFormat format = viz::RGBA_8888;
  const unsigned kTextureId = 123u;
  const unsigned kImageId = 234u;

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  viz::ResourceId id = resource_provider->CreateGpuMemoryBufferResource(
      gfx::Size(kWidth, kHeight), ResourceProvider::TEXTURE_HINT_DEFAULT,
      format, gfx::BufferUsage::GPU_READ_CPU_READ_WRITE, gfx::ColorSpace());

  InSequence sequence;

  {
    LayerTreeResourceProvider::ScopedWriteLockGpuMemoryBuffer lock(
        resource_provider.get(), id);
    EXPECT_TRUE(lock.GetGpuMemoryBuffer());
    // Create texture and image upon releasing the lock.
    EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(kTextureId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    EXPECT_CALL(*context, createImageCHROMIUM(_, kWidth, kHeight, GL_RGBA))
        .WillOnce(Return(kImageId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    EXPECT_CALL(*context, bindTexImage2DCHROMIUM(GL_TEXTURE_2D, kImageId));
  }
  // The image is created in the lock's destructor.
  Mock::VerifyAndClearExpectations(context);

  {
    LayerTreeResourceProvider::ScopedWriteLockGpuMemoryBuffer lock(
        resource_provider.get(), id);
    EXPECT_TRUE(lock.GetGpuMemoryBuffer());
    // Upload to GPU again since image is dirty after the write lock.
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    EXPECT_CALL(*context, releaseTexImage2DCHROMIUM(GL_TEXTURE_2D, kImageId));
    EXPECT_CALL(*context, bindTexImage2DCHROMIUM(GL_TEXTURE_2D, kImageId));
  }
  // The image is updated in the lock's destructor.
  Mock::VerifyAndClearExpectations(context);

  EXPECT_CALL(*context, destroyImageCHROMIUM(kImageId));
  EXPECT_CALL(*context, RetireTextureId(kTextureId));
}

TEST_P(ResourceProviderTest, CompressedTextureETC1Allocate) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new AllocationTrackingContext3D);
  AllocationTrackingContext3D* context = context_owned.get();
  context_owned->set_support_compressed_texture_etc1(true);
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  gfx::Size size(4, 4);
  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));
  int texture_id = 123;

  viz::ResourceId id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, viz::ETC1,
      gfx::ColorSpace());
  EXPECT_NE(0u, id);
  EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(texture_id));
  EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id)).Times(1);
  resource_provider->AllocateForTesting(id);

  EXPECT_CALL(*context, RetireTextureId(texture_id)).Times(1);
  resource_provider->DeleteResource(id);
}

TEST_P(ResourceProviderTest, CompressedTextureETC1Upload) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new AllocationTrackingContext3D);
  AllocationTrackingContext3D* context = context_owned.get();
  context_owned->set_support_compressed_texture_etc1(true);
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  gfx::Size size(4, 4);
  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));
  int texture_id = 123;
  uint8_t pixels[8];

  viz::ResourceId id = resource_provider->CreateResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, viz::ETC1,
      gfx::ColorSpace());
  EXPECT_NE(0u, id);
  EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(texture_id));
  EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, texture_id)).Times(2);
  EXPECT_CALL(*context,
              compressedTexImage2D(
                  _, 0, _, size.width(), size.height(), _, _, _)).Times(1);
  resource_provider->CopyToResource(id, pixels, size);

  EXPECT_CALL(*context, RetireTextureId(texture_id)).Times(1);
  resource_provider->DeleteResource(id);
}

INSTANTIATE_TEST_CASE_P(
    ResourceProviderTests,
    ResourceProviderTest,
    ::testing::Values(ResourceProvider::RESOURCE_TYPE_GL_TEXTURE,
                      ResourceProvider::RESOURCE_TYPE_BITMAP));

class TextureIdAllocationTrackingContext : public TestWebGraphicsContext3D {
 public:
  GLuint NextTextureId() override {
    base::AutoLock lock(namespace_->lock);
    return namespace_->next_texture_id++;
  }
  void RetireTextureId(GLuint) override {}
  GLuint PeekTextureId() {
    base::AutoLock lock(namespace_->lock);
    return namespace_->next_texture_id;
  }
};

TEST(ResourceProviderTest, TextureAllocationChunkSize) {
  auto context_owned(std::make_unique<TextureIdAllocationTrackingContext>());
  TextureIdAllocationTrackingContext* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();
  auto shared_bitmap_manager = std::make_unique<TestSharedBitmapManager>();

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  {
    size_t kTextureAllocationChunkSize = 1;
    auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
        context_provider.get(), shared_bitmap_manager.get(), nullptr,
        kDelegatedSyncPointsRequired,
        CreateResourceSettings(kTextureAllocationChunkSize)));

    viz::ResourceId id = resource_provider->CreateResource(
        size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
        gfx::ColorSpace());
    resource_provider->AllocateForTesting(id);
    Mock::VerifyAndClearExpectations(context);

    DCHECK_EQ(2u, context->PeekTextureId());
    resource_provider->DeleteResource(id);
  }

  {
    size_t kTextureAllocationChunkSize = 8;
    auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
        context_provider.get(), shared_bitmap_manager.get(), nullptr,
        kDelegatedSyncPointsRequired,
        CreateResourceSettings(kTextureAllocationChunkSize)));

    viz::ResourceId id = resource_provider->CreateResource(
        size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
        gfx::ColorSpace());
    resource_provider->AllocateForTesting(id);
    Mock::VerifyAndClearExpectations(context);

    DCHECK_EQ(10u, context->PeekTextureId());
    resource_provider->DeleteResource(id);
  }
}

TEST_P(ResourceProviderTest, GetSyncTokenForResources) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;

  gfx::Size size(1, 1);
  viz::ResourceFormat format = viz::RGBA_8888;

  // ~Random set of |release_count|s to set on sync tokens.
  uint64_t release_counts[5] = {7, 3, 10, 2, 5};

  ResourceProvider::ResourceIdArray array;
  for (uint32_t i = 0; i < arraysize(release_counts); ++i) {
    viz::ResourceId id = child_resource_provider_->CreateResource(
        size, ResourceProvider::TEXTURE_HINT_DEFAULT, format,
        gfx::ColorSpace());
    array.push_back(id);

    ResourceProvider::ScopedWriteLockGL lock(child_resource_provider_.get(),
                                             id);
    gpu::SyncToken token;
    token.Set(gpu::CommandBufferNamespace::INVALID, 0, gpu::CommandBufferId(),
              release_counts[i]);
    lock.set_sync_token(token);
  }

  gpu::SyncToken last_token =
      child_resource_provider_->GetSyncTokenForResources(array);
  EXPECT_EQ(last_token.release_count(), 10u);
}

TEST_P(ResourceProviderTest, ScopedWriteLockGL) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new StrictMock<AllocationTrackingContext3D>);
  AllocationTrackingContext3D* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  const int kWidth = 2;
  const int kHeight = 2;
  const viz::ResourceFormat format = viz::RGBA_8888;
  const unsigned kTextureId = 123u;

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  viz::ResourceId id = resource_provider->CreateResource(
      gfx::Size(kWidth, kHeight), ResourceProvider::TEXTURE_HINT_DEFAULT,
      format, gfx::ColorSpace());

  InSequence sequence;

  // First use will allocate lazily when accessing the texture.
  {
    EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(kTextureId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId)).Times(2);
    EXPECT_CALL(*context, texImage2D(GL_TEXTURE_2D, 0, GLInternalFormat(format),
                                     kWidth, kHeight, 0, GLDataFormat(format),
                                     GLDataType(format), nullptr));
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    EXPECT_EQ(lock.GetTexture(), kTextureId);
    Mock::VerifyAndClearExpectations(context);
  }

  // Subsequent uses will not allocate.
  {
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    EXPECT_EQ(lock.GetTexture(), kTextureId);
  }

  EXPECT_CALL(*context, RetireTextureId(kTextureId));
  resource_provider->DeleteResource(id);
}

TEST_P(ResourceProviderTest, ScopedWriteLockGL_GpuMemoryBuffer) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new StrictMock<AllocationTrackingContext3D>);
  AllocationTrackingContext3D* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  const int kWidth = 2;
  const int kHeight = 2;
  const viz::ResourceFormat format = viz::RGBA_8888;
  const unsigned kTextureId = 123u;
  const unsigned kImageId = 234u;

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  viz::ResourceId id = resource_provider->CreateGpuMemoryBufferResource(
      gfx::Size(kWidth, kHeight), ResourceProvider::TEXTURE_HINT_DEFAULT,
      format, gfx::BufferUsage::GPU_READ_CPU_READ_WRITE, gfx::ColorSpace());

  InSequence sequence;

  // First use will allocate lazily on accessing the texture.
  {
    EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(kTextureId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    EXPECT_CALL(*context, createImageCHROMIUM(_, kWidth, kHeight,
                                              GLInternalFormat(format)))
        .WillOnce(Return(kImageId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    EXPECT_CALL(*context, bindTexImage2DCHROMIUM(GL_TEXTURE_2D, kImageId));
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    EXPECT_EQ(lock.GetTexture(), kTextureId);
    Mock::VerifyAndClearExpectations(context);
  }

  // Subsequent uses will not allocate.
  {
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    EXPECT_EQ(lock.GetTexture(), kTextureId);
  }

  EXPECT_CALL(*context, destroyImageCHROMIUM(kImageId));
  EXPECT_CALL(*context, RetireTextureId(kTextureId));
  resource_provider->DeleteResource(id);
}

TEST_P(ResourceProviderTest, ScopedWriteLockGL_Mailbox) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new StrictMock<AllocationTrackingContext3D>);
  AllocationTrackingContext3D* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  const int kWidth = 2;
  const int kHeight = 2;
  const viz::ResourceFormat format = viz::RGBA_8888;
  const unsigned kTextureId = 123u;
  const unsigned kWorkerTextureId = 234u;

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  viz::ResourceId id = resource_provider->CreateResource(
      gfx::Size(kWidth, kHeight), ResourceProvider::TEXTURE_HINT_DEFAULT,
      format, gfx::ColorSpace());

  InSequence sequence;
  gpu::SyncToken sync_token;

  // First use will create mailbox when lock is created and allocate lazily in
  // ConsumeTexture.
  {
    EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(kTextureId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context,
                produceTextureDirectCHROMIUM(kTextureId, GL_TEXTURE_2D, _));
    lock.CreateMailbox();
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, _))
        .WillOnce(Return(kWorkerTextureId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kWorkerTextureId));
    EXPECT_CALL(*context, texImage2D(GL_TEXTURE_2D, 0, GLInternalFormat(format),
                                     kWidth, kHeight, 0, GLDataFormat(format),
                                     GLDataType(format), nullptr));
    EXPECT_EQ(kWorkerTextureId,
              lock.ConsumeTexture(context_provider->ContextGL()));
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, RetireTextureId(kWorkerTextureId));
    context_provider->ContextGL()->DeleteTextures(1, &kWorkerTextureId);
    Mock::VerifyAndClearExpectations(context);
  }

  // Subsequent uses will not create mailbox or allocate.
  {
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    lock.CreateMailbox();
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, _))
        .WillOnce(Return(kWorkerTextureId));
    EXPECT_EQ(kWorkerTextureId,
              lock.ConsumeTexture(context_provider->ContextGL()));
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, RetireTextureId(kWorkerTextureId));
    context_provider->ContextGL()->DeleteTextures(1, &kWorkerTextureId);

    sync_token = ResourceProvider::GenerateSyncTokenHelper(
        context_provider->ContextGL());
    lock.set_sync_token(sync_token);
    Mock::VerifyAndClearExpectations(context);
  }

  // Wait for worker context sync token before deleting texture.
  EXPECT_CALL(*context, waitSyncToken(MatchesSyncToken(sync_token)));
  EXPECT_CALL(*context, RetireTextureId(kTextureId));
  resource_provider->DeleteResource(id);
}

TEST_P(ResourceProviderTest, ScopedWriteLockGL_GpuMemoryBuffer_Mailbox) {
  if (GetParam() != ResourceProvider::RESOURCE_TYPE_GL_TEXTURE)
    return;
  std::unique_ptr<AllocationTrackingContext3D> context_owned(
      new StrictMock<AllocationTrackingContext3D>);
  AllocationTrackingContext3D* context = context_owned.get();
  auto context_provider = TestContextProvider::Create(std::move(context_owned));
  context_provider->BindToCurrentThread();

  const int kWidth = 2;
  const int kHeight = 2;
  const viz::ResourceFormat format = viz::RGBA_8888;
  const unsigned kTextureId = 123u;
  const unsigned kWorkerTextureId = 234u;
  const unsigned kImageId = 345u;

  auto resource_provider(std::make_unique<LayerTreeResourceProvider>(
      context_provider.get(), shared_bitmap_manager_.get(),
      gpu_memory_buffer_manager_.get(), kDelegatedSyncPointsRequired,
      CreateResourceSettings()));

  viz::ResourceId id = resource_provider->CreateGpuMemoryBufferResource(
      gfx::Size(kWidth, kHeight), ResourceProvider::TEXTURE_HINT_DEFAULT,
      format, gfx::BufferUsage::GPU_READ_CPU_READ_WRITE, gfx::ColorSpace());

  InSequence sequence;
  gpu::SyncToken sync_token;

  // First use will create mailbox when lock is created and allocate lazily in
  // ConsumeTexture.
  {
    EXPECT_CALL(*context, NextTextureId()).WillOnce(Return(kTextureId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kTextureId));
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context,
                produceTextureDirectCHROMIUM(kTextureId, GL_TEXTURE_2D, _));
    lock.CreateMailbox();
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, _))
        .WillOnce(Return(kWorkerTextureId));
    EXPECT_CALL(*context, createImageCHROMIUM(_, kWidth, kHeight,
                                              GLInternalFormat(format)))
        .WillOnce(Return(kImageId));
    EXPECT_CALL(*context, bindTexture(GL_TEXTURE_2D, kWorkerTextureId));
    EXPECT_CALL(*context, bindTexImage2DCHROMIUM(GL_TEXTURE_2D, kImageId));
    EXPECT_EQ(kWorkerTextureId,
              lock.ConsumeTexture(context_provider->ContextGL()));
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, RetireTextureId(kWorkerTextureId));
    context_provider->ContextGL()->DeleteTextures(1, &kWorkerTextureId);
    Mock::VerifyAndClearExpectations(context);
  }

  // Subsequent uses will not create mailbox or allocate.
  {
    ResourceProvider::ScopedWriteLockGL lock(resource_provider.get(), id);
    lock.CreateMailbox();
    Mock::VerifyAndClearExpectations(context);

    EXPECT_CALL(*context, createAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, _))
        .WillOnce(Return(kWorkerTextureId));
    EXPECT_EQ(kWorkerTextureId,
              lock.ConsumeTexture(context_provider->ContextGL()));
    Mock::VerifyAndClearExpectations(context);

    sync_token = ResourceProvider::GenerateSyncTokenHelper(
        context_provider->ContextGL());
    lock.set_sync_token(sync_token);

    EXPECT_CALL(*context, RetireTextureId(kWorkerTextureId));
    context_provider->ContextGL()->DeleteTextures(1, &kWorkerTextureId);
    Mock::VerifyAndClearExpectations(context);
  }

  // Wait for worker context sync token before deleting texture.
  EXPECT_CALL(*context, waitSyncToken(MatchesSyncToken(sync_token)));
  EXPECT_CALL(*context, destroyImageCHROMIUM(kImageId));
  EXPECT_CALL(*context, RetireTextureId(kTextureId));
  resource_provider->DeleteResource(id);
}

}  // namespace
}  // namespace cc
