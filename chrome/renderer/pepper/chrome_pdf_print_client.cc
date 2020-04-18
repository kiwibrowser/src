// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/chrome_pdf_print_client.h"

#include "components/printing/renderer/print_render_frame_helper.h"
#include "content/public/renderer/pepper_plugin_instance.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_plugin_container.h"

namespace {

blink::WebElement GetWebElement(PP_Instance instance_id) {
  content::PepperPluginInstance* instance =
      content::PepperPluginInstance::Get(instance_id);
  if (!instance)
    return blink::WebElement();
  return instance->GetContainer()->GetElement();
}

printing::PrintRenderFrameHelper* GetPrintRenderFrameHelper(
    const blink::WebElement& element) {
  if (element.IsNull())
    return nullptr;
  auto* render_frame =
      content::RenderFrame::FromWebFrame(element.GetDocument().GetFrame());
  return printing::PrintRenderFrameHelper::Get(render_frame);
}

}  // namespace

ChromePDFPrintClient::ChromePDFPrintClient() {}

ChromePDFPrintClient::~ChromePDFPrintClient() {}

bool ChromePDFPrintClient::IsPrintingEnabled(PP_Instance instance_id) {
  blink::WebElement element = GetWebElement(instance_id);
  printing::PrintRenderFrameHelper* helper = GetPrintRenderFrameHelper(element);
  return helper && helper->IsPrintingEnabled();
}

bool ChromePDFPrintClient::Print(PP_Instance instance_id) {
  blink::WebElement element = GetWebElement(instance_id);
  printing::PrintRenderFrameHelper* helper = GetPrintRenderFrameHelper(element);
  if (!helper)
    return false;
  helper->PrintNode(element);
  return true;
}
