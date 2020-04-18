// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/ipc_args.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace sandbox;

  uint32_t output_size = 0;
  std::unique_ptr<CrossCallParamsEx> params(CrossCallParamsEx::CreateFromBuffer(
      const_cast<uint8_t*>(data), size, &output_size));

  if (!params.get())
    return 0;

  uint32_t tag = params->GetTag();
  IPCParams ipc_params = {0};
  ipc_params.ipc_tag = tag;
  void* args[kMaxIpcParams];
  GetArgs(params.get(), &ipc_params, args);
  ReleaseArgs(&ipc_params, args);
  return 0;
}
