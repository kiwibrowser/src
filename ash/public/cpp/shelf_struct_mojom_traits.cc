// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_struct_mojom_traits.h"

#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "ui/gfx/image/mojo/image_skia_struct_traits.h"

namespace mojo {

using ShelfItemStructTraits =
    StructTraits<ash::mojom::ShelfItemDataView, ash::ShelfItem>;

// static
bool StructTraits<ash::mojom::ShelfIDDataView, ash::ShelfID>::Read(
    ash::mojom::ShelfIDDataView data,
    ash::ShelfID* out) {
  if (!data.ReadAppId(&out->app_id) || !data.ReadLaunchId(&out->launch_id))
    return false;
  // A non-empty launch id requires a non-empty app id.
  return out->launch_id.empty() || !out->app_id.empty();
}

// static
bool ShelfItemStructTraits::Read(ash::mojom::ShelfItemDataView data,
                                 ash::ShelfItem* out) {
  if (!data.ReadType(&out->type) || !data.ReadImage(&out->image) ||
      !data.ReadStatus(&out->status) || !data.ReadShelfId(&out->id) ||
      !data.ReadTitle(&out->title)) {
    return false;
  }
  out->shows_tooltip = data.shows_tooltip();
  out->pinned_by_policy = data.pinned_by_policy();
  return true;
}

}  // namespace mojo
