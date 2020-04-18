// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_RENDERER_DATE_TIME_PICKER_H_
#define CONTENT_RENDERER_ANDROID_RENDERER_DATE_TIME_PICKER_H_

#include "base/macros.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/blink/public/web/web_date_time_chooser_params.h"

namespace blink {
class WebDateTimeChooserCompletion;
}  // namespace blink

namespace content {
class RenderViewImpl;

class RendererDateTimePicker : public RenderViewObserver {
 public:
  RendererDateTimePicker(
      RenderViewImpl* sender,
      const blink::WebDateTimeChooserParams& params,
      blink::WebDateTimeChooserCompletion* completion);
  ~RendererDateTimePicker() override;

  bool Open();

 private:
  void OnReplaceDateTime(double value);
  void OnCancel();

  // RenderViewObserver
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;

  blink::WebDateTimeChooserParams chooser_params_;
  blink::WebDateTimeChooserCompletion* chooser_completion_;  // Not owned by us

  DISALLOW_COPY_AND_ASSIGN(RendererDateTimePicker);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ANDROID_RENDERER_DATE_TIME_PICKER_H_
