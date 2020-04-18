// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/app_list/app_list_struct_traits.h"

#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "ui/gfx/image/mojo/image_skia_struct_traits.h"
#include "ui/gfx/range/mojo/range_struct_traits.h"

namespace mojo {

////////////////////////////////////////////////////////////////////////////////
// SearchResultTag:

// static
bool StructTraits<ash::mojom::SearchResultTagDataView, ash::SearchResultTag>::
    Read(ash::mojom::SearchResultTagDataView data, ash::SearchResultTag* out) {
  if (!data.ReadRange(&out->range))
    return false;
  out->styles = data.styles();
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// SearchResultActionLabel:

// static
ash::mojom::SearchResultActionLabelDataView::Tag UnionTraits<
    ash::mojom::SearchResultActionLabelDataView,
    ash::SearchResultAction>::GetTag(const ash::SearchResultAction& action) {
  if (action.label_text.empty())
    return ash::mojom::SearchResultActionLabelDataView::Tag::IMAGE_LABEL;
  else
    return ash::mojom::SearchResultActionLabelDataView::Tag::TEXT_LABEL;
}

// static
bool UnionTraits<ash::mojom::SearchResultActionLabelDataView,
                 ash::SearchResultAction>::
    Read(ash::mojom::SearchResultActionLabelDataView data,
         ash::SearchResultAction* out) {
  switch (data.tag()) {
    case ash::mojom::SearchResultActionLabelDataView::Tag::IMAGE_LABEL: {
      ash::mojom::SearchResultActionImageLabelDataView image_label_data_view;
      data.GetImageLabelDataView(&image_label_data_view);
      if (!image_label_data_view.ReadBaseImage(&out->base_image))
        return false;
      if (!image_label_data_view.ReadHoverImage(&out->hover_image))
        return false;
      if (!image_label_data_view.ReadPressedImage(&out->pressed_image))
        return false;
      return true;
    }
    case ash::mojom::SearchResultActionLabelDataView::Tag::TEXT_LABEL: {
      if (!data.ReadTextLabel(&out->label_text))
        return false;
      return true;
    }
  }
  NOTREACHED();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// SearchResultAction:

// static
bool StructTraits<
    ash::mojom::SearchResultActionDataView,
    ash::SearchResultAction>::Read(ash::mojom::SearchResultActionDataView data,
                                   ash::SearchResultAction* out) {
  if (!data.ReadTooltipText(&out->tooltip_text))
    return false;
  if (!data.ReadLabel(out))
    return false;
  return true;
}

}  // namespace mojo
