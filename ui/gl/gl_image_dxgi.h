// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_IMAGE_DXGI_H_
#define UI_GL_GL_IMAGE_DXGI_H_

#include <DXGI1_2.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "base/win/scoped_handle.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_image.h"

typedef void* EGLStreamKHR;
typedef void* EGLConfig;
typedef void* EGLSurface;

namespace gl {

// TODO(776010): Reconcile the different GLImageDXGI types.  Remove
// GLImageDXGIHandle, and move its implementation into GLImageDXGI.
class GL_EXPORT GLImageDXGIBase : public GLImage {
 public:
  GLImageDXGIBase(const gfx::Size& size);

  // Safe downcast. Returns nullptr on failure.
  static GLImageDXGIBase* FromGLImage(GLImage* image);

  // GLImage implementation.
  gfx::Size GetSize() override;
  unsigned GetInternalFormat() override;
  bool BindTexImage(unsigned target) override;
  void ReleaseTexImage(unsigned target) override;
  bool CopyTexImage(unsigned target) override;
  bool CopyTexSubImage(unsigned target,
                       const gfx::Point& offset,
                       const gfx::Rect& rect) override;
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int z_order,
                            gfx::OverlayTransform transform,
                            const gfx::Rect& bounds_rect,
                            const gfx::RectF& crop_rect,
                            bool enable_blend,
                            gfx::GpuFence* gpu_fence) override;
  void SetColorSpace(const gfx::ColorSpace& color_space) override;
  void Flush() override;
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                    uint64_t process_tracing_id,
                    const std::string& dump_name) override;
  Type GetType() const override;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture() { return texture_; }
  const gfx::ColorSpace& color_space() const { return color_space_; }

  size_t level() const { return level_; }
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex() { return keyed_mutex_; }

 protected:
  ~GLImageDXGIBase() override;

  gfx::Size size_;
  gfx::ColorSpace color_space_;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex_;
  size_t level_ = 0;
};

class GL_EXPORT GLImageDXGI : public GLImageDXGIBase {
 public:
  GLImageDXGI(const gfx::Size& size, EGLStreamKHR stream);

  // GLImage implementation.
  bool BindTexImage(unsigned target) override;

  void SetTexture(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& texture,
                  size_t level);

 protected:
  ~GLImageDXGI() override;

  EGLStreamKHR stream_;
};

// This copies to a new texture on bind.
class GL_EXPORT CopyingGLImageDXGI : public GLImageDXGI {
 public:
  CopyingGLImageDXGI(const Microsoft::WRL::ComPtr<ID3D11Device>& d3d11_device,
                     const gfx::Size& size,
                     EGLStreamKHR stream);

  bool Initialize();
  bool InitializeVideoProcessor(
      const Microsoft::WRL::ComPtr<ID3D11VideoProcessor>& video_processor,
      const Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator>& enumerator);
  void UnbindFromTexture();

  // GLImage implementation.
  bool BindTexImage(unsigned target) override;

 private:
  ~CopyingGLImageDXGI() override;

  bool copied_ = false;

  Microsoft::WRL::ComPtr<ID3D11VideoDevice> video_device_;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> video_context_;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessor> d3d11_processor_;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> enumerator_;
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> decoder_copy_texture_;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorOutputView> output_view_;
};

class GL_EXPORT GLImageDXGIHandle : public GLImageDXGIBase {
 public:
  GLImageDXGIHandle(const gfx::Size& size,
                    uint32_t level,
                    gfx::BufferFormat format);

  bool Initialize(base::win::ScopedHandle handle);

  // GLImage implementation.
  bool BindTexImage(unsigned target) override;
  unsigned GetInternalFormat() override;
  void ReleaseTexImage(unsigned target) override;

 protected:
  ~GLImageDXGIHandle() override;

  EGLSurface surface_ = nullptr;
  base::win::ScopedHandle handle_;
  gfx::BufferFormat format_;
};
}

#endif  // UI_GL_GL_IMAGE_DXGI_H_
