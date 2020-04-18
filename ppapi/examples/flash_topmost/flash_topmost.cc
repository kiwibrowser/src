// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/logging.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/flash.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/size.h"
#include "ppapi/utility/completion_callback_factory.h"

const int32_t kTimerInterval = 200;

class MyInstance : public pp::Instance {
 public:
  explicit MyInstance(PP_Instance instance)
      : pp::Instance(instance),
        callback_factory_(this),
        pending_paint_(false),
        waiting_for_flush_completion_(false) {
  }
  virtual ~MyInstance() {
  }

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    ScheduleNextTimer();
    return true;
  }

  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
    if (position.size() != size_) {
      size_ = position.size();
      device_context_ = pp::Graphics2D(this, size_, false);
      if (!BindGraphics(device_context_))
        return;
    }

    Paint();
  }

 private:
  void ScheduleNextTimer() {
    pp::Module::Get()->core()->CallOnMainThread(
        kTimerInterval,
        callback_factory_.NewCallback(&MyInstance::OnTimer),
        0);
  }

  void OnTimer(int32_t) {
    ScheduleNextTimer();
    Paint();
  }

  void DidFlush(int32_t result) {
    waiting_for_flush_completion_ = false;
    if (pending_paint_)
      Paint();
  }

  void Paint() {
    if (waiting_for_flush_completion_) {
      pending_paint_ = true;
      return;
    }

    pending_paint_ = false;

    if (size_.IsEmpty())
      return;  // Nothing to do.

    pp::ImageData image = PaintImage(size_);
    if (!image.is_null()) {
      device_context_.ReplaceContents(&image);
      waiting_for_flush_completion_ = true;
      device_context_.Flush(
          callback_factory_.NewCallback(&MyInstance::DidFlush));
    }
  }

  pp::ImageData PaintImage(const pp::Size& size) {
    pp::ImageData image(this, PP_IMAGEDATAFORMAT_BGRA_PREMUL, size, false);
    if (image.is_null())
      return image;

    pp::Rect rect(size.width() / 8, size.height() / 4,
                  3 * size.width() / 4, size.height() / 2);
    uint32_t fill_color = pp::flash::Flash::IsRectTopmost(this, rect) ?
        0xff00ff00 : 0xffff0000;

    for (int y = 0; y < size.height(); y++) {
      for (int x = 0; x < size.width(); x++)
        *image.GetAddr32(pp::Point(x, y)) = fill_color;
    }

    for (int x = rect.x(); x < rect.x() + rect.width(); x++) {
      *image.GetAddr32(pp::Point(x, rect.y())) = 0xff202020;
      *image.GetAddr32(pp::Point(x, rect.y() + rect.height() - 1)) = 0xff202020;
    }
    for (int y = rect.y(); y < rect.y() + rect.height(); y++) {
      *image.GetAddr32(pp::Point(rect.x(), y)) = 0xff202020;
      *image.GetAddr32(pp::Point(rect.x() + rect.width() - 1, y)) = 0xff202020;
    }

    return image;
  }

  pp::CompletionCallbackFactory<MyInstance> callback_factory_;

  // Painting stuff.
  pp::Size size_;
  pp::Graphics2D device_context_;
  bool pending_paint_;
  bool waiting_for_flush_completion_;
};

class MyModule : public pp::Module {
 public:
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new MyInstance(instance);
  }
};

namespace pp {

// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new MyModule();
}

}  // namespace pp
