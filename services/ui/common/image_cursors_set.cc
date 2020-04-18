// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/common/image_cursors_set.h"

#include <algorithm>

#include "ui/base/cursor/image_cursors.h"

namespace ui {

ImageCursorsSet::ImageCursorsSet() : weak_ptr_factory_(this) {}

ImageCursorsSet::~ImageCursorsSet() {}

void ImageCursorsSet::AddImageCursors(
    std::unique_ptr<ImageCursors> image_cursors) {
  auto result = image_cursors_set_.insert(std::move(image_cursors));
  DCHECK(result.second);
}

void ImageCursorsSet::DeleteImageCursors(ImageCursors* image_cursors) {
  auto it =
      std::find_if(image_cursors_set_.begin(), image_cursors_set_.end(),
                   [image_cursors](const std::unique_ptr<ImageCursors>& elmt) {
                     return elmt.get() == image_cursors;
                   });
  DCHECK(it != image_cursors_set_.end());
  image_cursors_set_.erase(it);
}

base::WeakPtr<ImageCursorsSet> ImageCursorsSet::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ui
