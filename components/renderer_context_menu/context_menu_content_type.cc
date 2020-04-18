// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/renderer_context_menu/context_menu_content_type.h"

#include "base/bind.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "printing/buildflags/buildflags.h"
#include "third_party/blink/public/web/web_context_menu_data.h"

using blink::WebContextMenuData;
using content::WebContents;

namespace {

bool IsDevToolsURL(const GURL& url) {
  return url.SchemeIs(content::kChromeDevToolsScheme);
}

bool DefaultIsInternalResourcesURL(const GURL& url) {
  return url.SchemeIs(content::kChromeUIScheme);
}

}  // namespace

ContextMenuContentType::ContextMenuContentType(
    content::WebContents* web_contents,
    const content::ContextMenuParams& params,
    bool supports_custom_items)
    : params_(params),
      source_web_contents_(web_contents),
      supports_custom_items_(supports_custom_items),
      internal_resources_url_checker_(
          base::Bind(&DefaultIsInternalResourcesURL)) {
}

ContextMenuContentType::~ContextMenuContentType() {
}

bool ContextMenuContentType::SupportsGroup(int group) {
  const bool has_selection = !params_.selection_text.empty();

  if (supports_custom_items_ && !params_.custom_items.empty()) {
    if (group == ITEM_GROUP_CUSTOM)
      return true;

    if (!has_selection) {
      // For menus with custom items, if there is no selection, we do not
      // add items other than developer items. And for Pepper menu, don't even
      // add developer items.
      if (!params_.custom_context.is_pepper_menu)
        return group == ITEM_GROUP_DEVELOPER;

      return false;
    }

    // If there's a selection when there are custom items, fall through to
    // adding the normal ones after the custom ones.
  }

  if (IsDevToolsURL(params_.page_url)) {
    // DevTools mostly provides custom context menu and uses
    // only the following default options.
    if (group != ITEM_GROUP_CUSTOM &&
        group != ITEM_GROUP_EDITABLE &&
        group != ITEM_GROUP_COPY &&
        group != ITEM_GROUP_DEVELOPER) {
      return false;
    }
  }

  return SupportsGroupInternal(group);
}

bool ContextMenuContentType::SupportsGroupInternal(int group) {
  const bool has_link = !params_.unfiltered_link_url.is_empty();
  const bool has_selection = !params_.selection_text.empty();
  const bool is_password =
      params_.input_field_type == WebContextMenuData::kInputFieldTypePassword;

  switch (group) {
    case ITEM_GROUP_CUSTOM:
      return supports_custom_items_ && !params_.custom_items.empty();

    case ITEM_GROUP_PAGE: {
      bool is_candidate =
          params_.media_type == WebContextMenuData::kMediaTypeNone &&
          !has_link && !params_.is_editable && !has_selection;

      if (!is_candidate && params_.page_url.is_empty())
        DCHECK(params_.frame_url.is_empty());

      return is_candidate && !params_.page_url.is_empty() &&
          !IsInternalResourcesURL(params_.page_url);
    }

    case ITEM_GROUP_FRAME: {
      bool page_group_supported = SupportsGroupInternal(ITEM_GROUP_PAGE);
      return page_group_supported && !params_.frame_url.is_empty() &&
          !IsInternalResourcesURL(params_.page_url);
    }

    case ITEM_GROUP_LINK:
      return has_link;

    case ITEM_GROUP_MEDIA_IMAGE:
      return params_.media_type == WebContextMenuData::kMediaTypeImage;

    case ITEM_GROUP_SEARCHWEBFORIMAGE:
      // Image menu items imply search web for image item.
      return SupportsGroupInternal(ITEM_GROUP_MEDIA_IMAGE);

    case ITEM_GROUP_MEDIA_VIDEO:
      return params_.media_type == WebContextMenuData::kMediaTypeVideo;

    case ITEM_GROUP_MEDIA_AUDIO:
      return params_.media_type == WebContextMenuData::kMediaTypeAudio;

    case ITEM_GROUP_MEDIA_CANVAS:
      return params_.media_type == WebContextMenuData::kMediaTypeCanvas;

    case ITEM_GROUP_MEDIA_PLUGIN:
      return params_.media_type == WebContextMenuData::kMediaTypePlugin;

    case ITEM_GROUP_MEDIA_FILE:
#if defined(WEBCONTEXT_MEDIATYPEFILE_DEFINED)
      return params_.media_type == WebContextMenuData::kMediaTypeFile;
#else
      return false;
#endif

    case ITEM_GROUP_EDITABLE:
      return params_.is_editable;

    case ITEM_GROUP_COPY:
      return !params_.is_editable && has_selection;

    case ITEM_GROUP_SEARCH_PROVIDER:
      return has_selection && !is_password;

    case ITEM_GROUP_PRINT: {
      // Image menu items also imply print items.
      return has_selection || SupportsGroupInternal(ITEM_GROUP_MEDIA_IMAGE);
    }

    case ITEM_GROUP_ALL_EXTENSION:
      return true;

    case ITEM_GROUP_CURRENT_EXTENSION:
      return false;

    case ITEM_GROUP_DEVELOPER:
      return true;

    case ITEM_GROUP_DEVTOOLS_UNPACKED_EXT:
      return false;

    case ITEM_GROUP_PRINT_PREVIEW:
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
      return true;
#else
      return false;
#endif

    case ITEM_GROUP_PASSWORD:
      return params_.input_field_type ==
             blink::WebContextMenuData::kInputFieldTypePassword;

    default:
      NOTREACHED();
      return false;
  }
}

bool ContextMenuContentType::IsInternalResourcesURL(const GURL& url) {
  return internal_resources_url_checker_.Run(url);
}
