// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_
#define UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_

#include "base/macros.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/message_center_export.h"
#include "ui/views/view.h"

namespace message_center {

// ProportionalImageViews scale and center their images while preserving their
// original proportions.
class MESSAGE_CENTER_EXPORT ProportionalImageView : public views::View {
 public:
  // Internal class name.
  static const char kViewClassName[];

  explicit ProportionalImageView(const gfx::Size& view_size);
  ~ProportionalImageView() override;

  // |image| is scaled to fit within |view_size| and |max_image_size| while
  // maintaining its original aspect ratio. It is then centered within the view.
  void SetImage(const gfx::ImageSkia& image,
                const gfx::Size& max_image_size);

  // Overridden from views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  const char* GetClassName() const override;

 private:
  gfx::Size GetImageDrawingSize();

  gfx::ImageSkia image_;
  gfx::Size max_image_size_;

  DISALLOW_COPY_AND_ASSIGN(ProportionalImageView);
};

}  // namespace message_center

#endif // UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_
