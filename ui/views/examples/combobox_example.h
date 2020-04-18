// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_COMBOBOX_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_COMBOBOX_EXAMPLE_H_

#include "base/macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/examples/example_base.h"

namespace views {
namespace examples {

// A combobox model implementation that generates a list of "Item <index>".
class VIEWS_EXAMPLES_EXPORT ComboboxModelExample : public ui::ComboboxModel {
 public:
  ComboboxModelExample();
  ~ComboboxModelExample() override;

  // ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ComboboxModelExample);
};

class VIEWS_EXAMPLES_EXPORT ComboboxExample : public ExampleBase,
                                              public ComboboxListener {
 public:
  ComboboxExample();
  ~ComboboxExample() override;

  // ExampleBase:
  void CreateExampleView(View* container) override;

 private:
  // ComboboxListener:
  void OnPerformAction(Combobox* combobox) override;

  ComboboxModelExample combobox_model_;
  Combobox* combobox_ = nullptr;
  Combobox* disabled_combobox_ = nullptr;
  Combobox* action_combobox_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ComboboxExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_COMBOBOX_EXAMPLE_H_
