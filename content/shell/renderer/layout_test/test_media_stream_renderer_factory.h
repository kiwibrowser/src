// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_RENDERER_LAYOUT_TEST_TEST_MEDIA_STREAM_RENDERER_FACTORY_H_
#define CONTENT_SHELL_RENDERER_LAYOUT_TEST_TEST_MEDIA_STREAM_RENDERER_FACTORY_H_

#include <string>

#include "base/callback_forward.h"
#include "content/public/renderer/media_stream_renderer_factory.h"

namespace content {

// TestMediaStreamClient is a mock implementation of MediaStreamClient used when
// running layout tests.
class TestMediaStreamRendererFactory : public MediaStreamRendererFactory {
 public:
  TestMediaStreamRendererFactory();
  ~TestMediaStreamRendererFactory() override;

  // MediaStreamRendererFactory implementation.
  scoped_refptr<MediaStreamVideoRenderer> GetVideoRenderer(
      const blink::WebMediaStream& web_stream,
      const base::Closure& error_cb,
      const MediaStreamVideoRenderer::RepaintCB& repaint_cb,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
      override;

  scoped_refptr<MediaStreamAudioRenderer> GetAudioRenderer(
      const blink::WebMediaStream& web_stream,
      int render_frame_id,
      const std::string& device_id) override;
};

}  // namespace content

#endif  // CONTENT_SHELL_RENDERER_LAYOUT_TEST_TEST_MEDIA_STREAM_RENDERER_FACTORY_H_
