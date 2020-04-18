// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NOTIFICATIONS_NOTIFICATION_IMAGE_LOADER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NOTIFICATIONS_NOTIFICATION_IMAGE_LOADER_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

class ExecutionContext;
class KURL;
class ResourceError;

// Asynchronously downloads an image when given a url, decodes the loaded data,
// and passes the bitmap to the given callback.
class MODULES_EXPORT NotificationImageLoader final
    : public GarbageCollectedFinalized<NotificationImageLoader>,
      public ThreadableLoaderClient {
 public:
  // Type names are used in UMAs, so do not rename.
  enum class Type { kImage, kIcon, kBadge, kActionIcon };

  // The bitmap may be empty if the request failed or the image data could not
  // be decoded.
  using ImageCallback = base::OnceCallback<void(const SkBitmap&)>;

  explicit NotificationImageLoader(Type type);
  ~NotificationImageLoader() override;

  // Scales down |image| according to its type and returns result. If it is
  // already small enough, |image| is returned unchanged.
  static SkBitmap ScaleDownIfNeeded(const SkBitmap& image, Type type);

  // Asynchronously downloads an image from the given url, decodes the loaded
  // data, and passes the bitmap to the callback. Times out if the load takes
  // too long and ImageCallback is invoked with an empty bitmap.
  void Start(ExecutionContext* context,
             const KURL& url,
             ImageCallback image_callback);

  // Cancels the pending load, if there is one. The |m_imageCallback| will not
  // be run.
  void Stop();

  // ThreadableLoaderClient interface.
  void DidReceiveData(const char* data, unsigned length) override;
  void DidFinishLoading(unsigned long resource_identifier) override;
  void DidFail(const ResourceError& error) override;
  void DidFailRedirectCheck() override;

  void Trace(blink::Visitor* visitor) { visitor->Trace(threadable_loader_); }

 private:
  void RunCallbackWithEmptyBitmap();

  Type type_;
  bool stopped_;
  double start_time_;
  scoped_refptr<SharedBuffer> data_;
  ImageCallback image_callback_;
  Member<ThreadableLoader> threadable_loader_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NOTIFICATIONS_NOTIFICATION_IMAGE_LOADER_H_
