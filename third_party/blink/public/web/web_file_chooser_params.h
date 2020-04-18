/*
 * Copyright (C) 2010, 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FILE_CHOOSER_PARAMS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FILE_CHOOSER_PARAMS_H_

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_file_chooser_completion.h"

namespace blink {

struct WebFileChooserParams {
  // If |multiSelect| is true, the dialog allows the user to select multiple
  // files.
  bool multi_select;
  // If |directory| is true, the dialog allows the user to select a directory.
  bool directory;
  // If |saveAs| is true, the dialog allows the user to select a possibly
  // non-existent file. This can be used for a "Save As" dialog.
  bool save_as;
  // |title| is the title for a file chooser dialog. It can be an empty string.
  WebString title;
  // This contains MIME type strings such as "audio/*" "text/plain" or file
  // extensions beginning with a period (.) such as ".mp3" ".txt".
  // The dialog may restrict selectable files to files with the specified MIME
  // types or file extensions.
  // This list comes from an 'accept' attribute value of an INPUT element, and
  // it contains only lower-cased MIME type strings and file extensions.
  WebVector<WebString> accept_types;
  // |selectedFiles| has filenames which a file upload control already selected.
  // A WebViewClient implementation may ask a user to select
  //  - removing a file from the selected files,
  //  - appending other files, or
  //  - replacing with other files
  // before opening a file chooser dialog.
  WebVector<WebString> selected_files;
  // See http://www.w3.org/TR/html-media-capture/ for the semantics of the
  // capture attribute. If |useMediaCapture| is true, the media types
  // indicated in |acceptTypes| should be obtained from the device's
  // environment using a media capture mechanism. |capture| is deprecated and
  // provided for compatibility reasons.
  WebString capture;
  bool use_media_capture;
  // Whether WebFileChooserCompletion needs local paths or not. If the result
  // of file chooser is handled by the implementation of
  // WebFileChooserCompletion that can handle files without local paths,
  // 'false' should be specified to the flag.
  bool need_local_path;
  // If non-empty, represents the URL of the requestor if the request was
  // initiated by a document.
  WebURL requestor;

  WebFileChooserParams()
      : multi_select(false),
        directory(false),
        save_as(false),
        use_media_capture(false),
        need_local_path(true) {}
};

}  // namespace blink

#endif
