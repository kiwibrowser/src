// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/boringssl_util.h"

#include "absl/strings/string_view.h"
#include "platform/api/logging.h"
#include "third_party/boringssl/src/include/openssl/err.h"

namespace openscreen {

namespace {

int BoringSslErrorCallback(const char* str, size_t len, void* context) {
  OSP_LOG_ERROR << "[BoringSSL] " << absl::string_view(str, len);
  return 1;
}

}  // namespace

void LogAndClearBoringSslErrors() {
  ERR_print_errors_cb(BoringSslErrorCallback, nullptr);
  ERR_clear_error();
}

}  // namespace openscreen
