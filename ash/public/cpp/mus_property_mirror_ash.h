// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_MUS_PROPERTY_MIRROR_H_
#define ASH_PUBLIC_CPP_MUS_PROPERTY_MIRROR_H_

#include "ash/public/cpp/ash_public_export.h"
#include "base/macros.h"
#include "ui/views/mus/mus_property_mirror.h"

namespace ash {

// Relays aura content window properties to its root window (the mash frame).
// Ash relies on various window properties for frame titles, shelf items, etc.
// These properties are read from the client's root, not child content windows.
class ASH_PUBLIC_EXPORT MusPropertyMirrorAsh : public views::MusPropertyMirror {
 public:
  MusPropertyMirrorAsh();
  ~MusPropertyMirrorAsh() override;

  // MusPropertyMirror:
  void MirrorPropertyFromWidgetWindowToRootWindow(aura::Window* window,
                                                  aura::Window* root_window,
                                                  const void* key) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MusPropertyMirrorAsh);
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_MUS_PROPERTY_MIRROR_H_
