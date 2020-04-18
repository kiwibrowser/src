// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_ASH_DISPLAY_CONTROLLER_H_
#define ASH_DISPLAY_ASH_DISPLAY_CONTROLLER_H_

#include "ash/public/interfaces/ash_display_controller.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

class AshDisplayController : public mojom::AshDisplayController {
 public:
  AshDisplayController();
  ~AshDisplayController() override;

  void BindRequest(mojom::AshDisplayControllerRequest request);

  // mojom::AshDisplayController:
  void TakeDisplayControl(TakeDisplayControlCallback callback) override;
  void RelinquishDisplayControl(
      RelinquishDisplayControlCallback callback) override;

 private:
  mojo::BindingSet<mojom::AshDisplayController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(AshDisplayController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_ASH_DISPLAY_CONTROLLER_H_
