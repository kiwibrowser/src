// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_IMAGE_LOADER_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_IMAGE_LOADER_CLIENT_H_

#include <string>

#include "base/macros.h"
#include "chromeos/dbus/image_loader_client.h"

namespace chromeos {

// A fake implementation of ImageLoaderClient. This class does nothing.
class CHROMEOS_EXPORT FakeImageLoaderClient : public ImageLoaderClient {
 public:
  FakeImageLoaderClient() {}
  ~FakeImageLoaderClient() override {}

  // DBusClient ovveride.
  void Init(dbus::Bus* dbus) override {}

  // ImageLoaderClient override:
  void RegisterComponent(const std::string& name,
                         const std::string& version,
                         const std::string& component_folder_abs_path,
                         DBusMethodCallback<bool> callback) override;
  void LoadComponent(const std::string& name,
                     DBusMethodCallback<std::string> callback) override;
  void LoadComponentAtPath(
      const std::string& name,
      const base::FilePath& path,
      DBusMethodCallback<base::FilePath> callback) override;
  void RemoveComponent(const std::string& name,
                       DBusMethodCallback<bool> callback) override;
  void RequestComponentVersion(
      const std::string& name,
      DBusMethodCallback<std::string> callback) override;
  void UnmountComponent(const std::string& name,
                        DBusMethodCallback<bool> callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeImageLoaderClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_IMAGE_LOADER_CLIENT_H_
