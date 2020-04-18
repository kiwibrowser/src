// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_BOUNDED_LABEL_H_
#define UI_MESSAGE_CENTER_BOUNDED_LABEL_H_

#include <list>
#include <map>
#include <memory>

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/message_center/message_center_export.h"
#include "ui/views/view.h"

namespace gfx {
class FontList;
}

namespace message_center {

class InnerBoundedLabel;

namespace test {
class BoundedLabelTest;
}

// BoundedLabels display left aligned text up to a maximum number of lines, with
// ellipsis at the end of the last line for any omitted text. BoundedLabel is a
// direct subclass of views::Views rather than a subclass of views::Label
// to avoid exposing some of views::Label's methods that can't be made to work
// with BoundedLabel. See the description of InnerBoundedLabel in the
// bounded_label.cc file for details.
class MESSAGE_CENTER_EXPORT BoundedLabel : public views::View {
 public:
  BoundedLabel(const base::string16& text, const gfx::FontList& font_list);
  BoundedLabel(const base::string16& text);
  ~BoundedLabel() override;

  void SetColor(SkColor text_color);
  void SetLineHeight(int height);  // Pass in 0 for default height.
  void SetLineLimit(int lines);  // Pass in -1 for no limit.
  void SetText(const base::string16& text);  // Additionally clears caches.
  void SizeToFit(int fixed_width);  // Same interface as Label::SizeToFit.

  int GetLineHeight() const;
  int GetLineLimit() const;

  // Pass in a -1 width to use the preferred width, a -1 limit to skip limits.
  int GetLinesForWidthAndLimit(int width, int limit);
  gfx::Size GetSizeForWidthAndLines(int width, int lines);

  // views::View:
  int GetBaseline() const override;
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool CanProcessEventsWithinSubtree() const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  views::View* GetTooltipHandlerForPoint(const gfx::Point& point) override;
  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override;

 protected:
  // Overridden from views::View.
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;

 private:
  friend class test::BoundedLabelTest;

  base::string16 GetWrappedTextForTest(int width, int lines);

  std::unique_ptr<InnerBoundedLabel> label_;
  int line_limit_;
  int fixed_width_;

  DISALLOW_COPY_AND_ASSIGN(BoundedLabel);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_BOUNDED_LABEL_H_
