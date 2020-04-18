// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_CLIENT_NATIVE_PIXMAP_FACTORY_OZONE_H_
#define UI_OZONE_PUBLIC_CLIENT_NATIVE_PIXMAP_FACTORY_OZONE_H_

#include "ui/gfx/client_native_pixmap_factory.h"
#include "ui/ozone/ozone_export.h"

namespace ui {

OZONE_EXPORT std::unique_ptr<gfx::ClientNativePixmapFactory>
CreateClientNativePixmapFactoryOzone();

}  // namespace ui

#endif  // UI_OZONE_PUBLIC_CLIENT_NATIVE_PIXMAP_FACTORY_OZONE_H_
