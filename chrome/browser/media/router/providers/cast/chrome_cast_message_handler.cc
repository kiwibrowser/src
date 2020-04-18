// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/chrome_cast_message_handler.h"

#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_content_client.h"
#include "components/cast_channel/cast_message_handler.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/version_info/version_info.h"

namespace media_router {

cast_channel::CastMessageHandler* GetCastMessageHandler() {
  static cast_channel::CastMessageHandler* instance =
      new cast_channel::CastMessageHandler(
          cast_channel::CastSocketService::GetInstance(), GetUserAgent(),
          version_info::GetVersionNumber(),
          g_browser_process->GetApplicationLocale());
  return instance;
}

}  // namespace media_router
