// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_FRAME_HEADER_ORIGIN_TEXT_H_
#define ASH_FRAME_FRAME_HEADER_ORIGIN_TEXT_H_

#include "ash/ash_export.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"

namespace views {
class Label;
}

namespace ash {

// A URL's origin text with a slide in/out animation.
class ASH_EXPORT FrameHeaderOriginText : public views::View {
 public:
  FrameHeaderOriginText(const base::string16& origin,
                        SkColor active_color,
                        SkColor inactive_color);
  ~FrameHeaderOriginText() override;

  // Sets whether to paint the text with the active/inactive color.
  void SetPaintAsActive(bool active);

  // Slides the text in and out.
  void StartSlideAnimation();

  // How long the slide in+out animation takes.
  static base::TimeDelta AnimationDuration();

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Owned by the views hierarchy.
  views::Label* label_ = nullptr;

  const SkColor active_color_;
  const SkColor inactive_color_;

  DISALLOW_COPY_AND_ASSIGN(FrameHeaderOriginText);
};

}  // namespace ash

#endif  // ASH_FRAME_FRAME_HEADER_ORIGIN_TEXT_H_
