// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/view_properties.h"

#include "ui/gfx/geometry/insets.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

#if !defined(USE_AURA)
// aura_constants.cc also declared the bool ClassProperty type.
DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(VIEWS_EXPORT, bool);
#endif

DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(VIEWS_EXPORT, gfx::Insets*);

DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(VIEWS_EXPORT,
                                       views::BubbleDialogDelegateView*);

namespace views {

DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::Insets, kMarginsKey, nullptr);
DEFINE_UI_CLASS_PROPERTY_KEY(views::BubbleDialogDelegateView*,
                             kAnchoredDialogKey,
                             nullptr);

}  // namespace views
