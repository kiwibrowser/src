// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/flush_params.h"

namespace gpu {

FlushParams::FlushParams() = default;
FlushParams::FlushParams(const FlushParams& other) = default;
FlushParams::FlushParams(FlushParams&& other) = default;
FlushParams::~FlushParams() = default;
FlushParams& FlushParams::operator=(const FlushParams& other) = default;
FlushParams& FlushParams::operator=(FlushParams&& other) = default;

}  // namespace gpu
