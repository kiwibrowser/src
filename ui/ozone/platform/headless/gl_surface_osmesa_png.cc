// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/headless/gl_surface_osmesa_png.h"

#include <vector>

#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/gfx/codec/png_codec.h"

namespace ui {
namespace {

constexpr int kBytesPerPixelBGRA = 4;

void WritePngToFile(const base::FilePath& path,
                    std::vector<unsigned char> png_data) {
  DCHECK(!path.empty());
  base::WriteFile(path, reinterpret_cast<const char*>(png_data.data()),
                  png_data.size());
}

}  // namespace

GLSurfaceOSMesaPng::GLSurfaceOSMesaPng(base::FilePath output_path)
    : GLSurfaceOSMesa(
          gl::GLSurfaceFormat(gl::GLSurfaceFormat::PIXEL_LAYOUT_BGRA),
          gfx::Size(1, 1)),
      output_path_(output_path) {}

bool GLSurfaceOSMesaPng::IsOffscreen() {
  return false;
}

gfx::SwapResult GLSurfaceOSMesaPng::SwapBuffers(
    const PresentationCallback& callback) {
  if (!output_path_.empty())
    WriteBufferToPng();

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(callback, gfx::PresentationFeedback(base::TimeTicks::Now(),
                                                         base::TimeDelta(),
                                                         0 /* flags */)));
  return gfx::SwapResult::SWAP_ACK;
}

bool GLSurfaceOSMesaPng::SupportsPresentationCallback() {
  return true;
}

GLSurfaceOSMesaPng::~GLSurfaceOSMesaPng() {
  Destroy();
}

void GLSurfaceOSMesaPng::WriteBufferToPng() {
  // TODO(crbug.com/783792): Writing the PNG to a file won't work with the GPU
  // sandbox. This will produce no output unless --no-sandbox is used. Make this
  // work with a file handle passed from browser process and possibly change
  // output to be video.
  gfx::Size size = GetSize();
  std::vector<unsigned char> png_data;
  if (gfx::PNGCodec::Encode(static_cast<unsigned char*>(GetHandle()),
                            gfx::PNGCodec::FORMAT_BGRA, size,
                            size.width() * kBytesPerPixelBGRA,
                            false /* discard_transparency */,
                            std::vector<gfx::PNGCodec::Comment>(), &png_data)) {
    base::PostTaskWithTraits(
        FROM_HERE,
        {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(&WritePngToFile, output_path_, std::move(png_data)));
  }
}

}  // namespace ui
