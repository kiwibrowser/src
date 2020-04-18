// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/context_menu_content_type_factory.h"

#include "base/bind.h"
#include "chrome/common/url_constants.h"
#include "components/renderer_context_menu/context_menu_content_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/guest_view/web_view/context_menu_content_type_web_view.h"
#include "chrome/browser/renderer_context_menu/context_menu_content_type_app_mode.h"
#include "chrome/browser/renderer_context_menu/context_menu_content_type_extension_popup.h"
#include "chrome/browser/renderer_context_menu/context_menu_content_type_panel.h"
#include "chrome/browser/renderer_context_menu/context_menu_content_type_platform_app.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/extension.h"
#endif

#if defined(OS_CHROMEOS)
#include "components/session_manager/core/session_manager.h"
#endif

namespace {

bool CheckInternalResourcesURL(const GURL& url) {
  return url.SchemeIs(content::kChromeUIScheme) &&
         (url.host_piece() == chrome::kChromeUISyncResourcesHost);
}

bool IsUserSessionBlocked() {
#if defined(OS_CHROMEOS)
  if (session_manager::SessionManager::Get() &&
      session_manager::SessionManager::Get()->IsUserSessionBlocked()) {
    return true;
  }
#endif
  return false;
}

// Context menu content with no supported groups.
class NullContextMenuContentType : public ContextMenuContentType {
 public:
  NullContextMenuContentType(content::WebContents* web_contents,
                             const content::ContextMenuParams& params)
      : ContextMenuContentType(web_contents, params, false) {}
  ~NullContextMenuContentType() override = default;

  bool SupportsGroup(int group) override { return false; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NullContextMenuContentType);
};

}  // namespace

ContextMenuContentTypeFactory::ContextMenuContentTypeFactory() {
}

ContextMenuContentTypeFactory::~ContextMenuContentTypeFactory() {
}

// static.
ContextMenuContentType* ContextMenuContentTypeFactory::Create(
    content::WebContents* web_contents,
    const content::ContextMenuParams& params) {
  if (IsUserSessionBlocked())
    return new NullContextMenuContentType(web_contents, params);

  return SetInternalResourcesURLChecker(CreateInternal(web_contents, params));
}

// static.
ContextMenuContentType*
ContextMenuContentTypeFactory::SetInternalResourcesURLChecker(
    ContextMenuContentType* content_type) {
  content_type->set_internal_resources_url_checker(
      base::Bind(&CheckInternalResourcesURL));
  return content_type;
}

// static
ContextMenuContentType* ContextMenuContentTypeFactory::CreateInternal(
    content::WebContents* web_contents,
    const content::ContextMenuParams& params) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (chrome::IsRunningInForcedAppMode())
    return new ContextMenuContentTypeAppMode(web_contents, params);

  if (extensions::WebViewGuest::FromWebContents(web_contents))
    return new ContextMenuContentTypeWebView(web_contents, params);

  const extensions::ViewType view_type = extensions::GetViewType(web_contents);

  if (view_type == extensions::VIEW_TYPE_APP_WINDOW)
    return new ContextMenuContentTypePlatformApp(web_contents, params);

  if (view_type == extensions::VIEW_TYPE_EXTENSION_POPUP)
    return new ContextMenuContentTypeExtensionPopup(web_contents, params);

  if (view_type == extensions::VIEW_TYPE_PANEL)
    return new ContextMenuContentTypePanel(web_contents, params);
#endif

  return new ContextMenuContentType(web_contents, params, true);
}
