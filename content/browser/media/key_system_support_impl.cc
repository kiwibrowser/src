// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/key_system_support_impl.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/common/cdm_info.h"
#include "media/base/key_system_names.h"
#include "media/base/key_systems.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

void SendCdmAvailableUMA(const std::string& key_system, bool available) {
  base::UmaHistogramBoolean("Media.EME." +
                                media::GetKeySystemNameForUMA(key_system) +
                                ".LibraryCdmAvailable",
                            available);
}

}  // namespace

// static
void KeySystemSupportImpl::Create(
    media::mojom::KeySystemSupportRequest request) {
  DVLOG(3) << __func__;
  // The created object is bound to (and owned by) |request|.
  mojo::MakeStrongBinding(std::make_unique<KeySystemSupportImpl>(),
                          std::move(request));
}

// static
std::unique_ptr<CdmInfo> KeySystemSupportImpl::GetCdmInfoForKeySystem(
    const std::string& key_system) {
  DVLOG(2) << __func__ << ": key_system = " << key_system;
  for (const auto& cdm : CdmRegistry::GetInstance()->GetAllRegisteredCdms()) {
    if (cdm.supported_key_system == key_system ||
        (cdm.supports_sub_key_systems &&
         media::IsChildKeySystemOf(key_system, cdm.supported_key_system))) {
      return std::make_unique<CdmInfo>(cdm);
    }
  }

  return nullptr;
}

KeySystemSupportImpl::KeySystemSupportImpl() = default;

KeySystemSupportImpl::~KeySystemSupportImpl() = default;

void KeySystemSupportImpl::IsKeySystemSupported(
    const std::string& key_system,
    IsKeySystemSupportedCallback callback) {
  DVLOG(3) << __func__;
  std::unique_ptr<CdmInfo> cdm = GetCdmInfoForKeySystem(key_system);
  if (!cdm) {
    SendCdmAvailableUMA(key_system, false);
    std::move(callback).Run(false, {}, false, {});
    return;
  }

  const base::flat_set<media::EncryptionMode>& schemes =
      cdm->supported_encryption_schemes;
  SendCdmAvailableUMA(key_system, true);
  std::move(callback).Run(
      true, cdm->supported_video_codecs, cdm->supports_persistent_license,
      std::vector<media::EncryptionMode>(schemes.begin(), schemes.end()));
}

}  // namespace content
