// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_PIXEL_DUMP_H_
#define CONTENT_SHELL_TEST_RUNNER_PIXEL_DUMP_H_

#include "base/callback_forward.h"

class SkBitmap;

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace test_runner {

// Asks |web_frame|'s widget to dump its pixels and calls |callback| with the
// result.
void DumpPixelsAsync(blink::WebLocalFrame* web_frame,
                     float device_scale_factor_for_test,
                     base::OnceCallback<void(const SkBitmap&)> callback);

// Asks |web_frame| to print itself and calls |callback| with the result.
void PrintFrameAsync(blink::WebLocalFrame* web_frame,
                     base::OnceCallback<void(const SkBitmap&)> callback);

// Captures the current selection bounds rect of |web_frame| and creates a
// callback that will draw the rect (if any) on top of the |original_bitmap|,
// before passing |bitmap_with_selection_bounds_rect| to the
// |original_callback|.  Selection bounds rect is the rect enclosing the
// (possibly transformed) selection.
base::OnceCallback<void(const SkBitmap& original_bitmap)>
CreateSelectionBoundsRectDrawingCallback(
    blink::WebLocalFrame* web_frame,
    base::OnceCallback<void(const SkBitmap& bitmap_with_selection_bounds_rect)>
        original_callback);

// Copy to clipboard the image present at |x|, |y| coordinates in |web_frame|
// and pass the captured image to |callback|.
void CopyImageAtAndCapturePixels(
    blink::WebLocalFrame* web_frame,
    int x,
    int y,
    base::OnceCallback<void(const SkBitmap&)> callback);

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_PIXEL_DUMP_H_
