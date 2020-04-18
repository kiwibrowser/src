// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_CHANGE_NOTIFIER_FACTORY_CHROMEOS_H_
#define CHROMEOS_NETWORK_NETWORK_CHANGE_NOTIFIER_FACTORY_CHROMEOS_H_

#include "base/compiler_specific.h"
#include "chromeos/chromeos_export.h"
#include "net/base/network_change_notifier_factory.h"

namespace chromeos {

class NetworkChangeNotifierChromeos;

class CHROMEOS_EXPORT NetworkChangeNotifierFactoryChromeos
    : public net::NetworkChangeNotifierFactory {
 public:
  NetworkChangeNotifierFactoryChromeos() {}

  // net::NetworkChangeNotifierFactory overrides.
  net::NetworkChangeNotifier* CreateInstance() override;

  static NetworkChangeNotifierChromeos* GetInstance();
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_CHANGE_NOTIFIER_FACTORY_CHROMEOS_H_
