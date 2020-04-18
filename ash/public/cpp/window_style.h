// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_WINDOW_STYLE_H_
#define ASH_PUBLIC_CPP_WINDOW_STYLE_H_

#include <stdint.h>

#include "ash/public/cpp/ash_public_export.h"

namespace ash {

// Returns true if |type| is a valid WindowStyle.
ASH_PUBLIC_EXPORT bool IsValidWindowStyle(int32_t style);

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_WINDOW_STYLE_H_
