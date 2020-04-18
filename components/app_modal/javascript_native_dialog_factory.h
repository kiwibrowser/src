// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_APP_MODAL_JAVASCRIPT_NATIVE_DIALOG_FACTORY_H_
#define COMPONENTS_APP_MODAL_JAVASCRIPT_NATIVE_DIALOG_FACTORY_H_

#include "ui/gfx/native_widget_types.h"

namespace app_modal {

class JavaScriptAppModalDialog;
class NativeAppModalDialog;

class JavaScriptNativeDialogFactory {
 public:
  virtual ~JavaScriptNativeDialogFactory() {}

  // Creates an native modal dialog for a JavaScript dialog;
  virtual NativeAppModalDialog* CreateNativeJavaScriptDialog(
      JavaScriptAppModalDialog* dialog) = 0;
};

}  // namespace app_modal

#endif  // COMPONENTS_APP_MODAL_JAVASCRIPT_NATIVE_DIALOG_FACTORY_H_
