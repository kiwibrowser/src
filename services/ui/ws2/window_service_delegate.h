// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_DELEGATE_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_DELEGATE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_map.h"

namespace aura {
class PropertyConverter;
class Window;
}

namespace ui {
namespace ws2 {

// A delegate used by the WindowService for context-specific operations.
class COMPONENT_EXPORT(WINDOW_SERVICE) WindowServiceDelegate {
 public:
  // A client requested a new top-level window. Implementations should create a
  // new window, parenting it in the appropriate container. Return null to
  // reject the request.
  virtual std::unique_ptr<aura::Window> NewTopLevel(
      aura::PropertyConverter* property_converter,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties) = 0;

 protected:
  virtual ~WindowServiceDelegate() = default;
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_DELEGATE_H_
