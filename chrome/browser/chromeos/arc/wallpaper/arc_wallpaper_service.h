// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_WALLPAPER_ARC_WALLPAPER_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_WALLPAPER_ARC_WALLPAPER_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "ash/public/interfaces/wallpaper.mojom.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_decoder.h"
#include "components/arc/common/wallpaper.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

// Lives on the UI thread.
class ArcWallpaperService : public KeyedService,
                            public ConnectionObserver<mojom::WallpaperInstance>,
                            public mojom::WallpaperHost,
                            public ash::mojom::WallpaperObserver {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcWallpaperService* GetForBrowserContext(
      content::BrowserContext* context);

  ArcWallpaperService(content::BrowserContext* context,
                      ArcBridgeService* bridge_service);
  ~ArcWallpaperService() override;

  // ConnectionObserver<mojom::WallpaperInstance> overrides.
  void OnConnectionReady() override;

  // mojom::WallpaperHost overrides.
  void SetWallpaper(const std::vector<uint8_t>& data,
                    int32_t wallpaper_id) override;
  void SetDefaultWallpaper() override;
  void GetWallpaper(GetWallpaperCallback callback) override;

  // ash::mojom::WallpaperObserver overrides.
  void OnWallpaperChanged(uint32_t image_id) override;
  void OnWallpaperColorsChanged(
      const std::vector<SkColor>& prominent_colors) override;
  void OnWallpaperBlurChanged(bool blurred) override;

  class DecodeRequestSender {
   public:
    virtual ~DecodeRequestSender();

    // Decodes image |data| and notifies the result to |request|.
    virtual void SendDecodeRequest(ImageDecoder::ImageRequest* request,
                                   const std::vector<uint8_t>& data) = 0;
  };

  // Replace a way to decode images for unittests. Originally it uses
  // ImageDecoder which communicates with the external process.
  void SetDecodeRequestSenderForTesting(
      std::unique_ptr<DecodeRequestSender> sender);

 private:
  friend class TestApi;
  class AndroidIdStore;
  class DecodeRequest;
  struct WallpaperIdPair;

  // Initiates a set wallpaper request to //ash.
  void OnWallpaperDecoded(const gfx::ImageSkia& image, int32_t android_id);

  // Notifies wallpaper change if we have wallpaper instance.
  void NotifyWallpaperChanged(int android_id);

  // Notifies wallpaper change of |android_id|, then notify wallpaper change of
  // -1 to reset wallpaper cache at Android side.
  void NotifyWallpaperChangedAndReset(int android_id);

  // If the wallpaper is allowed to be shown on screen, stores the |image_id|
  // in order to track the wallpaper change later, otherwise notify the Android
  // side immediately that the request is not going through.
  void OnSetThirdPartyWallpaperCallback(int32_t android_id,
                                        bool allowed,
                                        uint32_t image_id);

  // Initiates an encoding image request after getting the wallpaper image.
  void OnGetWallpaperImageCallback(GetWallpaperCallback callback,
                                   const gfx::ImageSkia& image);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  std::unique_ptr<DecodeRequest> decode_request_;
  std::vector<WallpaperIdPair> id_pairs_;
  std::unique_ptr<DecodeRequestSender> decode_request_sender_;

  // The binding this instance uses to implement ash::mojom::WallpaperObserver.
  mojo::AssociatedBinding<ash::mojom::WallpaperObserver> observer_binding_;

  base::WeakPtrFactory<ArcWallpaperService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcWallpaperService);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_WALLPAPER_ARC_WALLPAPER_SERVICE_H_
