// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/zoom_view.h"

#include "base/i18n/number_formatting.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/location_bar/zoom_bubble_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/toolbar/toolbar_model.h"
#include "components/zoom/zoom_controller.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/size.h"

ZoomView::ZoomView(LocationBarView::Delegate* location_bar_delegate,
                   PageActionIconView::Delegate* delegate)
    : PageActionIconView(nullptr, 0, delegate),
      location_bar_delegate_(location_bar_delegate),
      icon_(&kZoomMinusIcon) {
  Update(nullptr);
}

ZoomView::~ZoomView() {
}

void ZoomView::Update(zoom::ZoomController* zoom_controller) {
  if (!zoom_controller ||
      (!ZoomBubbleView::GetZoomBubble() &&
       zoom_controller->IsAtDefaultZoom()) ||
      location_bar_delegate_->GetToolbarModel()->input_in_progress()) {
    SetVisible(false);
    ZoomBubbleView::CloseCurrentBubble();
    return;
  }

  current_zoom_percent_ = zoom_controller->GetZoomPercent();

  // The icon is hidden when the zoom level is default.
  icon_ = zoom_controller->GetZoomRelativeToDefault() ==
                  zoom::ZoomController::ZOOM_BELOW_DEFAULT_ZOOM
              ? &kZoomMinusIcon
              : &kZoomPlusIcon;
  if (GetNativeTheme())
    UpdateIcon();

  SetVisible(true);
}

void ZoomView::OnExecuting(PageActionIconView::ExecuteSource source) {
  ZoomBubbleView::ShowBubble(location_bar_delegate_->GetWebContents(),
                             gfx::Point(), ZoomBubbleView::USER_GESTURE);
}

views::BubbleDialogDelegateView* ZoomView::GetBubble() const {
  return ZoomBubbleView::GetZoomBubble();
}

const gfx::VectorIcon& ZoomView::GetVectorIcon() const {
  return *icon_;
}

base::string16 ZoomView::GetTextForTooltipAndAccessibleName() const {
  return l10n_util::GetStringFUTF16(IDS_TOOLTIP_ZOOM,
                                    base::FormatPercent(current_zoom_percent_));
}
