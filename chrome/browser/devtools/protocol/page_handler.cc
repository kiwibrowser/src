// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/protocol/page_handler.h"

#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"

PageHandler::PageHandler(content::WebContents* web_contents,
                         protocol::UberDispatcher* dispatcher)
    : content::WebContentsObserver(web_contents) {
  DCHECK(web_contents);
  protocol::Page::Dispatcher::wire(dispatcher, this);
}

PageHandler::~PageHandler() {
  ToggleAdBlocking(false /* enabled */);
}

void PageHandler::ToggleAdBlocking(bool enabled) {
  if (!web_contents())
    return;
  if (auto* client =
          ChromeSubresourceFilterClient::FromWebContents(web_contents())) {
    client->ToggleForceActivationInCurrentWebContents(enabled);
  }
}

protocol::Response PageHandler::Enable() {
  enabled_ = true;
  // Do not mark the command as handled. Let it fall through instead, so that
  // the handler in content gets a chance to process the command.
  return protocol::Response::FallThrough();
}

protocol::Response PageHandler::Disable() {
  enabled_ = false;
  ToggleAdBlocking(false /* enable */);
  // Do not mark the command as handled. Let it fall through instead, so that
  // the handler in content gets a chance to process the command.
  return protocol::Response::FallThrough();
}

protocol::Response PageHandler::SetAdBlockingEnabled(bool enabled) {
  if (!enabled_)
    return protocol::Response::Error("Page domain is disabled.");
  ToggleAdBlocking(enabled);
  return protocol::Response::OK();
}
