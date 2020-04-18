// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_WIDGET_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_WIDGET_EXAMPLE_H_

#include <string>

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/examples/example_base.h"
#include "ui/views/widget/widget.h"

namespace views {
namespace examples {

// WidgetExample demonstrates how to create a popup widget.
class VIEWS_EXAMPLES_EXPORT WidgetExample : public ExampleBase,
                                            public ButtonListener {
 public:
  WidgetExample();
  ~WidgetExample() override;

  // ExampleBase:
  void CreateExampleView(View* container) override;

 private:
  // Button tags used to identify various commands.
  enum Command {
    POPUP,        // Show a popup widget.
    DIALOG,       // Show a dialog widget.
    MODAL_DIALOG, // Show a modal dialog widget.
    CHILD,        // Show a child widget.
    CLOSE_WIDGET, // Close the sender button's widget.
  };

  // Construct a button with the specified |label| and |tag| in |container|.
  void BuildButton(View* container, const std::string& label, int tag);

  // Construct a Widget for |sender|, initialize with |params|, and call Show().
  void ShowWidget(View* sender, Widget::InitParams params);

  // ButtonListener:
  void ButtonPressed(Button* sender, const ui::Event& event) override;

  DISALLOW_COPY_AND_ASSIGN(WidgetExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_WIDGET_EXAMPLE_H_
