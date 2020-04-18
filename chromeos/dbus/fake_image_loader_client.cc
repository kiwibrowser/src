// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_image_loader_client.h"

#include <utility>

#include "base/optional.h"

namespace chromeos {

void FakeImageLoaderClient::RegisterComponent(
    const std::string& name,
    const std::string& version,
    const std::string& component_folder_abs_path,
    DBusMethodCallback<bool> callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeImageLoaderClient::LoadComponent(
    const std::string& name,
    DBusMethodCallback<std::string> callback) {
  std::move(callback).Run(base::nullopt);
}
void FakeImageLoaderClient::LoadComponentAtPath(
    const std::string& name,
    const base::FilePath& path,
    DBusMethodCallback<base::FilePath> callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeImageLoaderClient::RemoveComponent(const std::string& name,
                                            DBusMethodCallback<bool> callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeImageLoaderClient::RequestComponentVersion(
    const std::string& name,
    DBusMethodCallback<std::string> callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeImageLoaderClient::UnmountComponent(
    const std::string& name,
    DBusMethodCallback<bool> callback) {
  std::move(callback).Run(base::nullopt);
}

}  // namespace chromeos
