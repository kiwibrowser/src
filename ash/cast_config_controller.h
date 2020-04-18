// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_CAST_CONFIG_CONTROLLER_H_
#define ASH_CAST_CONFIG_CONTROLLER_H_

#include <vector>

#include "ash/public/interfaces/cast_config.mojom.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

// There is a single CastConfigController which receives device
// information. This observer interface sends that information to all the
// TrayCast items--there is one per display.
class CastConfigControllerObserver {
 public:
  virtual void OnDevicesUpdated(
      std::vector<mojom::SinkAndRoutePtr> devices) = 0;

 protected:
  virtual ~CastConfigControllerObserver() {}
};

// We want to establish our connection lazily and preferably only once, as
// TrayCast instances will come and go.
class CastConfigController : public ash::mojom::CastConfig {
 public:
  CastConfigController();
  ~CastConfigController() override;

  // Returns whether our SetClient() method has been called and the client
  // object pointer is still live.
  bool Connected();

  void AddObserver(CastConfigControllerObserver* observer);
  void RemoveObserver(CastConfigControllerObserver* observer);

  void BindRequest(mojom::CastConfigRequest request);

  // Return true if there are available cast devices.
  bool HasSinksAndRoutes() const;

  // Return true if casting is active. The route may be DIAL based, such as
  // casting YouTube where the cast sink directly streams content from another
  // server. In that case, this device is not actively transmitting information
  // to the cast sink.
  bool HasActiveRoute() const;

  // ash::mojom::CastConfig:
  void SetClient(mojom::CastConfigClientAssociatedPtrInfo client) override;
  void OnDevicesUpdated(std::vector<mojom::SinkAndRoutePtr> devices) override;

  // Methods to forward to |client_|.
  void RequestDeviceRefresh();
  void CastToSink(mojom::CastSinkPtr sink);
  void StopCasting(mojom::CastRoutePtr route);

 private:
  // Bindings for the CastConfig interface.
  mojo::BindingSet<mojom::CastConfig> bindings_;

  mojom::CastConfigClientAssociatedPtr client_;

  std::vector<mojom::SinkAndRoutePtr> sinks_and_routes_;

  base::ObserverList<CastConfigControllerObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(CastConfigController);
};

}  // namespace ash

#endif  // ASH_CAST_CONFIG_CONTROLLER_H_
