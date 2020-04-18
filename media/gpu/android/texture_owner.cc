// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/texture_owner.h"

#include "base/threading/thread_task_runner_handle.h"

namespace media {

TextureOwner::TextureOwner()
    : base::RefCountedDeleteOnSequence<TextureOwner>(
          base::ThreadTaskRunnerHandle::Get()),
      task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

TextureOwner::~TextureOwner() = default;

}  // namespace media
