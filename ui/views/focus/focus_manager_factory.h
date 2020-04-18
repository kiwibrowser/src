// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_FOCUS_FOCUS_MANAGER_FACTORY_H_
#define UI_VIEWS_FOCUS_FOCUS_MANAGER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "ui/views/views_export.h"

namespace views {

class FocusManager;
class Widget;

// A factory to create FocusManager. This is used in unit tests
// to inject a custom factory.
class VIEWS_EXPORT FocusManagerFactory {
 public:
  // Create a FocusManager for the given |widget| using the installed Factory.
  static std::unique_ptr<FocusManager> Create(Widget* widget,
                                              bool desktop_widget);

  // Installs FocusManagerFactory. If |factory| is NULL, it resets
  // to the default factory which creates plain FocusManager.
  static void Install(FocusManagerFactory* factory);

 protected:
  FocusManagerFactory();
  virtual ~FocusManagerFactory();

  // Create a FocusManager for the given |widget|.
  // The |desktop_widget| bool is true for widgets created in the desktop and
  // false for widgets created in the shell.
  virtual std::unique_ptr<FocusManager> CreateFocusManager(
      Widget* widget,
      bool desktop_widget) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusManagerFactory);
};

}  // namespace views

#endif  // UI_VIEWS_FOCUS_FOCUS_MANAGER_FACTORY_H_
