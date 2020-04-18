// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_ROOT_ELEMENT_H_
#define COMPONENTS_UI_DEVTOOLS_ROOT_ELEMENT_H_

#include "base/macros.h"
#include "components/ui_devtools/ui_element.h"

namespace ui_devtools {

class UI_DEVTOOLS_EXPORT RootElement : public UIElement {
 public:
  explicit RootElement(UIElementDelegate* ui_element_delegate);
  ~RootElement() override;

  // UIElement:
  std::vector<std::pair<std::string, std::string>> GetCustomProperties()
      const override;
  void GetBounds(gfx::Rect* bounds) const override;
  void SetBounds(const gfx::Rect& bounds) override;
  void GetVisible(bool* visible) const override;
  void SetVisible(bool visible) override;
  std::unique_ptr<protocol::Array<std::string>> GetAttributes() const override;
  std::pair<gfx::NativeWindow, gfx::Rect> GetNodeWindowAndBounds()
      const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RootElement);
};
}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_ROOT_ELEMENT_H_
