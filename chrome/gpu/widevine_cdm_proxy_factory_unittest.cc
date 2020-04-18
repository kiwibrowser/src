// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/widevine_cdm_proxy_factory.h"

#include "media/cdm/cdm_proxy.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(WidevineCdmProxyFactoryTest, CreateWidevineCdmProxy) {
  // This fucntion may return null on unsupported devices. Hence ignore the
  // return value and just make sure we do not crash in all cases.
  CreateWidevineCdmProxy();
}
