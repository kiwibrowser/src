// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_UTIL_AURALINUX_H_
#define UI_ACCESSIBILITY_AX_UTIL_AURALINUX_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "ui/accessibility/ax_export.h"

namespace ui {

// This singleton class initializes ATK (accessibility toolkit) and
// registers an implementation of the AtkUtil class, a global class that
// every accessible application needs to register once.
class AX_EXPORT AtkUtilAuraLinux {
 public:
  // Get the single instance of this class.
  static AtkUtilAuraLinux* GetInstance();

  AtkUtilAuraLinux() = default;

  void InitializeAsync();
  void InitializeForTesting();

 private:
  friend struct base::DefaultSingletonTraits<AtkUtilAuraLinux>;

  bool ShouldEnableAccessibility();

  bool PlatformShouldEnableAccessibility();
  void PlatformInitializeAsync();

  DISALLOW_COPY_AND_ASSIGN(AtkUtilAuraLinux);
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_UTIL_AURALINUX_H_
