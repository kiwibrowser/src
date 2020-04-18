// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_DELEGATE_CHROMEOS_H_
#define CHROME_BROWSER_EXTENSIONS_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_DELEGATE_CHROMEOS_H_

#include "base/callback_forward.h"
#include "extensions/browser/api/media_perception_private/media_perception_api_delegate.h"

namespace extensions {

class MediaPerceptionAPIDelegateChromeOS
    : public extensions::MediaPerceptionAPIDelegate {
 public:
  MediaPerceptionAPIDelegateChromeOS();
  ~MediaPerceptionAPIDelegateChromeOS() override;

  // extensions::MediaPerceptionAPIDelegate:
  void LoadCrOSComponent(
      const api::media_perception_private::ComponentType& type,
      LoadCrOSComponentCallback load_callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaPerceptionAPIDelegateChromeOS);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_DELEGATE_CHROMEOS_H_
