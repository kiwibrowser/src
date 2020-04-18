// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/web_request/chrome_extension_web_request_event_router_delegate.h"

#include "chrome/browser/extensions/extension_action_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"

namespace {

void NotifyWebRequestWithheldOnUI(int render_process_id,
                                  int render_frame_id,
                                  const std::string& extension_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // Track down the ExtensionActionRunner and the extension. Since this is
  // asynchronous, we could hit a null anywhere along the path.
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!rfh)
    return;
  // We don't count subframe blocked actions as yet, since there's no way to
  // surface this to the user. Ignore these (which is also what we do for
  // content scripts).
  if (rfh->GetParent())
    return;
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;
  extensions::ExtensionActionRunner* runner =
      extensions::ExtensionActionRunner::GetForWebContents(web_contents);
  if (!runner)
    return;

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(web_contents->GetBrowserContext())
          ->enabled_extensions()
          .GetByID(extension_id);
  if (!extension)
    return;

  runner->OnWebRequestBlocked(extension);
}

}  // namespace

ChromeExtensionWebRequestEventRouterDelegate::
    ChromeExtensionWebRequestEventRouterDelegate() {
}

ChromeExtensionWebRequestEventRouterDelegate::
    ~ChromeExtensionWebRequestEventRouterDelegate() {
}

void ChromeExtensionWebRequestEventRouterDelegate::NotifyWebRequestWithheld(
    int render_process_id,
    int render_frame_id,
    const std::string& extension_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&NotifyWebRequestWithheldOnUI, render_process_id,
                     render_frame_id, extension_id));
}
