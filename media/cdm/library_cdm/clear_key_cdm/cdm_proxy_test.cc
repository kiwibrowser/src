// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/library_cdm/clear_key_cdm/cdm_proxy_test.h"

#include <stdint.h>
#include <algorithm>

#include "base/logging.h"
#include "base/macros.h"
#include "media/cdm/library_cdm/cdm_host_proxy.h"
#include "media/cdm/library_cdm/clear_key_cdm/cdm_proxy_common.h"

namespace media {

CdmProxyTest::CdmProxyTest(CdmHostProxy* cdm_host_proxy)
    : cdm_host_proxy_(cdm_host_proxy) {}

CdmProxyTest::~CdmProxyTest() {}

void CdmProxyTest::Run(CompletionCB completion_cb) {
  DVLOG(1) << __func__;
  completion_cb_ = std::move(completion_cb);

  cdm_proxy_ = cdm_host_proxy_->RequestCdmProxy(this);
  if (!cdm_proxy_) {
    OnTestComplete(false);
    return;
  }

  cdm_proxy_->Initialize();
}

void CdmProxyTest::OnTestComplete(bool success) {
  DVLOG(1) << __func__ << ": success = " << success;
  std::move(completion_cb_).Run(success);
}

void CdmProxyTest::OnInitialized(Status status,
                                 Protocol protocol,
                                 uint32_t crypto_session_id) {
  DVLOG(1) << __func__ << ": status = " << status;

  if (status != Status::kOk ||
      crypto_session_id != kClearKeyCdmProxyCryptoSessionId) {
    OnTestComplete(false);
    return;
  }

  // Only one CdmProxy can be created during the lifetime of the CDM instance.
  if (cdm_host_proxy_->RequestCdmProxy(this)) {
    OnTestComplete(false);
    return;
  }

  cdm_proxy_->Process(cdm::CdmProxy::kIntelNegotiateCryptoSessionKeyExchange,
                      crypto_session_id, kClearKeyCdmProxyInputData.data(),
                      kClearKeyCdmProxyInputData.size(), 0);
}

void CdmProxyTest::OnProcessed(Status status,
                               const uint8_t* output_data,
                               uint32_t output_data_size) {
  DVLOG(1) << __func__ << ": status = " << status;

  if (status != Status::kOk ||
      !std::equal(output_data, output_data + output_data_size,
                  kClearKeyCdmProxyOutputData.begin())) {
    OnTestComplete(false);
    return;
  }

  cdm_proxy_->CreateMediaCryptoSession(kClearKeyCdmProxyInputData.data(),
                                       kClearKeyCdmProxyInputData.size());
}

void CdmProxyTest::OnMediaCryptoSessionCreated(Status status,
                                               uint32_t crypto_session_id,
                                               uint64_t output_data) {
  DVLOG(1) << __func__ << ": status = " << status;

  if (status != Status::kOk ||
      crypto_session_id != kClearKeyCdmProxyMediaCryptoSessionId) {
    OnTestComplete(false);
    return;
  }

  OnTestComplete(true);
}

void CdmProxyTest::NotifyHardwareReset() {
  DVLOG(1) << __func__;
  NOTREACHED();
}

}  // namespace media
