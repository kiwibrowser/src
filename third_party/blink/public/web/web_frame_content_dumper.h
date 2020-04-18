// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_CONTENT_DUMPER_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_CONTENT_DUMPER_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

class WebLocalFrame;
class WebView;
class WebString;

// Functions in this class should only be used for
// testing purposes.
// The exceptions to this rule are tracked in http://crbug.com/585164.
class WebFrameContentDumper {
 public:
  // Control of layoutTreeAsText output
  enum LayoutAsTextControl {
    kLayoutAsTextNormal = 0,
    kLayoutAsTextDebug = 1 << 0,
    kLayoutAsTextPrinting = 1 << 1,
    kLayoutAsTextWithLineTrees = 1 << 2
  };
  typedef unsigned LayoutAsTextControls;

  // Returns the contents of this frame as a string.  If the text is
  // longer than maxChars, it will be clipped to that length.
  //
  // If there is room, subframe text will be recursively appended. Each
  // frame will be separated by an empty line.
  // TODO(dglazkov): WebFrameContentDumper should only be used for
  // testing purposes and this function is being deprecated.
  // Don't add new callsites, please.
  // See http://crbug.com/585164 for details.
  BLINK_EXPORT static WebString DeprecatedDumpFrameTreeAsText(WebLocalFrame*,
                                                              size_t max_chars);

  // Dumps the contents of of a WebView as text, starting from the main
  // frame and recursively appending every subframe, separated by an
  // empty line.
  BLINK_EXPORT static WebString DumpWebViewAsText(WebView*, size_t max_chars);

  // Returns HTML text for the contents of this frame, generated
  // from the DOM.
  BLINK_EXPORT static WebString DumpAsMarkup(WebLocalFrame*);

  // Returns a text representation of the render tree.  This method is used
  // to support layout tests.
  BLINK_EXPORT static WebString DumpLayoutTreeAsText(
      WebLocalFrame*,
      LayoutAsTextControls to_show = kLayoutAsTextNormal);
};

}  // namespace blink

#endif
