// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/input_stream.h"

namespace download {

InputStream::~InputStream() = default;

void InputStream::Initialize() {}

void InputStream::RegisterDataReadyCallback(
    const mojo::SimpleWatcher::ReadyCallback& callback) {}

void InputStream::ClearDataReadyCallback() {}

void InputStream::RegisterCompletionCallback(base::OnceClosure callback) {}

}  // namespace download
