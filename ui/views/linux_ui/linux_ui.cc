// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/linux_ui/linux_ui.h"

#include "ui/base/ime/linux/linux_input_method_context_factory.h"
#include "ui/gfx/linux_font_delegate.h"
#include "ui/shell_dialogs/shell_dialog_linux.h"

namespace {

views::LinuxUI* g_linux_ui = NULL;

}  // namespace

namespace views {

void LinuxUI::SetInstance(LinuxUI* instance) {
  delete g_linux_ui;
  g_linux_ui = instance;
  LinuxInputMethodContextFactory::SetInstance(instance);
  LinuxFontDelegate::SetInstance(instance);
  ShellDialogLinux::SetInstance(instance);
  ui::SetTextEditKeyBindingsDelegate(instance);
}

LinuxUI* LinuxUI::instance() {
  return g_linux_ui;
}

}  // namespace views
