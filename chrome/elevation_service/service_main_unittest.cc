// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/elevation_service/service_main.h"

#include <wrl/client.h>
#include <memory>

#include "base/win/scoped_com_initializer.h"
#include "chrome/elevation_service/elevation_service_idl.h"
#include "chrome/install_static/install_util.h"
#include "testing/gtest/include/gtest/gtest.h"

class ServiceMainTest : public testing::Test {
 protected:
  ServiceMainTest() = default;

  void SetUp() override {
    com_initializer_ = std::make_unique<base::win::ScopedCOMInitializer>();
    ASSERT_TRUE(com_initializer_->Succeeded());

    service_main_ = elevation_service::ServiceMain::GetInstance();
    HRESULT hr = service_main_->RegisterClassObjects();
    if (SUCCEEDED(hr))
      class_registration_succeeded_ = true;

    ASSERT_HRESULT_SUCCEEDED(hr);
  }

  void TearDown() override {
    if (class_registration_succeeded_)
      service_main_->UnregisterClassObjects();

    com_initializer_.reset();
  }

  elevation_service::ServiceMain* service_main() { return service_main_; }

 private:
  elevation_service::ServiceMain* service_main_ = nullptr;
  std::unique_ptr<base::win::ScopedCOMInitializer> com_initializer_;
  bool class_registration_succeeded_ = false;

  DISALLOW_COPY_AND_ASSIGN(ServiceMainTest);
};

TEST_F(ServiceMainTest, ExitSignalTest) {
  // The waitable event starts unsignaled.
  ASSERT_FALSE(service_main()->IsExitSignaled());

  Microsoft::WRL::ComPtr<IUnknown> elevator;
  ASSERT_HRESULT_SUCCEEDED(
      ::CoCreateInstance(install_static::GetElevatorClsid(), nullptr,
                         CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&elevator)));

  // An object instance has been created upon the request, and is held by the
  // server module. Therefore, the waitable event remains unsignaled.
  ASSERT_FALSE(service_main()->IsExitSignaled());

  // Release the instance object. Now that the last (and the only) instance
  // object of the module is released, the event becomes signaled.
  elevator.Reset();
  ASSERT_TRUE(service_main()->IsExitSignaled());
}
