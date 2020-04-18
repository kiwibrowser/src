// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/printing/chrome_print_render_frame_helper_delegate.h"

#include <vector>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/prerender.mojom.h"
#include "chrome/common/url_constants.h"
#include "chrome/renderer/prerender/prerender_helper.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/extensions/extension_constants.h"
#include "extensions/common/constants.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

ChromePrintRenderFrameHelperDelegate::ChromePrintRenderFrameHelperDelegate() =
    default;

ChromePrintRenderFrameHelperDelegate::~ChromePrintRenderFrameHelperDelegate() =
    default;

bool ChromePrintRenderFrameHelperDelegate::CancelPrerender(
    content::RenderFrame* render_frame) {
  if (!prerender::PrerenderHelper::IsPrerendering(render_frame))
    return false;

  chrome::mojom::PrerenderCancelerPtr canceler;
  render_frame->GetRemoteInterfaces()->GetInterface(&canceler);
  canceler->CancelPrerenderForPrinting();
  return true;
}

// Return the PDF object element if |frame| is the out of process PDF extension.
blink::WebElement ChromePrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  GURL url = frame->GetDocument().Url();
  bool inside_print_preview = url.GetOrigin() == chrome::kChromeUIPrintURL;
  bool inside_pdf_extension =
      url.SchemeIs(extensions::kExtensionScheme) &&
      url.host_piece() == extension_misc::kPdfExtensionId;
  if (inside_print_preview || inside_pdf_extension) {
    // <object> with id="plugin" is created in
    // chrome/browser/resources/pdf/pdf.js.
    auto plugin_element = frame->GetDocument().GetElementById("plugin");
    if (!plugin_element.IsNull()) {
      return plugin_element;
    }
    NOTREACHED();
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
  return blink::WebElement();
}

bool ChromePrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  return !command_line->HasSwitch(switches::kDisablePrintPreview);
}

bool ChromePrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!frame->GetDocument().IsPluginDocument())
    return false;

  std::vector<extensions::MimeHandlerViewContainer*> mime_handlers =
      extensions::MimeHandlerViewContainer::FromRenderFrame(
          content::RenderFrame::FromWebFrame(frame));
  if (!mime_handlers.empty()) {
    // This message is handled in chrome/browser/resources/pdf/pdf.js and
    // instructs the PDF plugin to print. This is to make window.print() on a
    // PDF plugin document correctly print the PDF. See
    // https://crbug.com/448720.
    base::DictionaryValue message;
    message.SetString("type", "print");
    mime_handlers.front()->PostMessageFromValue(message);
    return true;
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
  return false;
}
