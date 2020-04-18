// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/windows/d3d11_texture_helper.h"
#include "mojo/public/c/system/platform_handle.h"

namespace {
#include "device/vr/windows/flip_pixel_shader.h"
#include "device/vr/windows/flip_vertex_shader.h"

constexpr int kAcquireWaitMS = 2000;
}

namespace device {

D3D11TextureHelper::RenderState::RenderState() {}
D3D11TextureHelper::RenderState::~RenderState() {}

D3D11TextureHelper::D3D11TextureHelper() {}

D3D11TextureHelper::~D3D11TextureHelper() {}

bool D3D11TextureHelper::CopyTextureToBackBuffer(bool flipY) {
  if (!EnsureInitialized())
    return false;
  if (!render_state_.source_texture_)
    return false;
  if (!render_state_.target_texture_)
    return false;

  HRESULT hr = render_state_.keyed_mutex_->AcquireSync(1, kAcquireWaitMS);
  if (FAILED(hr) || hr == WAIT_TIMEOUT || hr == WAIT_ABANDONED) {
    // We failed to acquire the lock.  We'll drop this frame, but subsequent
    // frames won't be affected.
    return false;
  }

  bool success = true;
  if (flipY) {
    success = CopyTextureWithFlip();
  } else {
    render_state_.d3d11_device_context_->CopyResource(
        render_state_.target_texture_.Get(),
        render_state_.source_texture_.Get());
  }

  render_state_.keyed_mutex_->ReleaseSync(0);
  return success;
}

bool D3D11TextureHelper::EnsureRenderTargetView() {
  if (!render_state_.render_target_view_) {
    D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
    render_target_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    render_target_view_desc.Texture2D.MipSlice = 0;
    HRESULT hr = render_state_.d3d11_device_->CreateRenderTargetView(
        render_state_.target_texture_.Get(), &render_target_view_desc,
        &render_state_.render_target_view_);
    if (FAILED(hr))
      return false;
  }
  return true;
}

bool D3D11TextureHelper::EnsureShaders() {
  if (!render_state_.flip_vertex_shader_) {
    HRESULT hr = render_state_.d3d11_device_->CreateVertexShader(
        g_flip_vertex, _countof(g_flip_vertex), nullptr,
        &render_state_.flip_vertex_shader_);
    if (FAILED(hr))
      return false;
  }

  if (!render_state_.flip_pixel_shader_) {
    HRESULT hr = render_state_.d3d11_device_->CreatePixelShader(
        g_flip_pixel, _countof(g_flip_pixel), nullptr,
        &render_state_.flip_pixel_shader_);
    if (FAILED(hr))
      return false;
  }

  return true;
}

bool D3D11TextureHelper::EnsureInputLayout() {
  if (!render_state_.input_layout_) {
    D3D11_INPUT_ELEMENT_DESC vertex_desc = {
        "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,
        0,          0, D3D11_INPUT_PER_VERTEX_DATA,
        0};
    HRESULT hr = render_state_.d3d11_device_->CreateInputLayout(
        &vertex_desc, 1, g_flip_vertex, _countof(g_flip_vertex),
        &render_state_.input_layout_);
    if (FAILED(hr))
      return false;
  }
  return true;
}

bool D3D11TextureHelper::EnsureVertexBuffer() {
  if (!render_state_.vertex_buffer_) {
    // Pairs of x/y coordinates for 2 triangles in a quad.
    float buffer_data[] = {-1, -1, -1, 1, 1, -1, -1, 1, 1, 1, 1, -1};
    D3D11_SUBRESOURCE_DATA vertex_buffer_data;
    vertex_buffer_data.pSysMem = buffer_data;
    vertex_buffer_data.SysMemPitch = 0;
    vertex_buffer_data.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC vertex_buffer_desc(sizeof(buffer_data),
                                          D3D11_BIND_VERTEX_BUFFER);
    HRESULT hr = render_state_.d3d11_device_->CreateBuffer(
        &vertex_buffer_desc, &vertex_buffer_data,
        &render_state_.vertex_buffer_);
    if (FAILED(hr))
      return false;
  }
  return true;
}

bool D3D11TextureHelper::EnsureSampler() {
  if (!render_state_.sampler_) {
    CD3D11_DEFAULT default_values;
    CD3D11_SAMPLER_DESC sampler_desc = CD3D11_SAMPLER_DESC(default_values);
    D3D11_SAMPLER_DESC sd = sampler_desc;
    HRESULT hr = render_state_.d3d11_device_->CreateSamplerState(
        &sd, render_state_.sampler_.GetAddressOf());
    if (FAILED(hr))
      return false;
  }
  return true;
}

bool D3D11TextureHelper::CopyTextureWithFlip() {
  if (!EnsureRenderTargetView() || !EnsureShaders() || !EnsureInputLayout() ||
      !EnsureVertexBuffer() || !EnsureSampler())
    return false;

  render_state_.d3d11_device_context_->OMSetRenderTargets(
      1, render_state_.render_target_view_.GetAddressOf(), nullptr);

  render_state_.d3d11_device_context_->VSSetShader(
      render_state_.flip_vertex_shader_.Get(), nullptr, 0);
  render_state_.d3d11_device_context_->PSSetShader(
      render_state_.flip_pixel_shader_.Get(), nullptr, 0);
  render_state_.d3d11_device_context_->IASetInputLayout(
      render_state_.input_layout_.Get());

  UINT stride = 2 * sizeof(float);
  UINT offset = 0;
  render_state_.d3d11_device_context_->IASetVertexBuffers(
      0, 1, render_state_.vertex_buffer_.GetAddressOf(), &stride, &offset);
  render_state_.d3d11_device_context_->PSSetSamplers(
      0, 1, render_state_.sampler_.GetAddressOf());

  D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
  shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  shader_resource_view_desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
  shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
  shader_resource_view_desc.Texture2D.MipLevels = 1;
  HRESULT hr = render_state_.d3d11_device_->CreateShaderResourceView(
      render_state_.source_texture_.Get(), &shader_resource_view_desc,
      render_state_.shader_resource_.ReleaseAndGetAddressOf());
  if (FAILED(hr))
    return false;
  render_state_.d3d11_device_context_->PSSetShaderResources(
      0, 1, render_state_.shader_resource_.GetAddressOf());

  D3D11_TEXTURE2D_DESC desc;
  render_state_.target_texture_->GetDesc(&desc);
  D3D11_VIEWPORT viewport = {0, 0, desc.Width, desc.Height, 0, 1};
  render_state_.d3d11_device_context_->RSSetViewports(1, &viewport);
  render_state_.d3d11_device_context_->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  render_state_.d3d11_device_context_->Draw(6, 0);
  return true;
}

void D3D11TextureHelper::SetSourceTexture(
    base::win::ScopedHandle texture_handle) {
  render_state_.source_texture_ = nullptr;
  render_state_.keyed_mutex_ = nullptr;

  if (!EnsureInitialized())
    return;
  texture_handle_ = std::move(texture_handle);
  HRESULT hr = render_state_.d3d11_device_->OpenSharedResource1(
      texture_handle_.Get(),
      IID_PPV_ARGS(render_state_.keyed_mutex_.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return;
  hr = render_state_.keyed_mutex_.CopyTo(
      render_state_.source_texture_.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    render_state_.keyed_mutex_ = nullptr;
  }
}

void D3D11TextureHelper::AllocateBackBuffer() {
  if (!EnsureInitialized())
    return;
  if (!render_state_.source_texture_)
    return;

  D3D11_TEXTURE2D_DESC desc_source;
  render_state_.source_texture_->GetDesc(&desc_source);
  desc_source.MiscFlags = 0;

  if (render_state_.target_texture_) {
    D3D11_TEXTURE2D_DESC desc_target;
    render_state_.target_texture_->GetDesc(&desc_target);
    // If the target should change size, format, or other properties reallocate
    // a new target.
    if (desc_source.Width != desc_target.Width ||
        desc_source.Height != desc_target.Height ||
        desc_source.MipLevels != desc_target.MipLevels ||
        desc_source.ArraySize != desc_target.ArraySize ||
        desc_source.Format != desc_target.Format ||
        desc_source.SampleDesc.Count != desc_target.SampleDesc.Count ||
        desc_source.SampleDesc.Quality != desc_target.SampleDesc.Quality ||
        desc_source.Usage != desc_target.Usage ||
        desc_source.BindFlags != desc_target.BindFlags ||
        desc_source.CPUAccessFlags != desc_target.CPUAccessFlags ||
        desc_source.MiscFlags != desc_target.MiscFlags) {
      render_state_.target_texture_ = nullptr;
    }
  }

  if (!render_state_.target_texture_) {
    // Ignoring error - target_texture_ will be null on failure.
    render_state_.d3d11_device_->CreateTexture2D(
        &desc_source, nullptr,
        render_state_.target_texture_.ReleaseAndGetAddressOf());
  }
}

const Microsoft::WRL::ComPtr<ID3D11Texture2D>&
D3D11TextureHelper::GetBackbuffer() {
  return render_state_.target_texture_;
}

void D3D11TextureHelper::SetBackbuffer(
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer) {
  if (render_state_.target_texture_ != back_buffer) {
    render_state_.render_target_view_ = nullptr;
  }
  render_state_.target_texture_ = back_buffer;
}

Microsoft::WRL::ComPtr<IDXGIAdapter> D3D11TextureHelper::GetAdapter() {
  Microsoft::WRL::ComPtr<IDXGIFactory1> dxgi_factory;
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
  if (FAILED(hr))
    return nullptr;
  if (adapter_index_ >= 0) {
    dxgi_factory->EnumAdapters(adapter_index_, adapter.GetAddressOf());
  } else {
    // We don't have a valid adapter index, lets see if we have a valid LUID.
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory4;
    hr = dxgi_factory.As(&dxgi_factory4);
    if (FAILED(hr))
      return nullptr;
    dxgi_factory4->EnumAdapterByLuid(adapter_luid_,
                                     IID_PPV_ARGS(adapter.GetAddressOf()));
  }
  return adapter;
}

Microsoft::WRL::ComPtr<ID3D11Device> D3D11TextureHelper::GetDevice() {
  EnsureInitialized();
  return render_state_.d3d11_device_;
}

bool D3D11TextureHelper::EnsureInitialized() {
  if (render_state_.d3d11_device_ &&
      SUCCEEDED(render_state_.d3d11_device_->GetDeviceRemovedReason()))
    return true;  // Already initialized.

  // If we were previously initialized, but lost the device, throw away old
  // state.  This will be initialized lazily as needed.
  render_state_ = {};

  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
  UINT flags = 0;
  D3D_FEATURE_LEVEL feature_level_out = D3D_FEATURE_LEVEL_11_1;

  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter = GetAdapter();
  if (!adapter) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
  HRESULT hr = D3D11CreateDevice(
      adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, feature_levels,
      arraysize(feature_levels), D3D11_SDK_VERSION, d3d11_device.GetAddressOf(),
      &feature_level_out, render_state_.d3d11_device_context_.GetAddressOf());
  if (SUCCEEDED(hr)) {
    hr = d3d11_device.As(&render_state_.d3d11_device_);
    if (FAILED(hr)) {
      render_state_.d3d11_device_context_ = nullptr;
    }
  }
  return SUCCEEDED(hr);
}

bool D3D11TextureHelper::SetAdapterIndex(int32_t index) {
  adapter_index_ = index;
  return (index >= 0);
}

bool D3D11TextureHelper::SetAdapterLUID(const LUID& luid) {
  adapter_luid_ = luid;
  adapter_index_ = -1;
  return true;
}

}  // namespace device
