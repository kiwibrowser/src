// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/media_router_internals_webui_message_handler.h"

#include "base/values.h"
#include "chrome/browser/media/router/media_router.h"

namespace media_router {

MediaRouterInternalsWebUIMessageHandler::
    MediaRouterInternalsWebUIMessageHandler(const MediaRouter* router)
    : router_(router) {
  DCHECK(router_);
}

void MediaRouterInternalsWebUIMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "initialized",
      base::BindRepeating(
          &MediaRouterInternalsWebUIMessageHandler::HandleInitialized,
          base::Unretained(this)));
}

void MediaRouterInternalsWebUIMessageHandler::HandleInitialized(
    const base::ListValue* args) {
  AllowJavascript();
  CallJavascriptFunction("media_router_internals.setStatus",
                         router_->GetState());
}

}  // namespace media_router
