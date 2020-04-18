// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/gl_renderer_copier.h"

#include <stdint.h>

#include <iterator>
#include <memory>

#include "base/bind.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/test/test_context_provider.h"
#include "components/viz/test/test_gles2_interface.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/vector2d.h"

namespace viz {

namespace {

class CopierTestGLES2Interface : public TestGLES2Interface {
 public:
  // Sets how GL will respond to queries regarding the implementation's internal
  // read-back format.
  void SetOptimalReadbackFormat(GLenum format, GLenum type) {
    format_ = format;
    type_ = type;
  }

  // GLES2Interface override.
  void GetIntegerv(GLenum pname, GLint* params) override {
    switch (pname) {
      case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
        ASSERT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
                  CheckFramebufferStatus(GL_FRAMEBUFFER));
        params[0] = format_;
        break;
      case GL_IMPLEMENTATION_COLOR_READ_TYPE:
        ASSERT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
                  CheckFramebufferStatus(GL_FRAMEBUFFER));
        params[0] = type_;
        break;
      default:
        TestGLES2Interface::GetIntegerv(pname, params);
        break;
    }
  }

 private:
  GLenum format_ = 0;
  GLenum type_ = 0;
};

}  // namespace

class GLRendererCopierTest : public testing::Test {
 public:
  void SetUp() override {
    auto context_provider = TestContextProvider::Create(
        std::make_unique<CopierTestGLES2Interface>());
    context_provider->BindToCurrentThread();
    copier_ = std::make_unique<GLRendererCopier>(
        std::move(context_provider), nullptr,
        base::BindRepeating([](const gfx::Rect& rect) { return rect; }));
  }

  void TearDown() override { copier_.reset(); }

  GLRendererCopier* copier() const { return copier_.get(); }

  CopierTestGLES2Interface* test_gl() const {
    return static_cast<CopierTestGLES2Interface*>(
        copier_->context_provider_->ContextGL());
  }

  // These simply forward method calls to GLRendererCopier.
  GLuint TakeCachedObjectOrCreate(const base::UnguessableToken& source,
                                  int which) {
    GLuint result = 0;
    copier_->TakeCachedObjectsOrCreate(source, which, 1, &result);
    return result;
  }
  void CacheObjectOrDelete(const base::UnguessableToken& source,
                           int which,
                           GLuint name) {
    copier_->CacheObjectsOrDelete(source, which, 1, &name);
  }
  std::unique_ptr<GLHelper::ScalerInterface> TakeCachedScalerOrCreate(
      const CopyOutputRequest& request) {
    return copier_->TakeCachedScalerOrCreate(request, true);
  }
  void CacheScalerOrDelete(const base::UnguessableToken& source,
                           std::unique_ptr<GLHelper::ScalerInterface> scaler) {
    copier_->CacheScalerOrDelete(source, std::move(scaler));
  }
  void FreeUnusedCachedResources() { copier_->FreeUnusedCachedResources(); }
  GLenum GetOptimalReadbackFormat() const {
    return copier_->GetOptimalReadbackFormat();
  }

  // These inspect the internal state of the GLRendererCopier's cache.
  size_t GetCopierCacheSize() { return copier_->cache_.size(); }
  bool CacheContainsObject(const base::UnguessableToken& source,
                           int which,
                           GLuint name) {
    return !copier_->cache_.empty() && copier_->cache_.count(source) != 0 &&
           copier_->cache_[source].object_names[which] == name;
  }
  bool CacheContainsScaler(const base::UnguessableToken& source,
                           int scale_from,
                           int scale_to) {
    return !copier_->cache_.empty() && copier_->cache_.count(source) != 0 &&
           copier_->cache_[source].scaler &&
           copier_->cache_[source].scaler->IsSameScaleRatio(
               gfx::Vector2d(scale_from, scale_from),
               gfx::Vector2d(scale_to, scale_to));
  }

  static constexpr int kKeepalivePeriod = GLRendererCopier::kKeepalivePeriod;

 private:
  std::unique_ptr<GLRendererCopier> copier_;
};

// Tests that named objects, such as textures or framebuffers, are only cached
// when the CopyOutputRequest has specified a "source" of requests.
TEST_F(GLRendererCopierTest, ReusesNamedObjects) {
  // With no source set in a copy request, expect to never re-use any textures
  // or framebuffers.
  base::UnguessableToken source;
  for (int which = 0; which < 3; ++which) {
    const GLuint a = TakeCachedObjectOrCreate(source, which);
    EXPECT_NE(0u, a);
    CacheObjectOrDelete(base::UnguessableToken(), which, a);
    const GLuint b = TakeCachedObjectOrCreate(source, which);
    EXPECT_NE(0u, b);
    CacheObjectOrDelete(base::UnguessableToken(), which, b);
    EXPECT_EQ(0u, GetCopierCacheSize());
  }

  // With a source set in the request, objects should now be cached and re-used.
  source = base::UnguessableToken::Create();
  for (int which = 0; which < 3; ++which) {
    const GLuint a = TakeCachedObjectOrCreate(source, which);
    EXPECT_NE(0u, a);
    CacheObjectOrDelete(source, which, a);
    const GLuint b = TakeCachedObjectOrCreate(source, which);
    EXPECT_NE(0u, b);
    EXPECT_EQ(a, b);
    CacheObjectOrDelete(source, which, b);
    EXPECT_EQ(1u, GetCopierCacheSize());
    EXPECT_TRUE(CacheContainsObject(source, which, a));
  }
}

// Tests that scalers are only cached when the CopyOutputRequest has specified a
// "source" of requests, and that different scalers are created if the scale
// ratio changes.
TEST_F(GLRendererCopierTest, ReusesScalers) {
  // With no source set in the request, expect to not cache a scaler.
  const auto request = CopyOutputRequest::CreateStubForTesting();
  ASSERT_FALSE(request->has_source());
  request->SetUniformScaleRatio(2, 1);
  std::unique_ptr<GLHelper::ScalerInterface> scaler =
      TakeCachedScalerOrCreate(*request);
  EXPECT_TRUE(scaler.get());
  CacheScalerOrDelete(base::UnguessableToken(), std::move(scaler));
  EXPECT_FALSE(CacheContainsScaler(base::UnguessableToken(), 2, 1));

  // With a source set in the request, a scaler can now be cached and re-used.
  request->set_source(base::UnguessableToken::Create());
  scaler = TakeCachedScalerOrCreate(*request);
  const auto* a = scaler.get();
  EXPECT_TRUE(a);
  CacheScalerOrDelete(request->source(), std::move(scaler));
  EXPECT_TRUE(CacheContainsScaler(request->source(), 2, 1));
  scaler = TakeCachedScalerOrCreate(*request);
  const auto* b = scaler.get();
  EXPECT_TRUE(b);
  EXPECT_EQ(a, b);
  EXPECT_TRUE(b->IsSameScaleRatio(gfx::Vector2d(2, 2), gfx::Vector2d(1, 1)));
  CacheScalerOrDelete(request->source(), std::move(scaler));
  EXPECT_TRUE(CacheContainsScaler(request->source(), 2, 1));

  // With a source set in the request, but a different scaling ratio needed, the
  // cached scaler should go away and a new one created, and only the new one
  // should ever appear in the cache.
  request->SetUniformScaleRatio(3, 2);
  scaler = TakeCachedScalerOrCreate(*request);
  const auto* c = scaler.get();
  EXPECT_TRUE(c);
  EXPECT_TRUE(c->IsSameScaleRatio(gfx::Vector2d(3, 3), gfx::Vector2d(2, 2)));
  EXPECT_FALSE(CacheContainsScaler(request->source(), 2, 1));
  CacheScalerOrDelete(request->source(), std::move(scaler));
  EXPECT_TRUE(CacheContainsScaler(request->source(), 3, 2));
}

// Tests that cached resources are freed if unused for a while.
TEST_F(GLRendererCopierTest, FreesUnusedResources) {
  // Request a texture, then cache it again.
  const base::UnguessableToken source = base::UnguessableToken::Create();
  const int which = 0;
  const GLuint a = TakeCachedObjectOrCreate(source, which);
  EXPECT_NE(0u, a);
  CacheObjectOrDelete(source, which, a);
  EXPECT_TRUE(CacheContainsObject(source, which, a));

  // Call FreesUnusedCachedResources() the maximum number of times before the
  // cache entry would be considered for freeing.
  for (int i = 0; i < kKeepalivePeriod - 1; ++i) {
    FreeUnusedCachedResources();
    EXPECT_TRUE(CacheContainsObject(source, which, a));
    if (HasFailure())
      break;
  }

  // Calling FreeUnusedCachedResources() just one more time should cause the
  // cache entry to be freed.
  FreeUnusedCachedResources();
  EXPECT_FALSE(CacheContainsObject(source, which, a));
  EXPECT_EQ(0u, GetCopierCacheSize());
}

TEST_F(GLRendererCopierTest, DetectsBGRAForReadbackFormat) {
  test_gl()->SetOptimalReadbackFormat(GL_BGRA_EXT, GL_UNSIGNED_BYTE);
  EXPECT_EQ(static_cast<GLenum>(GL_BGRA_EXT), GetOptimalReadbackFormat());
}

TEST_F(GLRendererCopierTest, DetectsRGBAForReadbackFormat) {
  test_gl()->SetOptimalReadbackFormat(GL_RGBA, GL_UNSIGNED_BYTE);
  EXPECT_EQ(static_cast<GLenum>(GL_RGBA), GetOptimalReadbackFormat());
}

TEST_F(GLRendererCopierTest, FallsBackOnRGBAForReadbackFormat_BadFormat) {
  test_gl()->SetOptimalReadbackFormat(GL_RGB, GL_UNSIGNED_BYTE);
  EXPECT_EQ(static_cast<GLenum>(GL_RGBA), GetOptimalReadbackFormat());
}

TEST_F(GLRendererCopierTest, FallsBackOnRGBAForReadbackFormat_BadType) {
  test_gl()->SetOptimalReadbackFormat(GL_BGRA_EXT, GL_UNSIGNED_SHORT);
  EXPECT_EQ(static_cast<GLenum>(GL_RGBA), GetOptimalReadbackFormat());
}

}  // namespace viz
