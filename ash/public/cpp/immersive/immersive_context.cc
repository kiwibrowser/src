// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/immersive/immersive_context.h"

#include "base/logging.h"

namespace ash {

// static
ImmersiveContext* ImmersiveContext::instance_ = nullptr;

ImmersiveContext::ImmersiveContext() {
  DCHECK(!instance_);
  instance_ = this;
}

ImmersiveContext::~ImmersiveContext() {
  DCHECK_EQ(instance_, this);
  instance_ = nullptr;
}

}  // namespace ash
