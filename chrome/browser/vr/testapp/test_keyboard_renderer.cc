// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/testapp/test_keyboard_renderer.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "chrome/browser/vr/skia_surface_provider.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/codec/png_codec.h"

namespace vr {

namespace {
constexpr char kKeyboardImagePath[] = "chrome/browser/vr/testapp/keyboard.png";
}  // namespace

TestKeyboardRenderer::TestKeyboardRenderer() = default;
TestKeyboardRenderer::~TestKeyboardRenderer() = default;

void TestKeyboardRenderer::Initialize(SkiaSurfaceProvider* provider,
                                      UiElementRenderer* renderer) {
  renderer_ = renderer;

  // Note that we simply render an image for the keyboard and the actual input
  // is provided by the physical keyboard.
  // Read and decode keyboard image.
  base::FilePath dir;
  base::PathService::Get(base::DIR_CURRENT, &dir);
  dir = dir.Append(base::FilePath().AppendASCII(kKeyboardImagePath));
  DCHECK(base::PathExists(dir));
  std::string file_contents;
  base::ReadFileToString(dir, &file_contents);
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(file_contents.data());
  SkBitmap bitmap;
  gfx::PNGCodec::Decode(data, file_contents.length(), &bitmap);

  drawn_size_.SetSize(bitmap.width(), bitmap.height());
  surface_ = provider->MakeSurface(drawn_size_);
  DCHECK(surface_);

  surface_->getCanvas()->drawBitmap(bitmap, 0, 0);
  texture_handle_ = provider->FlushSurface(surface_.get(), texture_handle_);
}

void TestKeyboardRenderer::Draw(const CameraModel& model,
                                const gfx::Transform& world_space_transform) {
  renderer_->DrawTexturedQuad(
      texture_handle_, 0, UiElementRenderer::kTextureLocationLocal,
      model.view_proj_matrix * world_space_transform, gfx::RectF(0, 0, 1, 1), 1,
      {drawn_size_.width(), drawn_size_.height()}, 0, true /* blend */);
}

}  // namespace vr
