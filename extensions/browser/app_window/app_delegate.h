// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_APP_WINDOW_APP_DELEGATE_H_
#define EXTENSIONS_BROWSER_APP_WINDOW_APP_DELEGATE_H_

#include "base/callback_forward.h"
#include "content/public/common/media_stream_request.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/window_open_disposition.h"

namespace content {
class BrowserContext;
class ColorChooser;
class RenderFrameHost;
class RenderViewHost;
class WebContents;
struct FileChooserParams;
struct OpenURLParams;
}

namespace gfx {
class Rect;
class Size;
}

namespace extensions {

class Extension;

// Interface to give packaged apps access to services in the browser, for things
// like handling links and showing UI prompts to the user.
class AppDelegate {
 public:
  virtual ~AppDelegate() {}

  // General initialization.
  virtual void InitWebContents(content::WebContents* web_contents) = 0;
  virtual void RenderViewCreated(content::RenderViewHost* render_view_host) = 0;

  // Resizes WebContents.
  virtual void ResizeWebContents(content::WebContents* web_contents,
                                 const gfx::Size& size) = 0;

  // Link handling.
  virtual content::WebContents* OpenURLFromTab(
      content::BrowserContext* context,
      content::WebContents* source,
      const content::OpenURLParams& params) = 0;
  virtual void AddNewContents(
      content::BrowserContext* context,
      std::unique_ptr<content::WebContents> new_contents,
      WindowOpenDisposition disposition,
      const gfx::Rect& initial_rect,
      bool user_gesture) = 0;

  // Feature support.
  virtual content::ColorChooser* ShowColorChooser(
      content::WebContents* web_contents,
      SkColor initial_color) = 0;
  virtual void RunFileChooser(content::RenderFrameHost* render_frame_host,
                              const content::FileChooserParams& params) = 0;
  virtual void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback,
      const Extension* extension) = 0;
  virtual bool CheckMediaAccessPermission(
      content::RenderFrameHost* render_frame_host,
      const GURL& security_origin,
      content::MediaStreamType type,
      const Extension* extension) = 0;
  virtual int PreferredIconSize() const = 0;

  // Web contents modal dialog support.
  virtual void SetWebContentsBlocked(content::WebContents* web_contents,
                                     bool blocked) = 0;
  virtual bool IsWebContentsVisible(content::WebContents* web_contents) = 0;

  // |callback| will be called when the process is about to terminate.
  virtual void SetTerminatingCallback(const base::Closure& callback) = 0;

  // Called when the app is hidden or shown.
  virtual void OnHide() = 0;
  virtual void OnShow() = 0;

  // Called when app web contents finishes focus traversal - gives the delegate
  // a chance to handle the focus change.
  // Return whether focus has been handled.
  virtual bool TakeFocus(content::WebContents* web_contents, bool reverse) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_APP_WINDOW_APP_DELEGATE_H_
