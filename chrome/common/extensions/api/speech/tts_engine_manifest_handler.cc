// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/speech/tts_engine_manifest_handler.h"

#include <stddef.h>

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;


TtsVoice::TtsVoice() : remote(false) {}

TtsVoice::TtsVoice(const TtsVoice& other) = default;

TtsVoice::~TtsVoice() {}

TtsVoices::TtsVoices() {}
TtsVoices::~TtsVoices() {}

//  static
bool TtsVoices::Parse(const base::ListValue* tts_voices,
                      TtsVoices* out_voices,
                      base::string16* error) {
  for (size_t i = 0; i < tts_voices->GetSize(); i++) {
    const base::DictionaryValue* one_tts_voice = nullptr;
    if (!tts_voices->GetDictionary(i, &one_tts_voice)) {
      *error = base::ASCIIToUTF16(errors::kInvalidTtsVoices);
      return false;
    }

    TtsVoice voice_data;
    if (one_tts_voice->HasKey(keys::kTtsVoicesVoiceName)) {
      if (!one_tts_voice->GetString(
              keys::kTtsVoicesVoiceName, &voice_data.voice_name)) {
        *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesVoiceName);
        return false;
      }
    }
    if (one_tts_voice->HasKey(keys::kTtsVoicesLang)) {
      if (!one_tts_voice->GetString(
              keys::kTtsVoicesLang, &voice_data.lang) ||
          !l10n_util::IsValidLocaleSyntax(voice_data.lang)) {
        *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesLang);
        return false;
      }
    }
    if (one_tts_voice->HasKey(keys::kTtsVoicesGender)) {
      if (!one_tts_voice->GetString(
              keys::kTtsVoicesGender, &voice_data.gender) ||
          (voice_data.gender != keys::kTtsGenderMale &&
           voice_data.gender != keys::kTtsGenderFemale)) {
        *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesGender);
        return false;
      }
    }
    if (one_tts_voice->HasKey(keys::kTtsVoicesRemote)) {
      if (!one_tts_voice->GetBoolean(
              keys::kTtsVoicesRemote, &voice_data.remote)) {
        *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesRemote);
        return false;
      }
    }
    if (one_tts_voice->HasKey(keys::kTtsVoicesEventTypes)) {
      const base::ListValue* event_types_list;
      if (!one_tts_voice->GetList(
              keys::kTtsVoicesEventTypes,
              &event_types_list)) {
        *error = base::ASCIIToUTF16(
            errors::kInvalidTtsVoicesEventTypes);
        return false;
      }
      for (size_t i = 0; i < event_types_list->GetSize(); i++) {
        std::string event_type;
        if (!event_types_list->GetString(i, &event_type)) {
          *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesEventTypes);
          return false;
        }
        if (event_type != keys::kTtsVoicesEventTypeEnd &&
            event_type != keys::kTtsVoicesEventTypeError &&
            event_type != keys::kTtsVoicesEventTypeMarker &&
            event_type != keys::kTtsVoicesEventTypeSentence &&
            event_type != keys::kTtsVoicesEventTypeStart &&
            event_type != keys::kTtsVoicesEventTypeWord) {
          *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesEventTypes);
          return false;
        }
        if (voice_data.event_types.find(event_type) !=
            voice_data.event_types.end()) {
          *error = base::ASCIIToUTF16(errors::kInvalidTtsVoicesEventTypes);
          return false;
        }
        voice_data.event_types.insert(event_type);
      }
    }
    out_voices->voices.push_back(voice_data);
  }
  return true;
}

// static
const std::vector<TtsVoice>* TtsVoices::GetTtsVoices(
    const Extension* extension) {
  TtsVoices* info =
      static_cast<TtsVoices*>(extension->GetManifestData(keys::kTtsVoices));
  return info ? &info->voices : nullptr;
}

TtsEngineManifestHandler::TtsEngineManifestHandler() {}

TtsEngineManifestHandler::~TtsEngineManifestHandler() {}

bool TtsEngineManifestHandler::Parse(Extension* extension,
                                     base::string16* error) {
  auto info = std::make_unique<TtsVoices>();
  const base::DictionaryValue* tts_dict = nullptr;
  if (!extension->manifest()->GetDictionary(keys::kTtsEngine, &tts_dict)) {
    *error = base::ASCIIToUTF16(errors::kInvalidTts);
    return false;
  }

  if (!tts_dict->HasKey(keys::kTtsVoices))
    return true;

  const base::ListValue* tts_voices = nullptr;
  if (!tts_dict->GetList(keys::kTtsVoices, &tts_voices)) {
    *error = base::ASCIIToUTF16(errors::kInvalidTtsVoices);
    return false;
  }

  if (!TtsVoices::Parse(tts_voices, info.get(), error))
    return false;

  extension->SetManifestData(keys::kTtsVoices, std::move(info));
  return true;
}

base::span<const char* const> TtsEngineManifestHandler::Keys() const {
  static constexpr const char* kKeys[] = {keys::kTtsEngine};
  return kKeys;
}

}  // namespace extensions
