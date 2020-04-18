// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_LABEL_TRAY_VIEW_H_
#define ASH_SYSTEM_TRAY_LABEL_TRAY_VIEW_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/views/view.h"

namespace gfx {
struct VectorIcon;
}  // namespace gfx

namespace ash {
class ViewClickListener;

// A view to display an icon and message text in the system menu which
// automatically hides when the message text is empty. Multi-line message text
// is supported.
class LabelTrayView : public views::View {
 public:
  // If |click_listener| is null then no action is taken on click.
  LabelTrayView(ViewClickListener* click_listener, const gfx::VectorIcon& icon);
  ~LabelTrayView() override;

  const base::string16& message() { return message_; }

  void SetMessage(const base::string16& message);

 private:
  views::View* CreateChildView(const base::string16& message) const;

  ViewClickListener* click_listener_;
  const gfx::VectorIcon& icon_;
  base::string16 message_;

  DISALLOW_COPY_AND_ASSIGN(LabelTrayView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_LABEL_TRAY_VIEW_H_
