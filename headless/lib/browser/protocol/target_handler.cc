// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/protocol/target_handler.h"

#include "build/build_config.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_browser_impl.h"
#include "ui/gfx/geometry/size.h"

namespace headless {
namespace protocol {

TargetHandler::TargetHandler(base::WeakPtr<HeadlessBrowserImpl> browser)
    : DomainHandler(Target::Metainfo::domainName, browser) {}

TargetHandler::~TargetHandler() = default;

void TargetHandler::Wire(UberDispatcher* dispatcher) {
  Target::Dispatcher::wire(dispatcher, this);
}

Response TargetHandler::CreateTarget(const std::string& url,
                                     Maybe<int> width,
                                     Maybe<int> height,
                                     Maybe<std::string> context_id,
                                     Maybe<bool> enable_begin_frame_control,
                                     std::string* out_target_id) {
#if defined(OS_MACOSX)
  if (enable_begin_frame_control.fromMaybe(false))
    return Response::Error("BeginFrameControl is not supported on MacOS yet");
#endif

  HeadlessBrowserContext* context;
  if (context_id.isJust()) {
    context = browser()->GetBrowserContextForId(context_id.fromJust());
    if (!context)
      return Response::InvalidParams("browserContextId");
  } else {
    context = browser()->GetDefaultBrowserContext();
    if (!context) {
      return Response::Error(
          "You specified no |browserContextId|, but "
          "there is no default browser context set on "
          "HeadlessBrowser");
    }
  }

  HeadlessWebContentsImpl* web_contents_impl = HeadlessWebContentsImpl::From(
      context->CreateWebContentsBuilder()
          .SetInitialURL(GURL(url))
          .SetWindowSize(gfx::Size(
              width.fromMaybe(browser()->options()->window_size.width()),
              height.fromMaybe(browser()->options()->window_size.height())))
          .SetEnableBeginFrameControl(
              enable_begin_frame_control.fromMaybe(false))
          .Build());

  *out_target_id = web_contents_impl->GetDevToolsAgentHostId();
  return Response::OK();
}

Response TargetHandler::CloseTarget(const std::string& target_id,
                                    bool* out_success) {
  HeadlessWebContents* web_contents =
      browser()->GetWebContentsForDevToolsAgentHostId(target_id);
  *out_success = false;
  if (web_contents) {
    web_contents->Close();
    *out_success = true;
  }
  return Response::OK();
}

Response TargetHandler::CreateBrowserContext(std::string* out_context_id) {
  auto builder = browser()->CreateBrowserContextBuilder();
  builder.SetIncognitoMode(true);
  HeadlessBrowserContext* browser_context = builder.Build();

  *out_context_id = browser_context->Id();
  return Response::OK();
}

Response TargetHandler::DisposeBrowserContext(const std::string& context_id) {
  HeadlessBrowserContext* context =
      browser()->GetBrowserContextForId(context_id);

  if (!context)
    return Response::InvalidParams("browserContextId");

  std::vector<HeadlessWebContents*> web_contents = context->GetAllWebContents();
  while (!web_contents.empty()) {
    for (auto* wc : web_contents)
      wc->Close();
    // Since HeadlessWebContents::Close spawns a nested run loop to await
    // closing, new web_contents could be opened. We need to re-query pages and
    // close them too.
    web_contents = context->GetAllWebContents();
  }
  context->Close();
  return Response::OK();
}

Response TargetHandler::GetBrowserContexts(
    std::unique_ptr<protocol::Array<protocol::String>>* browser_context_ids) {
  *browser_context_ids = std::make_unique<protocol::Array<protocol::String>>();
  for (auto* context : browser()->GetAllBrowserContexts()) {
    if (context != browser()->GetDefaultBrowserContext())
      (*browser_context_ids)->addItem(context->Id());
  }
  return Response::OK();
}

}  // namespace protocol
}  // namespace headless
