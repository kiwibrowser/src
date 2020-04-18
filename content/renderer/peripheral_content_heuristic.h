// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PERIPHERAL_CONTENT_HEURISTIC_H_
#define CONTENT_RENDERER_PERIPHERAL_CONTENT_HEURISTIC_H_

#include <set>

#include "content/common/content_export.h"
#include "content/public/renderer/render_frame.h"
#include "url/origin.h"

namespace gfx {
class Size;
}

namespace content {

class CONTENT_EXPORT PeripheralContentHeuristic {
 public:
  // Returns true if this content should have power saver enabled.
  //
  // Power Saver is enabled for content that are cross-origin and
  // heuristically determined to be not essential to the web page content.
  //
  // Content is defined to be cross-origin when the source's origin differs
  //  from the top level frame's origin. For example:
  //  - Cross-origin:  a.com -> b.com/plugin.swf
  //  - Cross-origin:  a.com -> b.com/iframe.html -> b.com/plugin.swf
  //  - Same-origin:   a.com -> b.com/iframe-to-a.html -> a.com/plugin.swf
  //
  // |origin_whitelist| is the whitelist of content origins.
  //
  // |main_frame_origin| is the origin of the main frame.
  //
  // |content_origin| is the origin of the content e.g. plugin or video source.
  //
  // |unobscured_size| is in zoom and device scale independent logical pixels.
  static RenderFrame::PeripheralContentStatus GetPeripheralStatus(
      const std::set<url::Origin>& origin_whitelist,
      const url::Origin& main_frame_origin,
      const url::Origin& content_origin,
      const gfx::Size& unobscured_size);

  // Returns true if content is considered "large", and thus essential.
  // |unobscured_size| is in zoom and device scale independent logical pixels.
  static bool IsLargeContent(const gfx::Size& unobscured_size);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PERIPHERAL_CONTENT_HEURISTIC_H_
