// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(jophba): Need unittests.

#ifndef PLATFORM_API_TLS_SOCKET_CREDS_H_
#define PLATFORM_API_TLS_SOCKET_CREDS_H_

#include <memory>
#include <string>
#include <vector>

#include "osp_base/macros.h"

namespace openscreen {

struct TlsSocketCreds {
 public:
  // TODO(jophba): add TLS certificate
  // TODO(jophba): add RSA Private key
  std::vector<uint8_t> tls_cert_der;
  std::vector<uint8_t> tls_pk_sha256_hash;

  std::vector<uint8_t> private_key_base64;
  std::vector<uint8_t> public_key_base64;
};

}  // namespace openscreen

#endif  // PLATFORM_API_TLS_SOCKET_CREDS_H_
