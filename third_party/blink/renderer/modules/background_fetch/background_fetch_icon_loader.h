// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_ICON_LOADER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_ICON_LOADER_H_

#include <memory>

#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_type_converters.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

class BackgroundFetchBridge;
class IconDefinition;
struct WebSize;

class MODULES_EXPORT BackgroundFetchIconLoader final
    : public GarbageCollectedFinalized<BackgroundFetchIconLoader>,
      public ThreadableLoaderClient {
 public:
  // The bitmap may be empty if the request failed or the image data
  // could not be decoded.
  using IconCallback = base::OnceCallback<void(const SkBitmap&)>;

  BackgroundFetchIconLoader();
  ~BackgroundFetchIconLoader() override;

  // Scales down |icon| and returns result. If it is already small enough,
  // |icon| is returned unchanged.
  static SkBitmap ScaleDownIfNeeded(const SkBitmap& icon);

  // Asynchronously download an icon from the given url, decodes the loaded
  // data, and passes the bitmap to the given callback.
  void Start(BackgroundFetchBridge* bridge,
             ExecutionContext* execution_context,
             HeapVector<IconDefinition>,
             IconCallback callback);

  // Cancels the pending load, if there is one. The |icon_callback_| will not
  // be run.
  void Stop();

  // ThreadableLoaderClient interface.
  void DidReceiveData(const char* data, unsigned length) override;
  void DidFinishLoading(unsigned long resource_identifier) override;
  void DidFail(const ResourceError& error) override;
  void DidFailRedirectCheck() override;

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(threadable_loader_);
    visitor->Trace(icons_);
  }

 private:
  friend class BackgroundFetchIconLoaderTest;
  void RunCallbackWithEmptyBitmap();

  // Callback for BackgroundFetchBridge::GetIconDisplaySize()
  void DidGetIconDisplaySizeIfSoLoadIcon(
      ExecutionContext* execution_context,
      IconCallback callback,
      const WebSize& icon_display_size_pixels);

  // Picks the best icon from the list of developer provided icons, for current
  // display, given the ideal |icon_display_size_pixels|, and returns its index
  // in the icons_ array.
  int PickBestIconForDisplay(ExecutionContext* execution_context,
                             const WebSize& icon_display_size_pixels);

  // Get a score for the given icon, based on ideal_size. The icon with the
  // highest score is chosen.
  double GetIconScore(IconDefinition icon, const int ideal_size);

  bool stopped_ = false;
  scoped_refptr<SharedBuffer> data_;
  IconCallback icon_callback_;
  HeapVector<IconDefinition> icons_;
  Member<ThreadableLoader> threadable_loader_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_ICON_LOADER_H_
