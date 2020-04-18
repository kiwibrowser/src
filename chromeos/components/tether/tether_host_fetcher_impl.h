// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_TETHER_HOST_FETCHER_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_TETHER_HOST_FETCHER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "chromeos/components/tether/tether_host_fetcher.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/cryptauth/remote_device_ref.h"

namespace cryptauth {
class RemoteDeviceProvider;
}  // namespace cryptauth

namespace chromeos {

namespace tether {

// Concrete TetherHostFetcher implementation. Despite the asynchronous function
// prototypes, callbacks are invoked synchronously.
class TetherHostFetcherImpl : public TetherHostFetcher,
                              public cryptauth::RemoteDeviceProvider::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<TetherHostFetcher> NewInstance(
        cryptauth::RemoteDeviceProvider* remote_device_provider);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<TetherHostFetcher> BuildInstance(
        cryptauth::RemoteDeviceProvider* remote_device_provider);

   private:
    static Factory* factory_instance_;
  };

  ~TetherHostFetcherImpl() override;

  // TetherHostFetcher:
  bool HasSyncedTetherHosts() override;
  void FetchAllTetherHosts(const TetherHostListCallback& callback) override;
  void FetchTetherHost(const std::string& device_id,
                       const TetherHostCallback& callback) override;

  // cryptauth::RemoteDeviceProvider::Observer:
  void OnSyncDeviceListChanged() override;

 protected:
  explicit TetherHostFetcherImpl(
      cryptauth::RemoteDeviceProvider* remote_device_provider);

 private:
  void CacheCurrentTetherHosts();

  cryptauth::RemoteDeviceProvider* remote_device_provider_;

  cryptauth::RemoteDeviceRefList current_remote_device_list_;

  DISALLOW_COPY_AND_ASSIGN(TetherHostFetcherImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_TETHER_HOST_FETCHER_IMPL_H_
