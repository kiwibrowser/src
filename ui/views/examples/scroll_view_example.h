// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_SCROLL_VIEW_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_SCROLL_VIEW_EXAMPLE_H_

#include <string>

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/examples/example_base.h"

namespace views {

class LabelButton;

namespace examples {

class VIEWS_EXAMPLES_EXPORT ScrollViewExample : public ExampleBase,
                                                public ButtonListener {
 public:
  ScrollViewExample();
  ~ScrollViewExample() override;

  // ExampleBase:
  void CreateExampleView(View* container) override;

 private:
  // ButtonListener:
  void ButtonPressed(Button* sender, const ui::Event& event) override;

  // Control buttons to change the size of scrollable and jump to
  // predefined position.
  LabelButton* wide_;
  LabelButton* tall_;
  LabelButton* big_square_;
  LabelButton* small_square_;
  LabelButton* scroll_to_;

  class ScrollableView;
  // The content of the scroll view.
  ScrollableView* scrollable_;

  // The scroll view to test.
  ScrollView* scroll_view_;

  DISALLOW_COPY_AND_ASSIGN(ScrollViewExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_SCROLL_VIEW_EXAMPLE_H_
