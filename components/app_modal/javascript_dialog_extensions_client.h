// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_APP_MODAL_JAVASCRIPT_DIALOG_EXTENSIONS_CLIENT_H_
#define COMPONENTS_APP_MODAL_JAVASCRIPT_DIALOG_EXTENSIONS_CLIENT_H_

#include <string>

class GURL;

namespace content {
class WebContents;
}

namespace app_modal {

// A client interface to access and control extensions/apps
// that opened a JavaScript dialog.
class JavaScriptDialogExtensionsClient {
 public:
  virtual ~JavaScriptDialogExtensionsClient() {}

  // Called when the extension associated with |web_contents| opened
  // a dialog.
  virtual void OnDialogOpened(content::WebContents* web_contents) = 0;

  // Called when a dialog created by the extension associated with
  // |web_contents| is closed.
  virtual void OnDialogClosed(content::WebContents* web_contents) = 0;

  // Sets the name of the extensions associated with the
  // |web_contents| in the |name_out| if there is one, returning true;
  // returns false otherwise.
  virtual bool GetExtensionName(content::WebContents* web_contents,
                                const GURL& alerting_frame_url,
                                std::string* name_out) = 0;
};

}  // namespace app_modal

#endif  // COMPONENTS_APP_MODAL_JAVASCRIPT_DIALOG_EXTENSIONS_CLIENT_H_
