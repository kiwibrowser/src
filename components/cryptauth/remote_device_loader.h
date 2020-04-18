// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_LOADER_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_LOADER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device.h"

namespace cryptauth {

class SecureMessageDelegate;

// Loads a collection of RemoteDevice objects from the given ExternalDeviceInfo
// protos that were synced from CryptAuth. We need to derive the PSK, which is
// a symmetric key used to authenticate each remote device.
class RemoteDeviceLoader {
 public:
  class Factory {
   public:
    static std::unique_ptr<RemoteDeviceLoader> NewInstance(
        const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
        const std::string& user_id,
        const std::string& user_private_key,
        std::unique_ptr<cryptauth::SecureMessageDelegate>
            secure_message_delegate);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<RemoteDeviceLoader> BuildInstance(
        const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
        const std::string& user_id,
        const std::string& user_private_key,
        std::unique_ptr<cryptauth::SecureMessageDelegate>
            secure_message_delegate);

   private:
    static Factory* factory_instance_;
  };

  // Creates the instance:
  // |device_info_list|: The ExternalDeviceInfo objects to convert to
  //                     RemoteDevice.
  // |user_private_key|: The private key of the user's local device. Used to
  //                     derive the PSK.
  // |secure_message_delegate|: Used to derive each persistent symmetric key.
  RemoteDeviceLoader(
      const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
      const std::string& user_id,
      const std::string& user_private_key,
      std::unique_ptr<cryptauth::SecureMessageDelegate>
          secure_message_delegate);

  virtual ~RemoteDeviceLoader();

  // Loads the RemoteDevice objects. |callback| will be invoked upon completion.
  // For space considerations, BeaconSeeds will only be loaded if
  // |should_load_beacon_seeds| is true.
  typedef base::Callback<void(const cryptauth::RemoteDeviceList&)>
      RemoteDeviceCallback;
  virtual void Load(bool should_load_beacon_seeds,
                    const RemoteDeviceCallback& callback);

 private:
  // Called when the PSK is derived for each device. If the PSKs for all devices
  // have been derived, then we can invoke |callback_|.
  void OnPSKDerived(const cryptauth::ExternalDeviceInfo& device,
                    const std::string& psk);

  // True if the loader should include BeaconSeeds in the loaded RemoteDevices.
  bool should_load_beacon_seeds_;

  // The remaining devices whose PSK we're waiting on.
  std::vector<cryptauth::ExternalDeviceInfo> remaining_devices_;

  // The id of the user who the remote devices belong to.
  const std::string user_id_;

  // The private key of the user's local device.
  const std::string user_private_key_;

  // Performs the PSK key derivation.
  std::unique_ptr<cryptauth::SecureMessageDelegate> secure_message_delegate_;

  // Invoked when the RemoteDevices are loaded.
  RemoteDeviceCallback callback_;

  // The collection of RemoteDevices to return.
  cryptauth::RemoteDeviceList remote_devices_;

  base::WeakPtrFactory<RemoteDeviceLoader> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceLoader);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_LOADER_H_
