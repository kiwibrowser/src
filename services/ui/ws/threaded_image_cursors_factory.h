// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_THREADED_IMAGE_CURSORS_FACTORY_H_
#define SERVICES_UI_WS_THREADED_IMAGE_CURSORS_FACTORY_H_

#include <memory>

namespace ui {
namespace ws {
class ThreadedImageCursors;

class ThreadedImageCursorsFactory {
 public:
  virtual ~ThreadedImageCursorsFactory() {}
  virtual std::unique_ptr<ThreadedImageCursors> CreateCursors() = 0;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_THREADED_IMAGE_CURSORS_FACTORY_H_
