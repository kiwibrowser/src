// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSION_API_FRAME_ID_MAP_HELPER_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSION_API_FRAME_ID_MAP_HELPER_H_

#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/extension_api_frame_id_map.h"

namespace content {
class WebContents;
}  // namespace content

namespace extensions {

class ChromeExtensionApiFrameIdMapHelper
    : public ExtensionApiFrameIdMapHelper,
      public content::NotificationObserver {
 public:
  explicit ChromeExtensionApiFrameIdMapHelper(ExtensionApiFrameIdMap* owner);
  ~ChromeExtensionApiFrameIdMapHelper() override;

  // Populates the |tab_id| and |window_id| for the given |web_contents|.
  // Returns true on success.
  static bool PopulateTabData(content::WebContents* web_contents,
                              int* tab_id,
                              int* window_id);

 private:
  // ChromeExtensionApiFrameIdMapHelper:
  void PopulateTabData(content::RenderFrameHost* rfh,
                       int* tab_id_out,
                       int* window_id_out) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  ExtensionApiFrameIdMap* owner_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ChromeExtensionApiFrameIdMapHelper);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSION_API_FRAME_ID_MAP_HELPER_H_
