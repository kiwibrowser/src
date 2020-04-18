// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/immersive/immersive_handler_factory.h"

#include "base/logging.h"

namespace ash {

// static
ImmersiveHandlerFactory* ImmersiveHandlerFactory::instance_ = nullptr;

ImmersiveHandlerFactory::ImmersiveHandlerFactory() {
  DCHECK(!instance_);
  instance_ = this;
}

ImmersiveHandlerFactory::~ImmersiveHandlerFactory() {
  DCHECK_EQ(instance_, this);
  instance_ = nullptr;
}

}  // namespace ash
