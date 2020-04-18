// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_IMMERSIVE_HANDLER_FACTORY_MASH_H_
#define ASH_WM_IMMERSIVE_HANDLER_FACTORY_MASH_H_

#include "ash/public/cpp/immersive/immersive_handler_factory.h"
#include "base/macros.h"

namespace ash {

class ImmersiveHandlerFactoryMash : public ImmersiveHandlerFactory {
 public:
  ImmersiveHandlerFactoryMash();
  ~ImmersiveHandlerFactoryMash() override;

  // ImmersiveHandlerFactory:
  std::unique_ptr<ImmersiveFocusWatcher> CreateFocusWatcher(
      ImmersiveFullscreenController* controller) override;
  std::unique_ptr<ImmersiveGestureHandler> CreateGestureHandler(
      ImmersiveFullscreenController* controller) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImmersiveHandlerFactoryMash);
};

}  // namespace ash

#endif  // ASH_WM_IMMERSIVE_HANDLER_FACTORY_MASH_H_
