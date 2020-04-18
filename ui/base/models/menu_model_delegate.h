// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_MODELS_MENU_MODEL_DELEGATE_H_
#define UI_BASE_MODELS_MENU_MODEL_DELEGATE_H_

namespace ui {

class MenuModelDelegate {
 public:
  // Invoked when an icon has been loaded from history.
  virtual void OnIconChanged(int index) = 0;

  // Invoked after items in |MenuModel| have been removed and/or added,
  // delegate should assume the entire contents of the model has changed.
  virtual void OnMenuStructureChanged() {}

 protected:
  virtual ~MenuModelDelegate() {}
};

}  // namespace ui

#endif  // UI_BASE_MODELS_MENU_MODEL_DELEGATE_H_
