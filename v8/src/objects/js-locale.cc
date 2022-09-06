// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_INTL_SUPPORT
#error Internationalization is expected to be enabled.
#endif  // V8_INTL_SUPPORT

#include "src/objects/js-locale.h"

#include <map>
#include <memory>
#include <string>

#include "src/api.h"
#include "src/global-handles.h"
#include "src/heap/factory.h"
#include "src/isolate.h"
#include "src/objects-inl.h"
#include "src/objects/js-locale-inl.h"
#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "unicode/unistr.h"
#include "unicode/uvernum.h"
#include "unicode/uversion.h"

#if U_ICU_VERSION_MAJOR_NUM >= 59
#include "unicode/char16ptr.h"
#endif

namespace v8 {
namespace internal {

namespace {

// Extracts value of a given property key in the Object.
Maybe<bool> ExtractStringSetting(Isolate* isolate, Handle<JSReceiver> options,
                                 const char* key, icu::UnicodeString* setting) {
  v8::Isolate* v8_isolate = reinterpret_cast<v8::Isolate*>(isolate);
  Handle<String> str = isolate->factory()->NewStringFromAsciiChecked(key);

  // JSReceiver::GetProperty could throw an exception and return an empty
  // MaybeHandle<Object>().
  // Returns Nothing<bool> on exception.
  Handle<Object> object;
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, object, JSReceiver::GetProperty(options, str), Nothing<bool>());

  if (object->IsString()) {
    v8::String::Utf8Value utf8_string(
        v8_isolate, v8::Utils::ToLocal(Handle<String>::cast(object)));
    *setting = icu::UnicodeString::fromUTF8(*utf8_string);
    return Just(true);
  }

  return Just(false);
}

// Inserts tags from options into locale string.
Maybe<bool> InsertOptionsIntoLocale(Isolate* isolate,
                                    Handle<JSReceiver> options,
                                    char* icu_locale) {
  CHECK(isolate);
  CHECK(icu_locale);

  static const std::array<std::pair<const char*, const char*>, 6>
      kOptionToUnicodeTagMap = {{{"calendar", "ca"},
                                 {"collation", "co"},
                                 {"hourCycle", "hc"},
                                 {"caseFirst", "kf"},
                                 {"numeric", "kn"},
                                 {"numberingSystem", "nu"}}};

  for (const auto& option_to_bcp47 : kOptionToUnicodeTagMap) {
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString value_unicode;

    Maybe<bool> found = ExtractStringSetting(
        isolate, options, option_to_bcp47.first, &value_unicode);
    // Return on exception.
    MAYBE_RETURN(found, Nothing<bool>());
    if (!found.FromJust()) {
      // Skip this key, user didn't specify it in options.
      continue;
    }
    DCHECK(found.FromJust());

    std::string value_string;
    value_unicode.toUTF8String(value_string);

    // Convert bcp47 key and value into legacy ICU format so we can use
    // uloc_setKeywordValue.
    const char* key = uloc_toLegacyKey(option_to_bcp47.second);
    if (!key) return Just(false);

    // Overwrite existing, or insert new key-value to the locale string.
    const char* value = uloc_toLegacyType(key, value_string.c_str());
    if (value) {
      // TODO(cira): ICU puts artificial limit on locale length, while BCP47
      // doesn't. Switch to C++ API when it's ready.
      // Related ICU bug - https://ssl.icu-project.org/trac/ticket/13417.
      uloc_setKeywordValue(key, value, icu_locale, ULOC_FULLNAME_CAPACITY,
                           &status);
      if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
        return Just(false);
      }
    } else {
      return Just(false);
    }
  }

  return Just(true);
}

// Fills in the JSLocale object slots with Unicode tag/values.
bool PopulateLocaleWithUnicodeTags(Isolate* isolate, const char* icu_locale,
                                   Handle<JSLocale> locale_holder) {
  CHECK(isolate);
  CHECK(icu_locale);

  Factory* factory = isolate->factory();

  UErrorCode status = U_ZERO_ERROR;
  UEnumeration* keywords = uloc_openKeywords(icu_locale, &status);
  if (!keywords) return true;

  char value[ULOC_FULLNAME_CAPACITY];
  while (const char* keyword = uenum_next(keywords, nullptr, &status)) {
    uloc_getKeywordValue(icu_locale, keyword, value, ULOC_FULLNAME_CAPACITY,
                         &status);
    if (U_FAILURE(status)) {
      status = U_ZERO_ERROR;
      continue;
    }

    // Ignore those we don't recognize - spec allows that.
    const char* bcp47_key = uloc_toUnicodeLocaleKey(keyword);
    if (bcp47_key) {
      const char* bcp47_value = uloc_toUnicodeLocaleType(bcp47_key, value);
      if (bcp47_value) {
          // It's either Boolean value.
          if (strncmp(bcp47_key, "kn", 2) == 0) {
            bool numeric = strcmp(bcp47_value, "true") == 0 ? true : false;
            Handle<Object> numeric_handle = factory->ToBoolean(numeric);
            locale_holder->set_numeric(*numeric_handle);
            continue;
          }
          // Or a string.
          Handle<String> bcp47_handle =
              factory->NewStringFromAsciiChecked(bcp47_value);
          if (strncmp(bcp47_key, "ca", 2) == 0) {
            locale_holder->set_calendar(*bcp47_handle);
          } else if (strncmp(bcp47_key, "kf", 2) == 0) {
            locale_holder->set_case_first(*bcp47_handle);
          } else if (strncmp(bcp47_key, "co", 2) == 0) {
            locale_holder->set_collation(*bcp47_handle);
          } else if (strncmp(bcp47_key, "hc", 2) == 0) {
            locale_holder->set_hour_cycle(*bcp47_handle);
          } else if (strncmp(bcp47_key, "nu", 2) == 0) {
            locale_holder->set_numbering_system(*bcp47_handle);
          }
      }
    }
  }

  uenum_close(keywords);

  return true;
}
}  // namespace

MaybeHandle<JSLocale> JSLocale::InitializeLocale(Isolate* isolate,
                                Handle<JSLocale> locale_holder,
                                Handle<String> locale,
                                Handle<JSReceiver> options) {
  static const char* const kMethod = "Intl.Locale";
  v8::Isolate* v8_isolate = reinterpret_cast<v8::Isolate*>(isolate);
  UErrorCode status = U_ZERO_ERROR;

  // Get ICU locale format, and canonicalize it.
  char icu_result[ULOC_FULLNAME_CAPACITY];
  char icu_canonical[ULOC_FULLNAME_CAPACITY];

  if (locale->length() == 0) {
    THROW_NEW_ERROR(isolate, NewRangeError(MessageTemplate::kLocaleNotEmpty),
                    JSLocale);
  }


  v8::String::Utf8Value bcp47_locale(v8_isolate, v8::Utils::ToLocal(locale));
  if (bcp47_locale.length() == 0) return MaybeHandle<JSLocale>();

  int icu_length = uloc_forLanguageTag(
      *bcp47_locale, icu_result, ULOC_FULLNAME_CAPACITY, nullptr, &status);

  if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING ||
      icu_length == 0) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }

  Maybe<bool> error = InsertOptionsIntoLocale(isolate, options, icu_result);
  if (error.IsNothing() || !error.FromJust()) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }

  uloc_canonicalize(icu_result, icu_canonical, ULOC_FULLNAME_CAPACITY, &status);
  if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }

  if (!PopulateLocaleWithUnicodeTags(isolate, icu_canonical, locale_holder)) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }

  // Extract language, script and region parts.
  char icu_language[ULOC_LANG_CAPACITY];
  uloc_getLanguage(icu_canonical, icu_language, ULOC_LANG_CAPACITY, &status);

  char icu_script[ULOC_SCRIPT_CAPACITY];
  uloc_getScript(icu_canonical, icu_script, ULOC_SCRIPT_CAPACITY, &status);

  char icu_region[ULOC_COUNTRY_CAPACITY];
  uloc_getCountry(icu_canonical, icu_region, ULOC_COUNTRY_CAPACITY, &status);

  if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }

  Factory* factory = isolate->factory();

  // NOTE: One shouldn't use temporary handles, because they can go out of
  // scope and be garbage collected before properly assigned.
  // DON'T DO THIS: locale_holder->set_language(*f->NewStringAscii...);
  Handle<String> language = factory->NewStringFromAsciiChecked(icu_language);
  locale_holder->set_language(*language);

  if (strlen(icu_script) != 0) {
    Handle<String> script = factory->NewStringFromAsciiChecked(icu_script);
    locale_holder->set_script(*script);
  }

  if (strlen(icu_region) != 0) {
    Handle<String> region = factory->NewStringFromAsciiChecked(icu_region);
    locale_holder->set_region(*region);
  }

  char icu_base_name[ULOC_FULLNAME_CAPACITY];
  uloc_getBaseName(icu_canonical, icu_base_name, ULOC_FULLNAME_CAPACITY,
                   &status);
  // We need to convert it back to BCP47.
  char bcp47_result[ULOC_FULLNAME_CAPACITY];
  uloc_toLanguageTag(icu_base_name, bcp47_result, ULOC_FULLNAME_CAPACITY, true,
                     &status);
  if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }
  Handle<String> base_name = factory->NewStringFromAsciiChecked(bcp47_result);
  locale_holder->set_base_name(*base_name);

  // Produce final representation of the locale string, for toString().
  uloc_toLanguageTag(icu_canonical, bcp47_result, ULOC_FULLNAME_CAPACITY, true,
                     &status);
  if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
    THROW_NEW_ERROR(
        isolate,
        NewRangeError(MessageTemplate::kLocaleBadParameters,
                      isolate->factory()->NewStringFromAsciiChecked(kMethod),
                      locale_holder),
        JSLocale);
    return MaybeHandle<JSLocale>();
  }
  Handle<String> locale_handle =
      factory->NewStringFromAsciiChecked(bcp47_result);
  locale_holder->set_locale(*locale_handle);

  return locale_holder;
}


namespace {

Handle<String> MorphLocale(Isolate* isolate, String* input,
                           int32_t (*morph_func)(const char*, char*, int32_t,
                                                 UErrorCode*)) {
  Factory* factory = isolate->factory();
  char localeBuffer[ULOC_FULLNAME_CAPACITY];
  UErrorCode status = U_ZERO_ERROR;
  DCHECK_NOT_NULL(morph_func);
  int32_t length = (*morph_func)(input->ToCString().get(), localeBuffer,
                                 ULOC_FULLNAME_CAPACITY, &status);
  DCHECK(U_SUCCESS(status));
  DCHECK_GT(length, 0);
  std::string locale(localeBuffer, length);
  std::replace(locale.begin(), locale.end(), '_', '-');
  return factory->NewStringFromAsciiChecked(locale.c_str());
}

}  // namespace

Handle<String> JSLocale::Maximize(Isolate* isolate, String* locale) {
  return MorphLocale(isolate, locale, uloc_addLikelySubtags);
}

Handle<String> JSLocale::Minimize(Isolate* isolate, String* locale) {
  return MorphLocale(isolate, locale, uloc_minimizeSubtags);
}

}  // namespace internal
}  // namespace v8
