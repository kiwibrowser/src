// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/chrome_impl.h"

#include <stddef.h>
#include <utility>

#include "base/values.h"
#include "chrome/test/chromedriver/chrome/devtools_client.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"
#include "chrome/test/chromedriver/chrome/devtools_http_client.h"
#include "chrome/test/chromedriver/chrome/page_load_strategy.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/chrome/web_view_impl.h"

ChromeImpl::~ChromeImpl() {
}

Status ChromeImpl::GetAsDesktop(ChromeDesktopImpl** desktop) {
  return Status(kUnknownError, "operation unsupported");
}

const BrowserInfo* ChromeImpl::GetBrowserInfo() const {
  return devtools_http_client_->browser_info();
}

bool ChromeImpl::HasCrashedWebView() {
  for (WebViewList::iterator it = web_views_.begin();
       it != web_views_.end(); ++it) {
    if ((*it)->WasCrashed())
      return true;
  }
  return false;
}

Status ChromeImpl::GetWebViewIdForFirstTab(std::string* web_view_id,
                                           bool w3c_compliant) {
  WebViewsInfo views_info;
  Status status = devtools_http_client_->GetWebViewsInfo(&views_info);
  if (status.IsError())
    return status;
  UpdateWebViews(views_info, w3c_compliant);
  for (size_t i = 0; i < views_info.GetSize(); ++i) {
    const WebViewInfo& view = views_info.Get(i);
    if (view.type == WebViewInfo::kPage) {
      *web_view_id = view.id;
      return Status(kOk);
    }
  }
  return Status(kUnknownError, "unable to discover open window in chrome");
}

Status ChromeImpl::GetWebViewIds(std::list<std::string>* web_view_ids,
                                 bool w3c_compliant) {
  WebViewsInfo views_info;
  Status status = devtools_http_client_->GetWebViewsInfo(&views_info);
  if (status.IsError())
    return status;
  UpdateWebViews(views_info, w3c_compliant);
  std::list<std::string> web_view_ids_tmp;
  for (WebViewList::const_iterator web_view_iter = web_views_.begin();
       web_view_iter != web_views_.end(); ++web_view_iter) {
    web_view_ids_tmp.push_back((*web_view_iter)->GetId());
  }
  web_view_ids->swap(web_view_ids_tmp);
  return Status(kOk);
}

void ChromeImpl::UpdateWebViews(const WebViewsInfo& views_info,
                                bool w3c_compliant) {
  // Check if some web views are closed (or in the case of background pages,
  // become inactive).
  WebViewList::iterator it = web_views_.begin();
  while (it != web_views_.end()) {
    const WebViewInfo* view = views_info.GetForId((*it)->GetId());
    if (!view || view->IsInactiveBackgroundPage()) {
      it = web_views_.erase(it);
    } else {
      ++it;
    }
  }

  // Check for newly-opened web views.
  for (size_t i = 0; i < views_info.GetSize(); ++i) {
    const WebViewInfo& view = views_info.Get(i);
    if (devtools_http_client_->IsBrowserWindow(view) &&
        !view.IsInactiveBackgroundPage()) {
      bool found = false;
      for (WebViewList::const_iterator web_view_iter = web_views_.begin();
           web_view_iter != web_views_.end(); ++web_view_iter) {
        if ((*web_view_iter)->GetId() == view.id) {
          found = true;
          break;
        }
      }
      if (!found) {
        std::unique_ptr<DevToolsClient> client(
            devtools_http_client_->CreateClient(view.id));
        for (const auto& listener : devtools_event_listeners_)
          client->AddListener(listener.get());
        // OnConnected will fire when DevToolsClient connects later.
        CHECK(!page_load_strategy_.empty());
        web_views_.push_back(make_linked_ptr(new WebViewImpl(
            view.id, w3c_compliant, devtools_http_client_->browser_info(),
            std::move(client), devtools_http_client_->device_metrics(),
            page_load_strategy_)));
      }
    }
  }
}

Status ChromeImpl::GetWebViewById(const std::string& id, WebView** web_view) {
  for (WebViewList::iterator it = web_views_.begin();
       it != web_views_.end(); ++it) {
    if ((*it)->GetId() == id) {
      *web_view = (*it).get();
      return Status(kOk);
    }
  }
  return Status(kUnknownError, "web view not found");
}

Status ChromeImpl::CloseWebView(const std::string& id) {
  Status status = devtools_http_client_->CloseWebView(id);
  if (status.IsError())
    return status;
  for (WebViewList::iterator iter = web_views_.begin();
       iter != web_views_.end(); ++iter) {
    if ((*iter)->GetId() == id) {
      web_views_.erase(iter);
      break;
    }
  }
  return Status(kOk);
}

Status ChromeImpl::ActivateWebView(const std::string& id) {
  return devtools_http_client_->ActivateWebView(id);
}

Status ChromeImpl::SetAcceptInsecureCerts() {
  Status status = devtools_websocket_client_->ConnectIfNecessary();
  if (status.IsError())
    return status;

  base::DictionaryValue params;
  params.SetBoolean("ignore", true);
  // We ignore the status returned by this command - If it is an error, the
  // target likely doesn't yet support the command. In that case, we'll fall
  // back to --ignore-certificate-errors.
  // TODO(eseckler): Handle status once we remove support for
  // --ignore-certificate-errors.
  devtools_websocket_client_->SendCommand("Security.setIgnoreCertificateErrors",
                                          params);
  return Status(kOk);
}

bool ChromeImpl::IsMobileEmulationEnabled() const {
  return false;
}

bool ChromeImpl::HasTouchScreen() const {
  return false;
}

std::string ChromeImpl::page_load_strategy() const {
  CHECK(!page_load_strategy_.empty());
  return page_load_strategy_;
}

Status ChromeImpl::Quit() {
  Status status = QuitImpl();
  if (status.IsOk())
    quit_ = true;
  return status;
}

ChromeImpl::ChromeImpl(std::unique_ptr<DevToolsHttpClient> http_client,
                       std::unique_ptr<DevToolsClient> websocket_client,
                       std::vector<std::unique_ptr<DevToolsEventListener>>
                           devtools_event_listeners,
                       std::string page_load_strategy)
    : quit_(false),
      devtools_http_client_(std::move(http_client)),
      devtools_websocket_client_(std::move(websocket_client)),
      devtools_event_listeners_(std::move(devtools_event_listeners)),
      page_load_strategy_(page_load_strategy) {}
