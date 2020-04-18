// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/library_cdm/clear_key_cdm/clear_key_cdm_proxy.h"

#include "base/bind_helpers.h"
#include "base/logging.h"
#include "media/base/content_decryption_module.h"
#include "media/cdm/library_cdm/clear_key_cdm/cdm_proxy_common.h"

namespace media {

ClearKeyCdmProxy::ClearKeyCdmProxy() : weak_factory_(this) {}

ClearKeyCdmProxy::~ClearKeyCdmProxy() {}

base::WeakPtr<CdmContext> ClearKeyCdmProxy::GetCdmContext() {
  DVLOG(1) << __func__;
  return weak_factory_.GetWeakPtr();
}

void ClearKeyCdmProxy::Initialize(Client* client, InitializeCB init_cb) {
  DVLOG(1) << __func__;

  std::move(init_cb).Run(
      Status::kOk, Protocol::kIntelConvergedSecurityAndManageabilityEngine,
      kClearKeyCdmProxyCryptoSessionId);
}

void ClearKeyCdmProxy::Process(Function function,
                               uint32_t crypto_session_id,
                               const std::vector<uint8_t>& input_data,
                               uint32_t expected_output_data_size,
                               ProcessCB process_cb) {
  DVLOG(2) << __func__;

  if (crypto_session_id != kClearKeyCdmProxyCryptoSessionId ||
      !std::equal(input_data.begin(), input_data.end(),
                  kClearKeyCdmProxyInputData.begin(),
                  kClearKeyCdmProxyInputData.end())) {
    std::move(process_cb).Run(Status::kFail, {});
    return;
  }

  std::move(process_cb)
      .Run(Status::kOk,
           std::vector<uint8_t>(kClearKeyCdmProxyOutputData.begin(),
                                kClearKeyCdmProxyOutputData.end()));
}

void ClearKeyCdmProxy::CreateMediaCryptoSession(
    const std::vector<uint8_t>& input_data,
    CreateMediaCryptoSessionCB create_media_crypto_session_cb) {
  DVLOG(2) << __func__;

  if (!std::equal(input_data.begin(), input_data.end(),
                  kClearKeyCdmProxyInputData.begin(),
                  kClearKeyCdmProxyInputData.end())) {
    std::move(create_media_crypto_session_cb).Run(Status::kFail, 0, 0);
    return;
  }

  std::move(create_media_crypto_session_cb)
      .Run(Status::kOk, kClearKeyCdmProxyMediaCryptoSessionId, 0);
}

void ClearKeyCdmProxy::SetKey(uint32_t crypto_session_id,
                              const std::vector<uint8_t>& key_id,
                              const std::vector<uint8_t>& key_blob) {}

void ClearKeyCdmProxy::RemoveKey(uint32_t crypto_session_id,
                                 const std::vector<uint8_t>& key_id) {}

Decryptor* ClearKeyCdmProxy::GetDecryptor() {
  DVLOG(1) << __func__;

  if (!aes_decryptor_) {
    aes_decryptor_ = base::MakeRefCounted<AesDecryptor>(
        base::DoNothing(), base::DoNothing(), base::DoNothing(),
        base::DoNothing());
  }

  return aes_decryptor_.get();
}

}  // namespace media
