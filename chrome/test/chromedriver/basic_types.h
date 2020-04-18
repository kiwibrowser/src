// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_BASIC_TYPES_H_
#define CHROME_TEST_CHROMEDRIVER_BASIC_TYPES_H_

struct WebPoint {
  WebPoint();
  WebPoint(int x, int y);
  ~WebPoint();

  void Offset(int x_, int y_);

  int x;
  int y;
};

struct WebSize {
  WebSize();
  WebSize(int width, int height);
  ~WebSize();

  int width;
  int height;
};

struct WebRect {
  WebRect();
  WebRect(int x, int y, int width, int height);
  WebRect(const WebPoint& origin, const WebSize& size);
  ~WebRect();

  int X() const;
  int Y() const;
  int Width() const;
  int Height() const;

  WebPoint origin;
  WebSize size;
};

#endif  // CHROME_TEST_CHROMEDRIVER_BASIC_TYPES_H_
