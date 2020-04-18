// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_DEVICE_LISTER_H_
#define CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_DEVICE_LISTER_H_

#include <string>

#include "chrome/browser/printing/cloud_print/device_description.h"

namespace cloud_print {

class PrivetDeviceLister {
 public:
  PrivetDeviceLister();
  virtual ~PrivetDeviceLister();

  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void DeviceChanged(const std::string& name,
                               const DeviceDescription& description) = 0;
    virtual void DeviceRemoved(const std::string& name) = 0;
    virtual void DeviceCacheFlushed() = 0;
  };

  // Start the PrivetServiceLister.
  virtual void Start() = 0;

  virtual void DiscoverNewDevices() = 0;
};

}  // namespace cloud_print

#endif  // CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_DEVICE_LISTER_H_
