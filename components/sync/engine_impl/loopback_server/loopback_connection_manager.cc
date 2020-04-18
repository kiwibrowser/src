// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "components/sync/engine_impl/loopback_server/loopback_connection_manager.h"

namespace syncer {

LoopbackConnectionManager::LoopbackConnectionManager(
    CancelationSignal* signal,
    const base::FilePath& persistent_file)
    : ServerConnectionManager("localhost", 0, false, signal),
      loopback_server_(persistent_file) {}

LoopbackConnectionManager::~LoopbackConnectionManager() {}

bool LoopbackConnectionManager::PostBufferToPath(
    PostBufferParams* params,
    const std::string& path,
    const std::string& auth_token) {
  loopback_server_.HandleCommand(
      params->buffer_in, &params->response.server_status,
      &params->response.response_code, &params->buffer_out);
  return params->response.server_status == HttpResponse::SERVER_CONNECTION_OK;
}

}  // namespace syncer
