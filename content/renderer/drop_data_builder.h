// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DROP_DATA_BUILDER_H_
#define CONTENT_RENDERER_DROP_DATA_BUILDER_H_

namespace blink {
class WebDragData;
}

namespace content {
struct DropData;

class DropDataBuilder {
 public:
  static DropData Build(const blink::WebDragData& data);
};

}  // namespace content

#endif  // CONTENT_RENDERER_DROP_DATA_BUILDER_H_
