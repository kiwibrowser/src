// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CAST_CONFIG_CLIENT_MEDIA_ROUTER_H_
#define CHROME_BROWSER_UI_ASH_CAST_CONFIG_CLIENT_MEDIA_ROUTER_H_

#include <memory>

#include "ash/public/interfaces/cast_config.mojom.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace media_router {
class MediaRouter;
}

class CastDeviceCache;

// A class which allows the ash tray to communicate with the media router.
class CastConfigClientMediaRouter : public ash::mojom::CastConfigClient,
                                    public content::NotificationObserver {
 public:
  CastConfigClientMediaRouter();
  ~CastConfigClientMediaRouter() override;

  static void SetMediaRouterForTest(media_router::MediaRouter* media_router);

 private:
  // CastConfigClient:
  void RequestDeviceRefresh() override;
  void CastToSink(ash::mojom::CastSinkPtr sink) override;
  void StopCasting(ash::mojom::CastRoutePtr route) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // |devices_| stores the current source/route status that we query from.
  // This will return null until the media router is initialized.
  CastDeviceCache* devices();
  std::unique_ptr<CastDeviceCache> devices_;

  content::NotificationRegistrar registrar_;

  ash::mojom::CastConfigPtr cast_config_;

  mojo::AssociatedBinding<ash::mojom::CastConfigClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(CastConfigClientMediaRouter);
};

#endif  // CHROME_BROWSER_UI_ASH_CAST_CONFIG_CLIENT_MEDIA_ROUTER_H_
