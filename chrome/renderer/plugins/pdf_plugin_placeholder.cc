// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/plugins/pdf_plugin_placeholder.h"

#include "chrome/common/pdf_uma.h"
#include "chrome/common/render_messages.h"
#include "chrome/grit/renderer_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/renderer/render_thread.h"
#include "gin/object_template_builder.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

gin::WrapperInfo PDFPluginPlaceholder::kWrapperInfo = {gin::kEmbedderNativeGin};

PDFPluginPlaceholder::PDFPluginPlaceholder(content::RenderFrame* render_frame,
                                           const blink::WebPluginParams& params,
                                           const std::string& html_data)
    : plugins::PluginPlaceholderBase(render_frame, params, html_data) {}

PDFPluginPlaceholder::~PDFPluginPlaceholder() {}

PDFPluginPlaceholder* PDFPluginPlaceholder::CreatePDFPlaceholder(
    content::RenderFrame* render_frame,
    const blink::WebPluginParams& params) {
  std::string template_html = ui::ResourceBundle::GetSharedInstance()
                                  .GetRawDataResource(IDR_PDF_PLUGIN_HTML)
                                  .as_string();
  webui::AppendWebUiCssTextDefaults(&template_html);

  base::DictionaryValue values;
  values.SetString("fileName", GURL(params.url).ExtractFileName());
  values.SetString("open", l10n_util::GetStringUTF8(IDS_ACCNAME_OPEN));
  std::string html_data = webui::GetI18nTemplateHtml(template_html, &values);

  return new PDFPluginPlaceholder(render_frame, params, html_data);
}

v8::Local<v8::Value> PDFPluginPlaceholder::GetV8Handle(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, this).ToV8();
}

gin::ObjectTemplateBuilder PDFPluginPlaceholder::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<PDFPluginPlaceholder>::GetObjectTemplateBuilder(isolate)
      .SetMethod<void (PDFPluginPlaceholder::*)()>(
          "openPDF", &PDFPluginPlaceholder::OpenPDFCallback);
}

void PDFPluginPlaceholder::OpenPDFCallback() {
  ReportPDFLoadStatus(PDFLoadStatus::kViewPdfClickedInPdfPluginPlaceholder);
  content::RenderThread::Get()->Send(
      new ChromeViewHostMsg_OpenPDF(routing_id(), GetPluginParams().url));
}
