// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_WEB_VIEW_FACTORY_H_
#define CHROMECAST_BROWSER_CAST_WEB_VIEW_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromecast/browser/cast_web_view.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class SiteInstance;
}  // namespace content

namespace extensions {
class Extension;
}  // namespace extensions

namespace chromecast {

class CastWebContentsManager;

class CastWebViewFactory {
 public:
  explicit CastWebViewFactory(content::BrowserContext* browser_context);
  virtual ~CastWebViewFactory();

  virtual std::unique_ptr<CastWebView> CreateWebView(
      const CastWebView::CreateParams& params,
      CastWebContentsManager* web_contents_manager,
      scoped_refptr<content::SiteInstance> site_instance,
      const extensions::Extension* extension,
      const GURL& initial_url);

  content::BrowserContext* browser_context() const { return browser_context_; }

 private:
  content::BrowserContext* const browser_context_;

  DISALLOW_COPY_AND_ASSIGN(CastWebViewFactory);
};

}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_WEB_VIEW_FACTORY_H_
