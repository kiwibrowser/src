// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_TABLET_MODE_BACKDROP_DELEGATE_IMPL_H_
#define ASH_WM_TABLET_MODE_TABLET_MODE_BACKDROP_DELEGATE_IMPL_H_

#include "ash/wm/workspace/backdrop_delegate.h"

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

// A backdrop delegate for MaximizedMode, which always creates a backdrop.
// This is also used in the WorkspaceLayoutManagerBackdropTest, hence
// is public.
class ASH_EXPORT TabletModeBackdropDelegateImpl : public BackdropDelegate {
 public:
  TabletModeBackdropDelegateImpl();
  ~TabletModeBackdropDelegateImpl() override;

 protected:
  bool HasBackdrop(aura::Window* window) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabletModeBackdropDelegateImpl);
};

}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_TABLET_MODE_BACKDROP_DELEGATE_IMPL_H_
