// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_FAKE_TABLET_MODE_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_FAKE_TABLET_MODE_CONTROLLER_H_

#include "ash/public/interfaces/tablet_mode.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"

// Simulates the TabletModeController in ash.
class FakeTabletModeController : ash::mojom::TabletModeController {
 public:
  FakeTabletModeController();

  ~FakeTabletModeController() override;

  bool was_client_set() const { return was_client_set_; }

  // Returns a mojo interface pointer bound to this object.
  ash::mojom::TabletModeControllerPtr CreateInterfacePtr();

  // ash::mojom::TabletModeController:
  void SetClient(ash::mojom::TabletModeClientPtr client) override;

 private:
  mojo::Binding<ash::mojom::TabletModeController> binding_;

  bool was_client_set_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeTabletModeController);
};

#endif  // CHROME_BROWSER_UI_ASH_FAKE_TABLET_MODE_CONTROLLER_H_
