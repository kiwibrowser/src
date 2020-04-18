// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/window_style.h"

#include "ash/public/interfaces/window_style.mojom.h"

namespace ash {

bool IsValidWindowStyle(int32_t type) {
  return type == static_cast<int32_t>(mojom::WindowStyle::DEFAULT) ||
         type == static_cast<int32_t>(mojom::WindowStyle::BROWSER);
}

}  // namespace ash
