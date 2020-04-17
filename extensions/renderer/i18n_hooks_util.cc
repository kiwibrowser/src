// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/i18n_hooks_util.h"

#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/message_bundle.h"
#include "gin/data_object_builder.h"

namespace extensions {
namespace i18n_hooks {

namespace {

struct DetectedLanguage {
  DetectedLanguage(const std::string& language, int percentage)
      : language(language), percentage(percentage) {}

  // Returns a new v8::Local<v8::Value> representing the serialized form of
  // this DetectedLanguage object.
  v8::Local<v8::Value> ToV8(v8::Isolate* isolate) const;

  std::string language;
  int percentage;
};

// LanguageDetectionResult object that holds detected langugae reliability and
// array of DetectedLanguage
struct LanguageDetectionResult {
  LanguageDetectionResult() {}
  ~LanguageDetectionResult() {}

  // Returns a new v8::Local<v8::Value> representing the serialized form of
  // this Result object.
  v8::Local<v8::Value> ToV8(v8::Local<v8::Context> context) const;

  // CLD detected language reliability
  bool is_reliable = false;

  // Array of detectedLanguage of size 1-3. The null is returned if
  // there were no languages detected
  std::vector<DetectedLanguage> languages;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageDetectionResult);
};

v8::Local<v8::Value> DetectedLanguage::ToV8(v8::Isolate* isolate) const {
  return gin::DataObjectBuilder(isolate)
      .Set("language", language)
      .Set("percentage", percentage)
      .Build();
}

v8::Local<v8::Value> LanguageDetectionResult::ToV8(
    v8::Local<v8::Context> context) const {
  v8::Isolate* isolate = context->GetIsolate();
  DCHECK(isolate->GetCurrentContext() == context);

  v8::Local<v8::Array> v8_languages = v8::Array::New(isolate, languages.size());
  for (uint32_t i = 0; i < languages.size(); ++i) {
    bool success =
        v8_languages->CreateDataProperty(context, i, languages[i].ToV8(isolate))
            .ToChecked();
    DCHECK(success) << "CreateDataProperty() should never fail.";
  }
  return gin::DataObjectBuilder(isolate)
      .Set("isReliable", is_reliable)
      .Set("languages", v8_languages.As<v8::Value>())
      .Build();
}

}  // namespace

v8::Local<v8::Value> GetI18nMessage(const std::string& message_name,
                                    const std::string& extension_id,
                                    v8::Local<v8::Value> v8_substitutions,
                                    content::RenderFrame* render_frame,
                                    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = context->GetIsolate();
  L10nMessagesMap* l10n_messages = nullptr;
  {
    ExtensionToL10nMessagesMap& messages_map = *GetExtensionToL10nMessagesMap();
    auto iter = messages_map.find(extension_id);
    if (iter != messages_map.end()) {
      l10n_messages = &iter->second;
    } else {
      if (!render_frame)
        return v8::Undefined(isolate);

      l10n_messages = &messages_map[extension_id];
      // A sync call to load message catalogs for current extension.
      // TODO(devlin): Wait, what?! A synchronous call to the browser to perform
      // potentially blocking work reading files from disk? That's Bad.
      {
        SCOPED_UMA_HISTOGRAM_TIMER("Extensions.SyncGetMessageBundle");
        render_frame->Send(
            new ExtensionHostMsg_GetMessageBundle(extension_id, l10n_messages));
      }
    }
  }

  std::string message =
      MessageBundle::GetL10nMessage(message_name, *l10n_messages);

  std::vector<std::string> substitutions;
  // For now, we just suppress all errors, but that's really not the best.
  // See https://crbug.com/807769.
  v8::TryCatch try_catch(isolate);
  if (v8_substitutions->IsArray()) {
    // chrome.i18n.getMessage("message_name", ["more", "params"]);
    v8::Local<v8::Array> placeholders = v8_substitutions.As<v8::Array>();
    uint32_t count = placeholders->Length();
    if (count > 9)
      return v8::Undefined(isolate);

    for (uint32_t i = 0; i < count; ++i) {
      v8::Local<v8::Value> placeholder;
      if (!placeholders->Get(context, i).ToLocal(&placeholder))
        return v8::Undefined(isolate);
      // Note: this tries to convert each entry to a JS string, which can fail.
      // If it does, String::Utf8Value() catches the error and doesn't surface
      // it to the calling script (though the call may still be observable,
      // since this goes through an object's toString() method). If it fails,
      // we just silently ignore the value.
      v8::String::Utf8Value string_value(isolate, placeholder);
      if (*string_value)
        substitutions.push_back(*string_value);
    }
  } else if (v8_substitutions->IsString()) {
    // chrome.i18n.getMessage("message_name", "one param");
    substitutions.push_back(gin::V8ToString(v8_substitutions));
  }
  // TODO(devlin): We currently just ignore any non-string, non-array values
  // for substitutions, but the type is documented as 'any'. We should either
  // enforce type more heavily, or throw an error here.

  // NOTE: We call ReplaceStringPlaceholders even if |substitutions| is empty
  // because we substitute $$ to be $ (in order to display a dollar sign in a
  // message). See https://crbug.com/127243.
  message = base::ReplaceStringPlaceholders(message, substitutions, nullptr);
  return gin::StringToV8(isolate, message);
}

v8::Local<v8::Value> DetectTextLanguage(v8::Local<v8::Context> context,
                                        const std::string& text) {
  LanguageDetectionResult result;

  return result.ToV8(context);
}

}  // namespace i18n_hooks
}  // namespace extensions
