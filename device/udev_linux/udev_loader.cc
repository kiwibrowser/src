// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/udev_linux/udev_loader.h"

#include <memory>

#include "base/logging.h"
#include "device/udev_linux/udev0_loader.h"
#include "device/udev_linux/udev1_loader.h"

namespace device {

namespace {

UdevLoader* g_udev_loader = NULL;

}  // namespace

// static
UdevLoader* UdevLoader::Get() {
  if (g_udev_loader)
    return g_udev_loader;

  std::unique_ptr<UdevLoader> udev_loader;
  udev_loader.reset(new Udev1Loader);
  if (udev_loader->Init()) {
    g_udev_loader = udev_loader.release();
    return g_udev_loader;
  }

  udev_loader.reset(new Udev0Loader);
  if (udev_loader->Init()) {
    g_udev_loader = udev_loader.release();
    return g_udev_loader;
  }
  CHECK(false);
  return NULL;
}

UdevLoader::~UdevLoader() = default;

}  // namespace device
