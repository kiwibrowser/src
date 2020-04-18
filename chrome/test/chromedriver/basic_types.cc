// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/basic_types.h"

WebPoint::WebPoint() : x(0), y(0) {}

WebPoint::WebPoint(int x, int y) : x(x), y(y) {}

WebPoint::~WebPoint() {}

void WebPoint::Offset(int x_, int y_) {
  x += x_;
  y += y_;
}

WebSize::WebSize() : width(0), height(0) {}

WebSize::WebSize(int width, int height) : width(width), height(height) {}

WebSize::~WebSize() {}

WebRect::WebRect() : origin(0, 0), size(0, 0) {}

WebRect::WebRect(int x, int y, int width, int height)
    : origin(x, y), size(width, height) {}

WebRect::WebRect(const WebPoint& origin, const WebSize& size)
    : origin(origin), size(size) {}

WebRect::~WebRect() {}

int WebRect::X() const { return origin.x; }

int WebRect::Y() const { return origin.y; }

int WebRect::Width() const { return size.width; }

int WebRect::Height() const { return size.height; }
