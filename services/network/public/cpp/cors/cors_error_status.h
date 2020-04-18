// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_CORS_CORS_ERROR_STATUS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_CORS_CORS_ERROR_STATUS_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/scoped_refptr.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/cors.mojom-shared.h"

namespace network {

struct COMPONENT_EXPORT(NETWORK_CPP_BASE) CORSErrorStatus {
  // This constructor is used by generated IPC serialization code.
  // Should not use this explicitly.
  // TODO(toyoshim, yhirano): Exploring a way to make this private, and allows
  // only serialization code for mojo can access.
  CORSErrorStatus();

  CORSErrorStatus(const CORSErrorStatus& status);

  explicit CORSErrorStatus(network::mojom::CORSError error);
  CORSErrorStatus(network::mojom::CORSError error,
                  const std::string& failed_parameter);

  ~CORSErrorStatus();

  bool operator==(const CORSErrorStatus& rhs) const;
  bool operator!=(const CORSErrorStatus& rhs) const { return !(*this == rhs); }

  network::mojom::CORSError cors_error;

  // Contains request method name, or header name that didn't pass a CORS check.
  std::string failed_parameter;
};

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_CORS_CORS_ERROR_STATUS_H_
