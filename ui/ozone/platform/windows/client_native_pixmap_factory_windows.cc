// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/windows/client_native_pixmap_factory_windows.h"

#include "ui/ozone/common/stub_client_native_pixmap_factory.h"

namespace ui {

gfx::ClientNativePixmapFactory* CreateClientNativePixmapFactoryWindows() {
  // TODO(camurcu): Implement the better (more performant) way.
  return CreateStubClientNativePixmapFactory();
}

}  // namespace ui
