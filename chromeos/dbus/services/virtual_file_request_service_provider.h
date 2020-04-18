// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SERVICES_VIRTUAL_FILE_REQUEST_SERVICE_PROVIDER_H_
#define CHROMEOS_DBUS_SERVICES_VIRTUAL_FILE_REQUEST_SERVICE_PROVIDER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/services/cros_dbus_service.h"
#include "dbus/exported_object.h"

namespace dbus {
class MethodCall;
}

namespace chromeos {

// VirtualFileRequestServiceProvider exposes D-Bus methods which will be
// called by the VirtualFileProvider service.
class CHROMEOS_EXPORT VirtualFileRequestServiceProvider
    : public CrosDBusService::ServiceProviderInterface {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Writes the requested data to the given pipe write end.
    virtual bool HandleReadRequest(const std::string& id,
                                   int64_t offset,
                                   int64_t size,
                                   base::ScopedFD pipe_write_end) = 0;

    // Releases resources associated with the ID.
    virtual bool HandleIdReleased(const std::string& id) = 0;
  };

  explicit VirtualFileRequestServiceProvider(
      std::unique_ptr<Delegate> delegate);
  ~VirtualFileRequestServiceProvider() override;

  // CrosDBusService::ServiceProviderInterface overrides:
  void Start(scoped_refptr<dbus::ExportedObject> exported_object) override;

 private:
  // Called on UI thread to handle incoming D-Bus method calls.
  void HandleReadRequest(dbus::MethodCall* method_call,
                         dbus::ExportedObject::ResponseSender response_sender);
  void HandleIdReleased(dbus::MethodCall* method_call,
                        dbus::ExportedObject::ResponseSender response_sender);

  std::unique_ptr<Delegate> delegate_;

  // Keep this last so that all weak pointers will be invalidated at the
  // beginning of destruction.
  base::WeakPtrFactory<VirtualFileRequestServiceProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VirtualFileRequestServiceProvider);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SERVICES_VIRTUAL_FILE_REQUEST_SERVICE_PROVIDER_H_
