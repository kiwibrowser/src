// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_COMMON_IMAGE_CURSORS_SET_H_
#define SERVICES_UI_COMMON_IMAGE_CURSORS_SET_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace ui {

class ImageCursors;

// Helper class wrapping a set of ImageCursors objects.
class ImageCursorsSet {
 public:
  ImageCursorsSet();
  ~ImageCursorsSet();

  void AddImageCursors(std::unique_ptr<ImageCursors> image_cursors);
  void DeleteImageCursors(ImageCursors* image_cursors);
  base::WeakPtr<ImageCursorsSet> GetWeakPtr();

 private:
  std::set<std::unique_ptr<ImageCursors>> image_cursors_set_;
  base::WeakPtrFactory<ImageCursorsSet> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImageCursorsSet);
};

}  // namespace ui

#endif  // SERVICES_UI_COMMON_IMAGE_CURSORS_SET_H_
