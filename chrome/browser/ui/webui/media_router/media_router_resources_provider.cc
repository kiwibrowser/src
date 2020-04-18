// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/media_router_resources_provider.h"

#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

void AddMainWebResources(content::WebUIDataSource* html_source) {
  html_source->AddResourcePath("media_router.js", IDR_MEDIA_ROUTER_JS);
  html_source->AddResourcePath("media_router_common.css",
                               IDR_MEDIA_ROUTER_COMMON_CSS);
  html_source->AddResourcePath("media_router.css",
                               IDR_MEDIA_ROUTER_CSS);
  html_source->AddResourcePath("media_router_data.js",
                               IDR_MEDIA_ROUTER_DATA_JS);
  html_source->AddResourcePath("media_router_ui_interface.js",
                               IDR_MEDIA_ROUTER_UI_INTERFACE_JS);
}

void AddPolymerElements(content::WebUIDataSource* html_source) {
  html_source->AddResourcePath(
      "icons/media_router_icons.html",
      IDR_MEDIA_ROUTER_ICONS_HTML);
  html_source->AddResourcePath(
      "elements/issue_banner/issue_banner.css",
      IDR_ISSUE_BANNER_CSS);
  html_source->AddResourcePath(
      "elements/issue_banner/issue_banner.html",
      IDR_ISSUE_BANNER_HTML);
  html_source->AddResourcePath(
      "elements/issue_banner/issue_banner.js",
      IDR_ISSUE_BANNER_JS);
  html_source->AddResourcePath(
      "elements/media_router_container/media_router_container.css",
      IDR_MEDIA_ROUTER_CONTAINER_CSS);
  html_source->AddResourcePath(
      "elements/media_router_container/media_router_container.html",
      IDR_MEDIA_ROUTER_CONTAINER_HTML);
  html_source->AddResourcePath(
      "elements/media_router_container/media_router_container.js",
      IDR_MEDIA_ROUTER_CONTAINER_JS);
  html_source->AddResourcePath(
      "elements/media_router_header/media_router_header.css",
      IDR_MEDIA_ROUTER_HEADER_CSS);
  html_source->AddResourcePath(
      "elements/media_router_header/media_router_header.html",
      IDR_MEDIA_ROUTER_HEADER_HTML);
  html_source->AddResourcePath(
      "elements/media_router_header/media_router_header.js",
      IDR_MEDIA_ROUTER_HEADER_JS);
  html_source->AddResourcePath(
      "elements/media_router_search_highlighter/"
      "media_router_search_highlighter.css",
      IDR_MEDIA_ROUTER_SEARCH_HIGHLIGHTER_CSS);
  html_source->AddResourcePath(
      "elements/media_router_search_highlighter/"
      "media_router_search_highlighter.html",
      IDR_MEDIA_ROUTER_SEARCH_HIGHLIGHTER_HTML);
  html_source->AddResourcePath(
      "elements/media_router_search_highlighter/"
      "media_router_search_highlighter.js",
      IDR_MEDIA_ROUTER_SEARCH_HIGHLIGHTER_JS);
  html_source->AddResourcePath("elements/route_controls/route_controls.css",
                               IDR_ROUTE_CONTROLS_CSS);
  html_source->AddResourcePath("elements/route_controls/route_controls.html",
                               IDR_ROUTE_CONTROLS_HTML);
  html_source->AddResourcePath("elements/route_controls/route_controls.js",
                               IDR_ROUTE_CONTROLS_JS);
  html_source->AddResourcePath("elements/route_details/route_details.css",
                               IDR_ROUTE_DETAILS_CSS);
  html_source->AddResourcePath(
      "elements/route_details/route_details.html",
      IDR_ROUTE_DETAILS_HTML);
  html_source->AddResourcePath(
      "elements/route_details/route_details.js",
      IDR_ROUTE_DETAILS_JS);
  html_source->AddResourcePath(
      "elements/media_router_container/pseudo_sink_search_state.js",
      IDR_PSEUDO_SINK_SEARCH_STATE_JS);
}

}  // namespace

namespace media_router {

void AddMediaRouterUIResources(content::WebUIDataSource* html_source) {
  AddMainWebResources(html_source);
  AddPolymerElements(html_source);
  html_source->SetDefaultResource(IDR_MEDIA_ROUTER_HTML);
}

}  // namespace media_router
