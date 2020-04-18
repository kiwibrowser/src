// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A helper function for using JsTemplate. See jstemplate_builder.h for more
// info.

#include "ui/base/webui/jstemplate_builder.h"

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"
#include "ui/resources/grit/webui_resources.h"

namespace webui {

namespace {

// Appends a script tag with a variable name |templateData| that has the JSON
// assigned to it.
void AppendJsonHtml(const base::DictionaryValue* json, std::string* output) {
  std::string javascript_string;
  AppendJsonJS(json, &javascript_string);

  // </ confuses the HTML parser because it could be a </script> tag.  So we
  // replace </ with <\/.  The extra \ will be ignored by the JS engine.
  base::ReplaceSubstringsAfterOffset(&javascript_string, 0, "</", "<\\/");

  output->append("<script>");
  output->append(javascript_string);
  output->append("</script>");
}

// Appends the source for load_time_data.js in a script tag.
void AppendLoadTimeData(std::string* output) {
  // fetch and cache the pointer of the jstemplate resource source text.
  base::StringPiece load_time_data_src(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBUI_JS_LOAD_TIME_DATA));

  if (load_time_data_src.empty()) {
    NOTREACHED() << "Unable to get loadTimeData src";
    return;
  }

  output->append("<script>");
  load_time_data_src.AppendToString(output);
  output->append("</script>");
}

// Appends the source for JsTemplates in a script tag.
void AppendJsTemplateSourceHtml(std::string* output) {
  // fetch and cache the pointer of the jstemplate resource source text.
  base::StringPiece jstemplate_src(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBUI_JSTEMPLATE_JS));

  if (jstemplate_src.empty()) {
    NOTREACHED() << "Unable to get jstemplate src";
    return;
  }

  output->append("<script>");
  jstemplate_src.AppendToString(output);
  output->append("</script>");
}

// Appends the code that processes the JsTemplate with the JSON. You should
// call AppendJsTemplateSourceHtml and AppendJsonHtml before calling this.
void AppendJsTemplateProcessHtml(
    const base::StringPiece& template_id,
    std::string* output) {
  output->append("<script>");
  output->append("var tp = document.getElementById('");
  output->append(template_id.data(), template_id.size());
  output->append("');");
  output->append("jstProcess(loadTimeData.createJsEvalContext(), tp);");
  output->append("</script>");
}

// Appends the source for i18n Templates in a script tag.
void AppendI18nTemplateSourceHtml(std::string* output) {
  base::StringPiece i18n_template_src(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBUI_I18N_TEMPLATE_JS));

  if (i18n_template_src.empty()) {
    NOTREACHED() << "Unable to get i18n template src";
    return;
  }

  output->append("<script>");
  i18n_template_src.AppendToString(output);
  output->append("</script>");
}

}  // namespace

std::string GetI18nTemplateHtml(const base::StringPiece& html_template,
                                const base::DictionaryValue* json) {
  ui::TemplateReplacements replacements;
  ui::TemplateReplacementsFromDictionaryValue(*json, &replacements);
  std::string output =
      ui::ReplaceTemplateExpressions(html_template, replacements);

  // TODO(dschuyler): After the i18n-content and i18n-values are replaced with
  // $i18n{} replacements, we will be able to return output at this point.
  // Remove Append*() lines that builds up the i18n replacement work to be done
  // in JavaScript.
  AppendLoadTimeData(&output);
  AppendJsonHtml(json, &output);
  AppendI18nTemplateSourceHtml(&output);

  return output;
}

std::string GetTemplatesHtml(const base::StringPiece& html_template,
                             const base::DictionaryValue* json,
                             const base::StringPiece& template_id) {
  ui::TemplateReplacements replacements;
  ui::TemplateReplacementsFromDictionaryValue(*json, &replacements);
  std::string output =
      ui::ReplaceTemplateExpressions(html_template, replacements);

  // TODO(dschuyler): After the i18n-content and i18n-values are replaced with
  // $i18n{} replacements, we will be able to return output at this point.
  // Remove Append*() lines that builds up the i18n replacement work to be done
  // in JavaScript.
  AppendLoadTimeData(&output);
  AppendJsonHtml(json, &output);
  AppendI18nTemplateSourceHtml(&output);
  AppendJsTemplateSourceHtml(&output);
  AppendJsTemplateProcessHtml(template_id, &output);
  return output;
}

void AppendJsonJS(const base::DictionaryValue* json, std::string* output) {
  // Convert the template data to a json string.
  DCHECK(json) << "must include json data structure";

  std::string jstext;
  JSONStringValueSerializer serializer(&jstext);
  serializer.Serialize(*json);
  output->append("loadTimeData.data = ");
  output->append(jstext);
  output->append(";");
}

}  // namespace webui
