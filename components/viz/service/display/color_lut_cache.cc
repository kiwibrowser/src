// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/color_lut_cache.h"

#include <stdint.h>
#include <cmath>
#include <vector>

#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/color_transform.h"
#include "ui/gfx/half_float.h"

// After a LUT has not been used for this many frames, we release it.
const uint32_t kMaxFramesUnused = 10;

ColorLUTCache::ColorLUTCache(gpu::gles2::GLES2Interface* gl,
                             bool texture_half_float_linear)
    : lut_cache_(0),
      gl_(gl),
      texture_half_float_linear_(texture_half_float_linear) {}

ColorLUTCache::~ColorLUTCache() {
  GLuint textures[10];
  size_t n = 0;
  for (const auto& cache_entry : lut_cache_) {
    textures[n++] = cache_entry.second.lut.texture;
    if (n == arraysize(textures)) {
      gl_->DeleteTextures(n, textures);
      n = 0;
    }
  }
  if (n)
    gl_->DeleteTextures(n, textures);
}

namespace {

void FloatToLUT(const float* f, gfx::HalfFloat* out, size_t num) {
  gfx::FloatToHalfFloat(f, out, num);
}

void FloatToLUT(float* f, unsigned char* out, size_t num) {
  for (size_t i = 0; i < num; i++) {
    out[i] = std::min<int>(255, std::max<int>(0, floorf(f[i] * 255.0f + 0.5f)));
  }
}

}  // namespace

template <typename T>
unsigned int ColorLUTCache::MakeLUT(const gfx::ColorTransform* transform,
                                    int lut_samples) {
  int lut_entries = lut_samples * lut_samples * lut_samples;
  float inverse = 1.0f / (lut_samples - 1);
  std::vector<T> lut(lut_entries * 4);
  std::vector<gfx::ColorTransform::TriStim> samples(lut_samples);
  T* lutp = lut.data();
  float one = 1.0f;
  T alpha;
  FloatToLUT(&one, &alpha, 1);
  for (int v = 0; v < lut_samples; v++) {
    for (int u = 0; u < lut_samples; u++) {
      for (int y = 0; y < lut_samples; y++) {
        samples[y].set_x(y * inverse);
        samples[y].set_y(u * inverse);
        samples[y].set_z(v * inverse);
      }
      transform->Transform(samples.data(), samples.size());
      T* lutp2 = lutp + lut_samples;
      FloatToLUT(reinterpret_cast<float*>(samples.data()), lutp2,
                 lut_samples * 3);
      for (int i = 0; i < lut_samples; i++) {
        *(lutp++) = *(lutp2++);
        *(lutp++) = *(lutp2++);
        *(lutp++) = *(lutp2++);
        *(lutp++) = alpha;
      }
    }
  }

  GLuint previously_bound_texture = 0;
  GLuint lut_texture = 0;
  gl_->GetIntegerv(GL_TEXTURE_BINDING_2D,
                   reinterpret_cast<GLint*>(&previously_bound_texture));
  gl_->GenTextures(1, &lut_texture);
  gl_->BindTexture(GL_TEXTURE_2D, lut_texture);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl_->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lut_samples,
                  lut_samples * lut_samples, 0, GL_RGBA,
                  sizeof(T) == 1 ? GL_UNSIGNED_BYTE : GL_HALF_FLOAT_OES,
                  lut.data());
  gl_->BindTexture(GL_TEXTURE_2D, previously_bound_texture);
  return lut_texture;
}

ColorLUTCache::LUT ColorLUTCache::GetLUT(const gfx::ColorTransform* transform) {
  auto iter = lut_cache_.Get(transform);
  if (iter != lut_cache_.end()) {
    iter->second.last_used_frame = current_frame_;
    return iter->second.lut;
  }

  LUT lut;
  // If input is HDR, and the output is full range, we're going to need
  // to produce values outside of 0-1, so we'll need to make a half-float
  // LUT. Also, we'll need to build a larger lut to maintain accuracy.
  // All LUT sizes should be odd as some transforms have a knee at 0.5.
  if (transform->GetDstColorSpace().FullRangeEncodedValues() &&
      transform->GetSrcColorSpace().IsHDR() && texture_half_float_linear_) {
    lut.size = 37;
    lut.texture = MakeLUT<uint16_t>(transform, lut.size);
  } else {
    lut.size = 17;
    lut.texture = MakeLUT<unsigned char>(transform, lut.size);
  }
  lut_cache_.Put(transform, CacheVal(lut, current_frame_));
  return lut;
}

void ColorLUTCache::Swap() {
  current_frame_++;
  while (!lut_cache_.empty() &&
         current_frame_ - lut_cache_.rbegin()->second.last_used_frame >
             kMaxFramesUnused) {
    gl_->DeleteTextures(1, &lut_cache_.rbegin()->second.lut.texture);
    lut_cache_.ShrinkToSize(lut_cache_.size() - 1);
  }
}
