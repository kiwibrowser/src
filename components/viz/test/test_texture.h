// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_TEST_TEXTURE_H_
#define COMPONENTS_VIZ_TEST_TEST_TEXTURE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <unordered_map>

#include "base/memory/ref_counted.h"
#include "components/viz/common/resources/resource_format.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

size_t TextureSizeBytes(const gfx::Size& size, ResourceFormat format);

struct TestTexture : public base::RefCounted<TestTexture> {
  TestTexture();

  void Reallocate(const gfx::Size& size, ResourceFormat format);
  bool IsValidParameter(GLenum pname);

  gfx::Size size;
  ResourceFormat format;
  std::unique_ptr<uint8_t[]> data;

  using TextureParametersMap = std::unordered_map<GLenum, GLint>;
  TextureParametersMap params;

 private:
  friend class base::RefCounted<TestTexture>;
  ~TestTexture();
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_TEST_TEXTURE_H_
