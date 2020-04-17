// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/button_drag_utils.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/canvas_painter.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/image/image.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/drag_utils.h"
#include "ui/views/paint_info.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace button_drag_utils {

// Maximum width of the link drag image in pixels.
void SetURLAndDragImage(const GURL& url,
                        const base::string16& title,
                        const gfx::ImageSkia& icon,
                        const gfx::Point* press_pt,
                        const views::Widget& widget,
                        ui::OSExchangeData* data) {
  DCHECK(url.is_valid());
  DCHECK(data);
  data->SetURL(url, title);
  SetDragImage(url, title, icon, press_pt, widget, data);
}

void SetDragImage(const GURL& url,
                  const base::string16& title,
                  const gfx::ImageSkia& icon,
                  const gfx::Point* press_pt,
                  const views::Widget& widget,
                  ui::OSExchangeData* data) {
}

}  // namespace button_drag_utils
