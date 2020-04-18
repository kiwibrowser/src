// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_RESOURCES_PROVIDER_H_
#define CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_RESOURCES_PROVIDER_H_

namespace content {
class WebUIDataSource;
}

namespace media_router {

// Adds the resources needed by Media Router to |html_source|.
void AddMediaRouterUIResources(content::WebUIDataSource* html_source);

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_RESOURCES_PROVIDER_H_
