// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_web_view_factory.h"

#include "chromecast/browser/cast_web_view_default.h"
#include "chromecast/chromecast_buildflags.h"

#if BUILDFLAG(ENABLE_CHROMECAST_EXTENSIONS)
#include "chromecast/browser/cast_web_view_extension.h"
#endif

namespace chromecast {

CastWebViewFactory::CastWebViewFactory(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(browser_context_);
}

CastWebViewFactory::~CastWebViewFactory() {}

std::unique_ptr<CastWebView> CastWebViewFactory::CreateWebView(
    const CastWebView::CreateParams& params,
    CastWebContentsManager* web_contents_manager,
    scoped_refptr<content::SiteInstance> site_instance,
    const extensions::Extension* extension,
    const GURL& initial_url) {
#if BUILDFLAG(ENABLE_CHROMECAST_EXTENSIONS)
  if (extension) {
    return std::make_unique<CastWebViewExtension>(
        params, browser_context_, site_instance, extension, initial_url);
  }
#endif
  return std::make_unique<CastWebViewDefault>(params, web_contents_manager,
                                              browser_context_, site_instance);
}

}  // namespace chromecast
