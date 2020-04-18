// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_dxgi.h"
#include <wrl.h>
#include <vector>
#include "base/trace_event/trace_event.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_image_dxgi.h"

namespace gpu {

GpuMemoryBufferFactoryDXGI::GpuMemoryBufferFactoryDXGI() {}

GpuMemoryBufferFactoryDXGI::~GpuMemoryBufferFactoryDXGI() {}

gfx::GpuMemoryBufferHandle GpuMemoryBufferFactoryDXGI::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int client_id,
    SurfaceHandle surface_handle) {
  TRACE_EVENT0("gpu", "GpuMemoryBufferFactoryDXGI::CreateGpuMemoryBuffer");

  gfx::GpuMemoryBufferHandle handle;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device =
      gl::QueryD3D11DeviceObjectFromANGLE();
  if (!d3d11_device)
    return handle;

  DXGI_FORMAT dxgi_format;
  switch (format) {
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBX_8888:
      dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
      break;
    default:
      NOTREACHED();
      return handle;
  }

  // We are binding as a shader resource and render target regardless of usage,
  // so make sure that the usage is one that we support.
  DCHECK(usage == gfx::BufferUsage::GPU_READ ||
         usage == gfx::BufferUsage::SCANOUT);

  D3D11_TEXTURE2D_DESC desc = {
      size.width(),
      size.height(),
      1,
      1,
      dxgi_format,
      {1, 0},
      D3D11_USAGE_DEFAULT,
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
      0,
      D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
          D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX};

  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture;

  if (FAILED(d3d11_device->CreateTexture2D(&desc, nullptr,
                                           d3d11_texture.GetAddressOf())))
    return handle;

  Microsoft::WRL::ComPtr<IDXGIResource1> dxgi_resource;
  if (FAILED(d3d11_texture.CopyTo(dxgi_resource.GetAddressOf())))
    return handle;

  HANDLE texture_handle;
  if (FAILED(dxgi_resource->CreateSharedHandle(
          nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
          nullptr, &texture_handle)))
    return handle;

  size_t buffer_size;
  if (!BufferSizeForBufferFormatChecked(size, format, &buffer_size))
    return handle;

  handle.dxgi_handle = IPC::PlatformFileForTransit(texture_handle);
  handle.type = gfx::DXGI_SHARED_HANDLE;
  handle.id = id;

  return handle;
}

void GpuMemoryBufferFactoryDXGI::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id) {}

ImageFactory* GpuMemoryBufferFactoryDXGI::AsImageFactory() {
  return this;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryDXGI::CreateImageForGpuMemoryBuffer(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat,
    int client_id,
    SurfaceHandle surface_handle) {
  if (handle.type != gfx::DXGI_SHARED_HANDLE)
    return nullptr;
  // Transfer ownership of handle to GLImageDXGIHandle.
  base::win::ScopedHandle handle_owner;
  handle_owner.Set(handle.dxgi_handle.GetHandle());
  auto image = base::MakeRefCounted<gl::GLImageDXGIHandle>(size, 0, format);
  if (!image->Initialize(std::move(handle_owner)))
    return nullptr;
  return image;
}

scoped_refptr<gl::GLImage> GpuMemoryBufferFactoryDXGI::CreateAnonymousImage(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    unsigned internalformat,
    bool* is_cleared) {
  NOTIMPLEMENTED();
  return nullptr;
}

unsigned GpuMemoryBufferFactoryDXGI::RequiredTextureType() {
  return GL_TEXTURE_2D;
}

bool GpuMemoryBufferFactoryDXGI::SupportsFormatRGB() {
  return true;
}
}  // namespace gpu
