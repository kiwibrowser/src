// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_MULTIDEVICE_SETUP_FAKE_MULTIDEVICE_SETUP_OBSERVER_H_
#define CHROMEOS_SERVICES_MULTIDEVICE_SETUP_FAKE_MULTIDEVICE_SETUP_OBSERVER_H_

#include "base/macros.h"
#include "chromeos/services/multidevice_setup/public/mojom/multidevice_setup.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace chromeos {

namespace multidevice_setup {

// Fake MultiDeviceSetupObserver implementation for tests.
class FakeMultiDeviceSetupObserver : public mojom::MultiDeviceSetupObserver {
 public:
  FakeMultiDeviceSetupObserver();
  ~FakeMultiDeviceSetupObserver() override;

  mojom::MultiDeviceSetupObserverPtr GenerateInterfacePtr();

  size_t num_new_user_events_handled() { return num_new_user_events_handled_; }

  size_t num_existing_user_host_switched_events_handled() {
    return num_existing_user_host_switched_events_handled_;
  }

  size_t num_existing_user_chromebook_added_events_handled() {
    return num_existing_user_chromebook_added_events_handled_;
  }

  // mojom::MultiDeviceSetupObserver:
  void OnPotentialHostExistsForNewUser() override;
  void OnConnectedHostSwitchedForExistingUser() override;
  void OnNewChromebookAddedForExistingUser() override;

 private:
  size_t num_new_user_events_handled_ = 0u;
  size_t num_existing_user_host_switched_events_handled_ = 0u;
  size_t num_existing_user_chromebook_added_events_handled_ = 0u;

  mojo::BindingSet<mojom::MultiDeviceSetupObserver> bindings_;

  DISALLOW_COPY_AND_ASSIGN(FakeMultiDeviceSetupObserver);
};

}  // namespace multidevice_setup

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_MULTIDEVICE_SETUP_FAKE_MULTIDEVICE_SETUP_OBSERVER_H_
