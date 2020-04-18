// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_HEADLESS_GL_SURFACE_OSMESA_PNG_H_
#define UI_OZONE_PLATFORM_HEADLESS_GL_SURFACE_OSMESA_PNG_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "ui/gl/gl_surface_osmesa.h"

namespace ui {

class GLSurfaceOSMesaPng : public gl::GLSurfaceOSMesa {
 public:
  explicit GLSurfaceOSMesaPng(base::FilePath output_path);

  // gl::GLSurfaceOSMesa:
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  bool SupportsPresentationCallback() override;

 private:
  ~GLSurfaceOSMesaPng() override;

  // Write contents of buffer out to PNG file at |output_path_|.
  void WriteBufferToPng();

  // If not empty then buffer contents will be written out to this location on
  // swap.
  base::FilePath output_path_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesaPng);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_HEADLESS_GL_SURFACE_OSMESA_PNG_H_
