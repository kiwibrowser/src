// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_WINDOWS_D3D11_TEXTURE_HELPER_H
#define DEVICE_VR_WINDOWS_D3D11_TEXTURE_HELPER_H

#include <D3D11_1.h>
#include <DXGI1_4.h>
#include <wrl.h>

#include "base/win/scoped_handle.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace device {

class D3D11TextureHelper {
 public:
  D3D11TextureHelper();
  ~D3D11TextureHelper();

  bool EnsureInitialized();
  bool SetAdapterIndex(int32_t index);
  bool SetAdapterLUID(const LUID& luid);

  bool CopyTextureToBackBuffer(bool flipY);  // Return true on success.
  void SetSourceTexture(base::win::ScopedHandle texture_handle);

  void AllocateBackBuffer();
  const Microsoft::WRL::ComPtr<ID3D11Texture2D>& GetBackbuffer();
  void SetBackbuffer(Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer);
  Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();

 private:
  bool CopyTextureWithFlip();
  bool EnsureRenderTargetView();
  bool EnsureShaders();
  bool EnsureInputLayout();
  bool EnsureVertexBuffer();
  bool EnsureSampler();
  Microsoft::WRL::ComPtr<IDXGIAdapter> GetAdapter();

  struct RenderState {
    RenderState();
    ~RenderState();

    Microsoft::WRL::ComPtr<ID3D11Device1> d3d11_device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context_;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> flip_pixel_shader_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> flip_vertex_shader_;

    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> target_texture_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> source_texture_;
    Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex_;
  };

  RenderState render_state_;
  int32_t adapter_index_ = -1;
  LUID adapter_luid_ = {};

  base::win::ScopedHandle texture_handle_;  // do we need to store this?
};
}  // namespace device

#endif  // DEVICE_VR_WINDOWS_D3D11_TEXTURE_HELPER_H
