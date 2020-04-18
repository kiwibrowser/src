// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_MUS_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_MUS_H_

#include "ash/public/cpp/immersive/immersive_context.h"
#include "base/macros.h"

class ImmersiveContextMus : public ash::ImmersiveContext {
 public:
  ImmersiveContextMus();
  ~ImmersiveContextMus() override;

  // ash::ImmersiveContext:
  void InstallResizeHandleWindowTargeter(
      ash::ImmersiveFullscreenController* controller) override;
  void OnEnteringOrExitingImmersive(
      ash::ImmersiveFullscreenController* controller,
      bool entering) override;
  gfx::Rect GetDisplayBoundsInScreen(views::Widget* widget) override;
  void AddPointerWatcher(views::PointerWatcher* watcher,
                         views::PointerWatcherEventTypes events) override;
  void RemovePointerWatcher(views::PointerWatcher* watcher) override;
  bool DoesAnyWindowHaveCapture() override;
  bool IsMouseEventsEnabled() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImmersiveContextMus);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_MUS_H_
