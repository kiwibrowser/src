// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_ASH_FOCUS_RULES_H_
#define ASH_WM_ASH_FOCUS_RULES_H_

#include "ash/ash_export.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/wm/core/base_focus_rules.h"

namespace ash {
namespace wm {

class ASH_EXPORT AshFocusRules : public ::wm::BaseFocusRules {
 public:
  AshFocusRules();
  ~AshFocusRules() override;

 private:
  // ::wm::BaseFocusRules:
  bool IsToplevelWindow(aura::Window* window) const override;
  bool SupportsChildActivation(aura::Window* window) const override;
  bool IsWindowConsideredVisibleForActivation(
      aura::Window* window) const override;
  bool CanActivateWindow(aura::Window* window) const override;
  bool CanFocusWindow(aura::Window* window,
                      const ui::Event* event) const override;
  aura::Window* GetNextActivatableWindow(aura::Window* ignore) const override;

  aura::Window* GetTopmostWindowToActivateForContainerIndex(
      int index,
      aura::Window* ignore) const;
  aura::Window* GetTopmostWindowToActivateInContainer(
      aura::Window* container,
      aura::Window* ignore) const;

  DISALLOW_COPY_AND_ASSIGN(AshFocusRules);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_ASH_FOCUS_RULES_H_
