// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_PROXY_COMMON_H_
#define MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_PROXY_COMMON_H_

#include <stdint.h>
#include <array>

namespace media {

// Constants used to test CdmProxy stack using ClearKeyCdm and ClearKeyCdmProxy.
constexpr uint32_t kClearKeyCdmProxyCryptoSessionId = 1;
constexpr uint32_t kClearKeyCdmProxyMediaCryptoSessionId = 23;
constexpr std::array<uint8_t, 3> kClearKeyCdmProxyInputData = {4, 5, 6};
constexpr std::array<uint8_t, 4> kClearKeyCdmProxyOutputData = {7, 8, 9, 10};

}  // namespace media

#endif  // MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_CDM_PROXY_COMMON_H_
