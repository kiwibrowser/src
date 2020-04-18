// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_CHILD_DWRITE_FONT_PROXY_INIT_WIN_H_
#define CONTENT_PUBLIC_CHILD_DWRITE_FONT_PROXY_INIT_WIN_H_

#include "content/common/content_export.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

// Initializes the dwrite font proxy. If |connector| is non-null, it will be
// used to create the underlying mojo connection to the browser during
// initialization. Otherwise, the connection will be created on demand using a
// connector obtained from the current ChildThread.
CONTENT_EXPORT void InitializeDWriteFontProxy(
    service_manager::Connector* connector = nullptr);

// Uninitialize the dwrite font proxy. This is safe to call even if the proxy
// has not been initialized. After this, calls to load fonts may fail.
CONTENT_EXPORT void UninitializeDWriteFontProxy();

}  // namespace content

#endif  // CONTENT_PUBLIC_CHILD_DWRITE_FONT_PROXY_INIT_WIN_H_
