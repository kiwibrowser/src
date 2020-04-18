// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "build/build_config.h"
#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/geolocation_handler.h"
#endif
#include "device/geolocation/geolocation_provider_impl.h"
#include "device/geolocation/network_location_request.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "services/device/device_service_test_base.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/geolocation.mojom.h"
#include "services/device/public/mojom/geolocation_config.mojom.h"
#include "services/device/public/mojom/geolocation_context.mojom.h"
#include "services/device/public/mojom/geolocation_control.mojom.h"

namespace device {

namespace {

void CheckBoolReturnValue(base::OnceClosure quit_closure,
                          bool expect,
                          bool result) {
  EXPECT_EQ(expect, result);
  std::move(quit_closure).Run();
}

// Observer that waits until a TestURLFetcher with the specified fetcher_id
// starts, after which it is made available through .fetcher().
class TestURLFetcherObserver : public net::TestURLFetcher::DelegateForTests {
 public:
  explicit TestURLFetcherObserver(int expected_fetcher_id)
      : expected_fetcher_id_(expected_fetcher_id) {
    factory_.SetDelegateForTests(this);
  }
  virtual ~TestURLFetcherObserver() {}

  void Wait() { loop_.Run(); }

  net::TestURLFetcher* fetcher() { return fetcher_; }

  // net::TestURLFetcher::DelegateForTests:
  void OnRequestStart(int fetcher_id) override {
    if (fetcher_id == expected_fetcher_id_) {
      fetcher_ = factory_.GetFetcherByID(fetcher_id);
      fetcher_->SetDelegateForTests(nullptr);
      factory_.SetDelegateForTests(nullptr);
      loop_.Quit();
    }
  }
  void OnChunkUpload(int fetcher_id) override {}
  void OnRequestEnd(int fetcher_id) override {}

 private:
  const int expected_fetcher_id_;
  net::TestURLFetcher* fetcher_ = nullptr;
  net::TestURLFetcherFactory factory_;
  base::RunLoop loop_;
};

class GeolocationServiceUnitTest : public DeviceServiceTestBase {
 public:
  GeolocationServiceUnitTest() = default;
  ~GeolocationServiceUnitTest() override = default;

 protected:
  void SetUp() override {
    DeviceServiceTestBase::SetUp();

#if defined(OS_CHROMEOS)
    chromeos::DBusThreadManager::Initialize();
    chromeos::NetworkHandler::Initialize();
#endif

    connector()->BindInterface(mojom::kServiceName, &geolocation_control_);
    geolocation_control_->UserDidOptIntoLocationServices();

    connector()->BindInterface(mojom::kServiceName, &geolocation_context_);
    geolocation_context_->BindGeolocation(MakeRequest(&geolocation_));
  }

  void TearDown() override {
    DeviceServiceTestBase::TearDown();

#if defined(OS_CHROMEOS)
    chromeos::NetworkHandler::Shutdown();
    chromeos::DBusThreadManager::Shutdown();
#endif

    // Let the GeolocationImpl destruct earlier than GeolocationProviderImpl to
    // make sure the base::CallbackList<> member in GeolocationProviderImpl is
    // empty.
    geolocation_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void BindGeolocationConfig() {
    connector()->BindInterface(mojom::kServiceName, &geolocation_config_);
  }

  mojom::GeolocationControlPtr geolocation_control_;
  mojom::GeolocationContextPtr geolocation_context_;
  mojom::GeolocationPtr geolocation_;
  mojom::GeolocationConfigPtr geolocation_config_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationServiceUnitTest);
};

#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
// ChromeOS fails to perform network geolocation when zero wifi networks are
// detected in a scan: https://crbug.com/767300.
#else
TEST_F(GeolocationServiceUnitTest, UrlWithApiKey) {
  // Unique ID (derived from Gerrit CL number):
  device::NetworkLocationRequest::url_fetcher_id_for_tests = 675023;

  // Intercept the URLFetcher from network geolocation request.
  TestURLFetcherObserver observer(
      device::NetworkLocationRequest::url_fetcher_id_for_tests);

  geolocation_->SetHighAccuracy(true);
  observer.Wait();
  DCHECK(observer.fetcher());

  // Verify full URL including a fake Google API key.
  std::string expected_url =
      "https://www.googleapis.com/geolocation/v1/"
      "geolocate?key=";
  expected_url.append(kTestGeolocationApiKey);
  EXPECT_EQ(expected_url, observer.fetcher()->GetOriginalURL());
}
#endif

TEST_F(GeolocationServiceUnitTest, GeolocationConfig) {
  BindGeolocationConfig();
  {
    base::RunLoop run_loop;
    geolocation_config_->IsHighAccuracyLocationBeingCaptured(
        base::BindOnce(&CheckBoolReturnValue, run_loop.QuitClosure(), false));
    run_loop.Run();
  }

  geolocation_->SetHighAccuracy(true);
  {
    base::RunLoop run_loop;
    geolocation_config_->IsHighAccuracyLocationBeingCaptured(
        base::BindOnce(&CheckBoolReturnValue, run_loop.QuitClosure(), true));
    run_loop.Run();
  }
}

}  // namespace

}  // namespace device
