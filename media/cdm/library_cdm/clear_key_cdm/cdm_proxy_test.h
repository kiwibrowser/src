// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_PROXY_TEST_H_
#define MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_PROXY_TEST_H_

#include "base/callback.h"
#include "base/macros.h"
#include "media/cdm/api/content_decryption_module.h"

namespace media {

class CdmHostProxy;

class CdmProxyTest : public cdm::CdmProxyClient {
 public:
  using CompletionCB = base::OnceCallback<void(bool success)>;

  explicit CdmProxyTest(CdmHostProxy* cdm_host_proxy);
  ~CdmProxyTest() override;

  // Runs the test and returns the test result through |completion_cb|.
  void Run(CompletionCB completion_cb);

 private:
  void OnTestComplete(bool success);

  // cdm::CdmProxyClient implementation.
  void OnInitialized(Status status,
                     Protocol protocol,
                     uint32_t crypto_session_id) final;
  void OnProcessed(Status status,
                   const uint8_t* output_data,
                   uint32_t output_data_size) final;
  void OnMediaCryptoSessionCreated(Status status,
                                   uint32_t crypto_session_id,
                                   uint64_t output_data) final;
  void NotifyHardwareReset() final;

  CdmHostProxy* const cdm_host_proxy_ = nullptr;
  CompletionCB completion_cb_;
  cdm::CdmProxy* cdm_proxy_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CdmProxyTest);
};

}  // namespace media

#endif  // MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_PROXY_TEST_H_
