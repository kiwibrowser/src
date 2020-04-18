// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See header file for description of RendererPreconnect class

#include "components/network_hints/renderer/renderer_preconnect.h"

#include "components/network_hints/common/network_hints_common.h"
#include "components/network_hints/common/network_hints_messages.h"
#include "content/public/renderer/render_thread.h"

using content::RenderThread;

namespace network_hints {

RendererPreconnect::RendererPreconnect() {
}

RendererPreconnect::~RendererPreconnect() {
}

void RendererPreconnect::Preconnect(const GURL& url, bool allow_credentials) {
  if (!url.is_valid())
    return;

  RenderThread::Get()->Send(
      new NetworkHintsMsg_Preconnect(url, allow_credentials, 1));
}

}  // namespace network_hints
