// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_PROCESS_ERROR_CALLBACK_H_
#define MOJO_EDK_EMBEDDER_PROCESS_ERROR_CALLBACK_H_

#include <string>

#include "base/callback.h"

namespace mojo {
namespace edk {

using ProcessErrorCallback =
    base::RepeatingCallback<void(const std::string& error)>;

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_PROCESS_ERROR_CALLBACK_H_
