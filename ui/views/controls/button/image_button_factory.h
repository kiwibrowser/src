// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UI_VIEWS_CONTROLS_BUTTON_IMAGE_BUTTON_FACTORY_H_
#define UI_VIEWS_CONTROLS_BUTTON_IMAGE_BUTTON_FACTORY_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/views_export.h"

namespace gfx {
struct VectorIcon;
}

namespace views {

class ButtonListener;
class ImageButton;

// Creates an ImageButton with an ink drop and a centered image in preparation
// for applying a vector icon with SetImageFromVectorIcon below.
VIEWS_EXPORT ImageButton* CreateVectorImageButton(ButtonListener* listener);

// Sets images on |button| for STATE_NORMAL and STATE_DISABLED from the given
// vector icon and color. |related_text_color| is normally the main text color
// used in the parent view, and the actual color used is derived from that. Call
// again to update the button if |related_text_color| is changing.
VIEWS_EXPORT void SetImageFromVectorIcon(
    ImageButton* button,
    const gfx::VectorIcon& icon,
    SkColor related_text_color = gfx::kGoogleGrey900);

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_BUTTON_IMAGE_BUTTON_FACTORY_H_
