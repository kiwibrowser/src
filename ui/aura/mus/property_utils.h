// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_PROPERTY_UTILS_H_
#define UI_AURA_MUS_PROPERTY_UTILS_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "ui/aura/aura_export.h"

namespace ui {
namespace mojom {
enum class WindowType;
}
}

namespace aura {

class Window;

// Configures the two window type properties on |window|. Specifically this
// sets the property client::kWindowTypeKey as well as calling SetType().
// This *must* be called before Init(). No-op for WindowType::UNKNOWN.
AURA_EXPORT void SetWindowType(Window* window,
                               ui::mojom::WindowType window_type);

// Returns the window type specified in |properties|, or WindowType::UNKNOWN.
AURA_EXPORT ui::mojom::WindowType GetWindowTypeFromProperties(
    const std::map<std::string, std::vector<uint8_t>>& properties);

}  // namespace aura

#endif  // UI_AURA_MUS_PROPERTY_UTILS_H_
