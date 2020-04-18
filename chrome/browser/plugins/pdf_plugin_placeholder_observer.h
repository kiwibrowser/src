// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PDF_PLUGIN_PLACEHOLDER_OBSERVER_H_
#define CHROME_BROWSER_PLUGINS_PDF_PLUGIN_PLACEHOLDER_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Separate class from PluginObserver, since PluginObserver is created only when
// ENABLE_PLUGINS is true. PDFPluginPlaceholderObserver should also run on
// Android, where ENABLE_PLUGINS is false.
class PDFPluginPlaceholderObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PDFPluginPlaceholderObserver> {
 public:
  ~PDFPluginPlaceholderObserver() override;

  // content::WebContentsObserver:
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  friend class content::WebContentsUserData<PDFPluginPlaceholderObserver>;

  explicit PDFPluginPlaceholderObserver(content::WebContents* web_contents);

  // Message handlers:
  void OnOpenPDF(content::RenderFrameHost* render_frame_host, const GURL& url);

  DISALLOW_COPY_AND_ASSIGN(PDFPluginPlaceholderObserver);
};

#endif  // CHROME_BROWSER_PLUGINS_PDF_PLUGIN_PLACEHOLDER_OBSERVER_H_
