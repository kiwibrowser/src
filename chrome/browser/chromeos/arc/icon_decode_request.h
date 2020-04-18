// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ICON_DECODE_REQUEST_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ICON_DECODE_REQUEST_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/image_decoder.h"

namespace gfx {
class ImageSkia;
}  // namespace gfx

namespace arc {

class IconDecodeRequest : public ImageDecoder::ImageRequest {
 public:
  using SetIconCallback = base::OnceCallback<void(const gfx::ImageSkia& icon)>;

  IconDecodeRequest(SetIconCallback set_icon_callback, int requested_size);
  ~IconDecodeRequest() override;

  // Disables async safe decoding requests when unit tests are executed.
  // Icons are decoded at a separate process created by ImageDecoder. In unit
  // tests these tasks may not finish before the test exits, which causes a
  // failure in the base::MessageLoopCurrent::Get()->IsIdleForTesting() check
  // in test_browser_thread_bundle.cc.
  static void DisableSafeDecodingForTesting();

  // Starts image decoding. Safe asynchronous decoding is used unless
  // DisableSafeDecodingForTesting() is called.
  void StartWithOptions(const std::vector<uint8_t>& image_data);

  // ImageDecoder::ImageRequest:
  void OnImageDecoded(const SkBitmap& bitmap) override;
  void OnDecodeImageFailed() override;

 private:
  SetIconCallback set_icon_callback_;
  int requested_size_ = 0;

  DISALLOW_COPY_AND_ASSIGN(IconDecodeRequest);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ICON_DECODE_REQUEST_H_
