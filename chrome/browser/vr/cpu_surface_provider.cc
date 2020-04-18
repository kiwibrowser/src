// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/cpu_surface_provider.h"

#include "cc/paint/skia_paint_canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/GrContextOptions.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "ui/gfx/geometry/size.h"

namespace vr {

CpuSurfaceProvider::CpuSurfaceProvider() = default;

CpuSurfaceProvider::~CpuSurfaceProvider() = default;

sk_sp<SkSurface> CpuSurfaceProvider::MakeSurface(const gfx::Size& size) {
  return SkSurface::MakeRasterN32Premul(size.width(), size.height());
}

GLuint CpuSurfaceProvider::FlushSurface(SkSurface* surface,
                                        GLuint reuse_texture_id) {
  GLuint texture_id = reuse_texture_id;
  if (texture_id == 0) {
    glGenTextures(1, &texture_id);
  }

  cc::SkiaPaintCanvas paint_canvas(surface->getCanvas());
  paint_canvas.flush();
  SkPixmap pixmap;
  CHECK(surface->peekPixels(&pixmap));

  SkColorType type = pixmap.colorType();
  DCHECK(type == kRGBA_8888_SkColorType || type == kBGRA_8888_SkColorType);
  GLint format = (type == kRGBA_8888_SkColorType ? GL_RGBA : GL_BGRA);

  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, format, pixmap.width(), pixmap.height(), 0,
               format, GL_UNSIGNED_BYTE, pixmap.addr());

  return texture_id;
}

}  // namespace vr
