// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/error_page_util.h"

#import <Foundation/Foundation.h>

#import "base/ios/ns_error_util.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "components/error_page/common/error.h"
#include "components/error_page/common/error_page_params.h"
#include "components/error_page/common/localized_error.h"
#include "components/grit/components_resources.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/net/protocol_handler_util.h"
#include "net/base/net_errors.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/scale_factor.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* GetErrorPage(NSError* error, bool is_post, bool is_off_the_record) {
  NSString* url_spec = error.userInfo[NSURLErrorFailingURLStringErrorKey];
  NSError* final_error = base::ios::GetFinalUnderlyingErrorFromError(error);
  if (!final_error)
    final_error = error;
  int net_error = net::ERR_FAILED;
  if ([final_error.domain isEqualToString:net::kNSErrorDomain]) {
    net_error = final_error.code;
    DCHECK_NE(0, net_error);
  } else {
    // This function may only be called with an NSError created with
    // web::NetErrorFromError.
    NOTREACHED();
  }

  base::DictionaryValue error_strings;
  error_page::LocalizedError::GetStrings(
      net_error, error_page::Error::kNetErrorDomain,
      GURL(base::SysNSStringToUTF16(url_spec)), is_post,
      /*stale_copy_in_cache=*/false,
      /*can_show_network_diagnostics_dialog=*/false, is_off_the_record,
      GetApplicationContext()->GetApplicationLocale(),
      /*params=*/nullptr, &error_strings);

  ui::ScaleFactor scale_factor =
      ui::ResourceBundle::GetSharedInstance().GetMaxScaleFactor();

  const base::StringPiece template_html(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
          IDR_NET_ERROR_HTML, scale_factor));
  if (template_html.empty())
    NOTREACHED() << "unable to load template. ID: " << IDR_NET_ERROR_HTML;
  return base::SysUTF8ToNSString(webui::GetTemplatesHtml(
      template_html, &error_strings, /*template_id=*/"t"));
}
