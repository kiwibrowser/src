// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_IME_IME_DRIVER_BRIDGE_H_
#define SERVICES_UI_IME_IME_DRIVER_BRIDGE_H_

#include <utility>

#include "base/containers/queue.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/public/interfaces/ime/ime.mojom.h"

namespace service_manager {
class Connector;
}

namespace ui {

class IMEDriverBridge : public mojom::IMEDriver {
 public:
  IMEDriverBridge();
  ~IMEDriverBridge() override;

  void Init(service_manager::Connector* connector, bool is_test_config);
  void AddBinding(mojom::IMEDriverRequest request);
  void SetDriver(mojom::IMEDriverPtr driver);

 private:
  // mojom::IMEDriver:
  void StartSession(mojom::StartSessionDetailsPtr details) override;

  mojo::BindingSet<mojom::IMEDriver> bindings_;
  mojom::IMEDriverPtr driver_;

  base::queue<mojom::StartSessionDetailsPtr> pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(IMEDriverBridge);
};

}  // namespace ui

#endif  // SERVICES_UI_IME_IME_DRIVER_BRIDGE_H_
