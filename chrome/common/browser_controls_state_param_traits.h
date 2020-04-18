// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/browser_controls_state.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/blink/public/web/web_console_message.h"

IPC_ENUM_TRAITS_MAX_VALUE(blink::WebConsoleMessage::Level,
                          blink::WebConsoleMessage::kLevelLast)
IPC_ENUM_TRAITS_MAX_VALUE(content::BrowserControlsState,
                          content::BROWSER_CONTROLS_STATE_LAST)
