// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/widevine_cdm_proxy_factory.h"

#include <comdef.h>
#include <initguid.h>

#include <iomanip>

#include "build/build_config.h"
#include "media/cdm/cdm_proxy.h"
#include "media/gpu/windows/d3d11_cdm_proxy.h"

namespace {

// Helpers for printing HRESULTs.
struct PrintHr {
  explicit PrintHr(HRESULT hr) : hr(hr) {}
  HRESULT hr;
};

std::ostream& operator<<(std::ostream& os, const PrintHr& phr) {
  std::ios_base::fmtflags ff = os.flags();
  os << _com_error(phr.hr).ErrorMessage() << " (" << std::showbase << std::hex
     << std::uppercase << std::setfill('0') << std::setw(8) << phr.hr << ")";
  os.flags(ff);
  return os;
}

// clang-format off
DEFINE_GUID(kD3D11ConfigWidevineStreamId,
  0x586e681, 0x4e14, 0x4133, 0x85, 0xe5, 0xa1, 0x4, 0x1f, 0x59, 0x9e, 0x26);
// clang-format on

}  // namespace

std::unique_ptr<media::CdmProxy> CreateWidevineCdmProxy() {
  Microsoft::WRL::ComPtr<ID3D11Device> device;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> video_device;

  // D3D11CdmProxy requires D3D_FEATURE_LEVEL_11_1.
  const D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};

  // Create device and pupulate |device|.
  HRESULT hresult = D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels,
      arraysize(feature_levels), D3D11_SDK_VERSION, device.GetAddressOf(),
      nullptr, nullptr);

  if (FAILED(hresult)) {
    DLOG(ERROR) << "Failed to create the D3D11Device: " << PrintHr(hresult);
    return nullptr;
  }

  hresult = device.CopyTo(video_device.GetAddressOf());
  if (FAILED(hresult)) {
    DLOG(ERROR) << "Failed to get ID3D11VideoDevice: " << PrintHr(hresult);
    return nullptr;
  }

  D3D11_VIDEO_CONTENT_PROTECTION_CAPS caps = {};

  // Check whether kD3D11ConfigWidevineStreamId is supported.
  // We do not care about decoder support so just use a null decoder profile.
  hresult = video_device->GetContentProtectionCaps(
      &kD3D11ConfigWidevineStreamId, nullptr, &caps);
  if (FAILED(hresult)) {
    DLOG(ERROR) << "Failed to GetContentProtectionCaps: " << PrintHr(hresult);
    return nullptr;
  }

  media::D3D11CdmProxy::FunctionIdMap function_id_map{
      {media::CdmProxy::Function::kIntelNegotiateCryptoSessionKeyExchange,
       0x90000001}};

  return std::make_unique<media::D3D11CdmProxy>(
      kD3D11ConfigWidevineStreamId,
      media::CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine,
      std::move(function_id_map));
}
