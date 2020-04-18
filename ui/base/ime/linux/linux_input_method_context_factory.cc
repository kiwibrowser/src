// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/linux/linux_input_method_context_factory.h"

namespace {

const ui::LinuxInputMethodContextFactory* g_linux_input_method_context_factory =
    NULL;

}  // namespace

namespace ui {

// static
const LinuxInputMethodContextFactory*
LinuxInputMethodContextFactory::instance() {
  return g_linux_input_method_context_factory;
}

// static
void LinuxInputMethodContextFactory::SetInstance(
    const LinuxInputMethodContextFactory* instance) {
  g_linux_input_method_context_factory = instance;
}

}  // namespace ui
