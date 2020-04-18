// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_CHROME_VIRTUAL_FILE_REQUEST_SERVICE_PROVIDER_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_CHROME_VIRTUAL_FILE_REQUEST_SERVICE_PROVIDER_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/services/virtual_file_request_service_provider.h"

namespace chromeos {

// Chrome's VirtualFileRequestServiceProvider::Delegate implementation.
class ChromeVirtualFileRequestServiceProviderDelegate
    : public VirtualFileRequestServiceProvider::Delegate {
 public:
  ChromeVirtualFileRequestServiceProviderDelegate();
  ~ChromeVirtualFileRequestServiceProviderDelegate() override;

  // VirtualFileRequestServiceProvider::Delegate overrides:
  bool HandleReadRequest(const std::string& id,
                         int64_t offset,
                         int64_t size,
                         base::ScopedFD pipe_write_end) override;
  bool HandleIdReleased(const std::string& id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeVirtualFileRequestServiceProviderDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_CHROME_VIRTUAL_FILE_REQUEST_SERVICE_PROVIDER_DELEGATE_H_
