/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_IMAGEBITMAP_IMAGE_BITMAP_FACTORIES_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_IMAGEBITMAP_IMAGE_BITMAP_FACTORIES_H_

#include <memory>
#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/image_bitmap_source.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/fileapi/file_reader_loader.h"
#include "third_party/blink/renderer/core/fileapi/file_reader_loader_client.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap_options.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkImage;

namespace blink {

class Blob;
class EventTarget;
class ImageBitmapSource;
class ImageBitmapOptions;

typedef HTMLImageElementOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrBlobOrImageDataOrImageBitmapOrOffscreenCanvas
    ImageBitmapSourceUnion;

class ImageBitmapFactories final
    : public GarbageCollectedFinalized<ImageBitmapFactories>,
      public Supplement<LocalDOMWindow>,
      public Supplement<WorkerGlobalScope>,
      public TraceWrapperBase {
  USING_GARBAGE_COLLECTED_MIXIN(ImageBitmapFactories);

 public:
  static const char kSupplementName[];

  static ScriptPromise createImageBitmap(ScriptState*,
                                         EventTarget&,
                                         const ImageBitmapSourceUnion&,
                                         const ImageBitmapOptions&);
  static ScriptPromise createImageBitmap(ScriptState*,
                                         EventTarget&,
                                         const ImageBitmapSourceUnion&,
                                         int sx,
                                         int sy,
                                         int sw,
                                         int sh,
                                         const ImageBitmapOptions&);
  static ScriptPromise createImageBitmap(ScriptState*,
                                         EventTarget&,
                                         ImageBitmapSource*,
                                         base::Optional<IntRect> crop_rect,
                                         const ImageBitmapOptions&);
  static ScriptPromise CreateImageBitmapFromBlob(
      ScriptState*,
      EventTarget&,
      ImageBitmapSource*,
      base::Optional<IntRect> crop_rect,
      const ImageBitmapOptions&);

  virtual ~ImageBitmapFactories() = default;

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "ImageBitmapLoader";
  }

 private:
  class ImageBitmapLoader final
      : public GarbageCollectedFinalized<ImageBitmapLoader>,
        public FileReaderLoaderClient {
   public:
    static ImageBitmapLoader* Create(ImageBitmapFactories& factory,
                                     base::Optional<IntRect> crop_rect,
                                     const ImageBitmapOptions& options,
                                     ScriptState* script_state) {
      return new ImageBitmapLoader(factory, crop_rect, script_state, options);
    }

    void LoadBlobAsync(Blob*);
    ScriptPromise Promise() { return resolver_->Promise(); }

    void Trace(blink::Visitor*);

    ~ImageBitmapLoader() override = default;

   private:
    ImageBitmapLoader(ImageBitmapFactories&,
                      base::Optional<IntRect> crop_rect,
                      ScriptState*,
                      const ImageBitmapOptions&);

    enum ImageBitmapRejectionReason {
      kUndecodableImageBitmapRejectionReason,
      kAllocationFailureImageBitmapRejectionReason,
    };

    void RejectPromise(ImageBitmapRejectionReason);

    void ScheduleAsyncImageBitmapDecoding(DOMArrayBuffer*);
    void DecodeImageOnDecoderThread(
        scoped_refptr<base::SingleThreadTaskRunner>,
        DOMArrayBuffer*,
        const String& premultiply_alpha_option,
        const String& color_space_conversion_option);
    void ResolvePromiseOnOriginalThread(sk_sp<SkImage>);

    // FileReaderLoaderClient
    void DidStartLoading() override {}
    void DidReceiveData() override {}
    void DidFinishLoading() override;
    void DidFail(FileError::ErrorCode) override;

    std::unique_ptr<FileReaderLoader> loader_;
    Member<ImageBitmapFactories> factory_;
    Member<ScriptPromiseResolver> resolver_;
    base::Optional<IntRect> crop_rect_;
    ImageBitmapOptions options_;
  };

  static ImageBitmapFactories& From(EventTarget&);

  template <class GlobalObject>
  static ImageBitmapFactories& FromInternal(GlobalObject&);

  void AddLoader(ImageBitmapLoader*);
  void DidFinishLoading(ImageBitmapLoader*);

  HeapHashSet<Member<ImageBitmapLoader>> pending_loaders_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_IMAGEBITMAP_IMAGE_BITMAP_FACTORIES_H_
