// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/context_menus_custom_bindings.h"

#include <stdint.h>

#include "base/bind.h"
#include "content/public/renderer/render_thread.h"
#include "extensions/common/extension_messages.h"
#include "v8/include/v8.h"

namespace {

void GetNextContextMenuId(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int context_menu_id = -1;
  content::RenderThread::Get()->Send(
      new ExtensionHostMsg_GenerateUniqueID(&context_menu_id));
  args.GetReturnValue().Set(static_cast<int32_t>(context_menu_id));
}

}  // namespace

namespace extensions {

ContextMenusCustomBindings::ContextMenusCustomBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {}

void ContextMenusCustomBindings::AddRoutes() {
  RouteHandlerFunction("GetNextContextMenuId",
                       base::Bind(&GetNextContextMenuId));
}

}  // extensions
