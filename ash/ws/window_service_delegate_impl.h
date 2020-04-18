// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WS_WINDOW_SERVICE_DELEGATE_IMPL_H_
#define ASH_WS_WINDOW_SERVICE_DELEGATE_IMPL_H_

#include <memory>

#include "services/ui/ws2/window_service_delegate.h"

namespace ash {

class WindowServiceDelegateImpl : public ui::ws2::WindowServiceDelegate {
 public:
  WindowServiceDelegateImpl();
  ~WindowServiceDelegateImpl() override;

  // ui::ws2::WindowServiceDelegate:
  std::unique_ptr<aura::Window> NewTopLevel(
      aura::PropertyConverter* property_converter,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties)
      override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowServiceDelegateImpl);
};

}  // namespace ash

#endif  // ASH_WS_WINDOW_SERVICE_DELEGATE_IMPL_H_
