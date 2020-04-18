// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHUTDOWN_CONTROLLER_H_
#define ASH_SHUTDOWN_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/shutdown.mojom.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

enum class ShutdownReason;

// Handles actual device shutdown by making requests to powerd over D-Bus.
// Caches the DeviceRebootOnShutdown device policy sent from Chrome over mojo.
class ASH_EXPORT ShutdownController : public mojom::ShutdownController {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    // Called when shutdown policy changes.
    virtual void OnShutdownPolicyChanged(bool reboot_on_shutdown) = 0;
  };

  ShutdownController();
  ~ShutdownController() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool reboot_on_shutdown() const { return reboot_on_shutdown_; }

  // Shuts down or reboots based on the current DeviceRebootOnShutdown policy.
  // Does not trigger the shutdown fade-out animation. For animated shutdown
  // use ShellPort::RequestShutdown(). Virtual for testing.
  virtual void ShutDownOrReboot(ShutdownReason reason);

  // Binds the mojom::ShutdownController interface request to this object.
  void BindRequest(mojom::ShutdownControllerRequest request);

  // Sets the reboot policy. Used for testing only.
  void SetRebootOnShutdownForTesting(bool reboot_on_shutdown);

 private:
  // mojom::ShutdownController:
  void SetRebootOnShutdown(bool reboot_on_shutdown) override;
  void RequestShutdownFromLoginScreen() override;

  // Cached copy of the DeviceRebootOnShutdown policy from chrome.
  bool reboot_on_shutdown_ = false;

  base::ObserverList<Observer> observers_;

  // Bindings for the ShutdownController interface.
  mojo::BindingSet<mojom::ShutdownController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ShutdownController);
};

}  // namespace ash

#endif  // ASH_SHUTDOWN_CONTROLLER_H_
