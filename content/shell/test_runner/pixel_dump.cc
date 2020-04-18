// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/pixel_dump.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/skia_paint_canvas.h"
#include "content/shell/test_runner/layout_test_runtime_flags.h"
#include "mojo/public/cpp/system/data_pipe_drainer.h"
#include "services/service_manager/public/cpp/connector.h"
// FIXME: Including platform_canvas.h here is a layering violation.
#include "skia/ext/platform_canvas.h"
#include "third_party/blink/public/mojom/clipboard/clipboard.mojom.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_image.h"
#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_page_popup.h"
#include "third_party/blink/public/web/web_print_params.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/point.h"

namespace test_runner {

namespace {

class BitmapDataPipeDrainer : public mojo::DataPipeDrainer::Client {
 public:
  BitmapDataPipeDrainer(mojo::ScopedDataPipeConsumerHandle handle,
                        base::OnceCallback<void(const SkBitmap&)> callback)
      : callback_(std::move(callback)), drainer_(this, std::move(handle)) {}

  void OnDataAvailable(const void* data, size_t num_bytes) override {
    const unsigned char* ptr = static_cast<const unsigned char*>(data);
    data_.insert(data_.end(), ptr, ptr + num_bytes);
  }

  void OnDataComplete() override {
    SkBitmap bitmap;
    if (!gfx::PNGCodec::Decode(data_.data(), data_.size(), &bitmap))
      bitmap.reset();
    std::move(callback_).Run(bitmap);
    delete this;
  }

 private:
  base::OnceCallback<void(const SkBitmap&)> callback_;
  mojo::DataPipeDrainer drainer_;
  std::vector<unsigned char> data_;
};

class CaptureCallback : public base::RefCountedThreadSafe<CaptureCallback> {
 public:
  explicit CaptureCallback(base::OnceCallback<void(const SkBitmap&)> callback);

  void set_wait_for_popup(bool wait) { wait_for_popup_ = wait; }
  void set_popup_position(const gfx::Point& position) {
    popup_position_ = position;
  }

  void DidCompositeAndReadback(const SkBitmap& bitmap);

 private:
  friend class base::RefCountedThreadSafe<CaptureCallback>;
  ~CaptureCallback();

  base::OnceCallback<void(const SkBitmap&)> callback_;
  SkBitmap main_bitmap_;
  bool wait_for_popup_;
  gfx::Point popup_position_;
};

void DrawSelectionRect(
    const blink::WebRect& wr,
    base::OnceCallback<void(const SkBitmap&)> original_callback,
    const SkBitmap& bitmap) {
  // Render a red rectangle bounding selection rect
  cc::SkiaPaintCanvas canvas(bitmap);
  cc::PaintFlags flags;
  flags.setColor(0xFFFF0000);  // Fully opaque red
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  flags.setStrokeWidth(1.0f);
  SkIRect rect;  // Bounding rect
  rect.set(wr.x, wr.y, wr.x + wr.width, wr.y + wr.height);
  canvas.drawIRect(rect, flags);

  std::move(original_callback).Run(bitmap);
}

void CapturePixelsForPrinting(
    blink::WebLocalFrame* web_frame,
    base::OnceCallback<void(const SkBitmap&)> callback) {
  auto* frame_widget = web_frame->LocalRoot()->FrameWidget();
  frame_widget->UpdateAllLifecyclePhases();

  blink::WebSize page_size_in_pixels = frame_widget->Size();

  int page_count = web_frame->PrintBegin(page_size_in_pixels);
  int totalHeight = page_count * (page_size_in_pixels.height + 1) - 1;

  bool is_opaque = false;

  SkBitmap bitmap;
  if (!bitmap.tryAllocN32Pixels(page_size_in_pixels.width, totalHeight,
                                is_opaque)) {
    LOG(ERROR) << "Failed to create bitmap width=" << page_size_in_pixels.width
               << " height=" << totalHeight;
    std::move(callback).Run(SkBitmap());
    return;
  }

  cc::SkiaPaintCanvas canvas(bitmap);
  web_frame->PrintPagesForTesting(&canvas, page_size_in_pixels);
  web_frame->PrintEnd();

  std::move(callback).Run(bitmap);
}

CaptureCallback::CaptureCallback(
    base::OnceCallback<void(const SkBitmap&)> callback)
    : callback_(std::move(callback)), wait_for_popup_(false) {}

CaptureCallback::~CaptureCallback() {}

void CaptureCallback::DidCompositeAndReadback(const SkBitmap& bitmap) {
  TRACE_EVENT2("shell", "CaptureCallback::didCompositeAndReadback", "x",
               bitmap.info().width(), "y", bitmap.info().height());
  if (!wait_for_popup_) {
    std::move(callback_).Run(bitmap);
    return;
  }
  if (main_bitmap_.isNull()) {
    if (main_bitmap_.tryAllocPixels(bitmap.info())) {
      bitmap.readPixels(main_bitmap_.info(), main_bitmap_.getPixels(),
                        main_bitmap_.rowBytes(), 0, 0);
    }
    return;
  }
  SkCanvas canvas(main_bitmap_);
  canvas.drawBitmap(bitmap, popup_position_.x(), popup_position_.y());
  std::move(callback_).Run(main_bitmap_);
}

}  // namespace

void DumpPixelsAsync(blink::WebLocalFrame* web_frame,
                     float device_scale_factor_for_test,
                     base::OnceCallback<void(const SkBitmap&)> callback) {
  DCHECK(web_frame);
  DCHECK_LT(0.0, device_scale_factor_for_test);
  DCHECK(!callback.is_null());

  blink::WebWidget* web_widget = web_frame->FrameWidget();
  auto capture_callback =
      base::MakeRefCounted<CaptureCallback>(std::move(callback));
  auto did_readback = base::BindRepeating(
      &CaptureCallback::DidCompositeAndReadback, capture_callback);
  web_widget->CompositeAndReadbackAsync(did_readback);
  if (blink::WebPagePopup* popup = web_widget->GetPagePopup()) {
    capture_callback->set_wait_for_popup(true);
    blink::WebPoint position = popup->PositionRelativeToOwner();
    position.x *= device_scale_factor_for_test;
    position.y *= device_scale_factor_for_test;
    capture_callback->set_popup_position(position);
    popup->CompositeAndReadbackAsync(did_readback);
  }
}

void PrintFrameAsync(blink::WebLocalFrame* web_frame,
                     base::OnceCallback<void(const SkBitmap&)> callback) {
  DCHECK(web_frame);
  DCHECK(!callback.is_null());
  web_frame->GetTaskRunner(blink::TaskType::kInternalTest)
      ->PostTask(FROM_HERE, base::BindOnce(&CapturePixelsForPrinting,
                                           base::Unretained(web_frame),
                                           std::move(callback)));
}

base::OnceCallback<void(const SkBitmap&)>
CreateSelectionBoundsRectDrawingCallback(
    blink::WebLocalFrame* web_frame,
    base::OnceCallback<void(const SkBitmap&)> original_callback) {
  DCHECK(web_frame);
  DCHECK(!original_callback.is_null());

  // If there is no selection rect, just return the original callback.
  blink::WebRect wr = web_frame->GetSelectionBoundsRectForTesting();
  if (wr.IsEmpty())
    return original_callback;

  return base::BindOnce(&DrawSelectionRect, wr, std::move(original_callback));
}

void CopyImageAtAndCapturePixels(
    blink::WebLocalFrame* web_frame,
    int x,
    int y,
    base::OnceCallback<void(const SkBitmap&)> callback) {
  blink::mojom::ClipboardHostPtr clipboard;
  blink::Platform::Current()->GetConnector()->BindInterface(
      blink::Platform::Current()->GetBrowserServiceName(), &clipboard);

  uint64_t sequence_number_before;
  clipboard->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE,
                               &sequence_number_before);
  web_frame->CopyImageAt(blink::WebPoint(x, y));
  uint64_t sequence_number_after;
  clipboard->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE,
                               &sequence_number_after);
  if (sequence_number_before == sequence_number_after) {
    std::move(callback).Run(SkBitmap());
    return;
  }

  blink::mojom::SerializedBlobPtr serialized_blob;
  clipboard->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE, &serialized_blob);
  blink::mojom::BlobPtr blob(std::move(serialized_blob->blob));
  mojo::DataPipe pipe;
  blob->ReadAll(std::move(pipe.producer_handle), nullptr);
  // Self-destructs after draining the pipe.
  new BitmapDataPipeDrainer(std::move(pipe.consumer_handle),
                            std::move(callback));
}

}  // namespace test_runner
