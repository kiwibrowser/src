/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/html/html_meta_element.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/http_equiv.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/wtf/text/string_to_number.h"

namespace blink {

using namespace HTMLNames;

inline HTMLMetaElement::HTMLMetaElement(Document& document)
    : HTMLElement(metaTag, document) {}

DEFINE_NODE_FACTORY(HTMLMetaElement)

static bool IsInvalidSeparator(UChar c) {
  return c == ';';
}

// Though isspace() considers \t and \v to be whitespace, Win IE doesn't.
static bool IsSeparator(UChar c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '=' ||
         c == ',' || c == '\0';
}

void HTMLMetaElement::ParseContentAttribute(
    const String& content,
    void* data,
    Document* document,
    bool viewport_meta_zero_values_quirk) {
  bool has_invalid_separator = false;

  // Tread lightly in this code -- it was specifically designed to mimic Win
  // IE's parsing behavior.
  unsigned key_begin, key_end;
  unsigned value_begin, value_end;

  String buffer = content.DeprecatedLower();
  unsigned length = buffer.length();
  for (unsigned i = 0; i < length; /* no increment here */) {
    // skip to first non-separator, but don't skip past the end of the string
    while (IsSeparator(buffer[i])) {
      if (i >= length)
        break;
      i++;
    }
    key_begin = i;

    // skip to first separator
    while (!IsSeparator(buffer[i])) {
      has_invalid_separator |= IsInvalidSeparator(buffer[i]);
      if (i >= length)
        break;
      i++;
    }
    key_end = i;

    // skip to first '=', but don't skip past a ',' or the end of the string
    while (buffer[i] != '=') {
      has_invalid_separator |= IsInvalidSeparator(buffer[i]);
      if (buffer[i] == ',' || i >= length)
        break;
      i++;
    }

    // Skip to first non-separator, but don't skip past a ',' or the end of the
    // string.
    while (IsSeparator(buffer[i])) {
      if (buffer[i] == ',' || i >= length)
        break;
      i++;
    }
    value_begin = i;

    // skip to first separator
    while (!IsSeparator(buffer[i])) {
      has_invalid_separator |= IsInvalidSeparator(buffer[i]);
      if (i >= length)
        break;
      i++;
    }
    value_end = i;

    SECURITY_DCHECK(i <= length);

    String key_string = buffer.Substring(key_begin, key_end - key_begin);
    String value_string =
        buffer.Substring(value_begin, value_end - value_begin);
    ProcessViewportKeyValuePair(document, !has_invalid_separator, key_string,
                                value_string, viewport_meta_zero_values_quirk,
                                data);
  }
  if (has_invalid_separator && document) {
    String message =
        "Error parsing a meta element's content: ';' is not a valid key-value "
        "pair separator. Please use ',' instead.";
    document->AddConsoleMessage(ConsoleMessage::Create(
        kRenderingMessageSource, kWarningMessageLevel, message));
  }
}

static inline float ClampLengthValue(float value) {
  // Limits as defined in the css-device-adapt spec.
  if (value != ViewportDescription::kValueAuto)
    return std::min(float(10000), std::max(value, float(1)));
  return value;
}

static inline float ClampScaleValue(float value) {
  // Limits as defined in the css-device-adapt spec.
  if (value != ViewportDescription::kValueAuto)
    return std::min(float(10), std::max(value, float(0.1)));
  return value;
}

float HTMLMetaElement::ParsePositiveNumber(Document* document,
                                           bool report_warnings,
                                           const String& key_string,
                                           const String& value_string,
                                           bool* ok) {
  size_t parsed_length;
  float value;
  if (value_string.Is8Bit())
    value = CharactersToFloat(value_string.Characters8(), value_string.length(),
                              parsed_length);
  else
    value = CharactersToFloat(value_string.Characters16(),
                              value_string.length(), parsed_length);
  if (!parsed_length) {
    if (report_warnings)
      ReportViewportWarning(document, kUnrecognizedViewportArgumentValueError,
                            value_string, key_string);
    if (ok)
      *ok = false;
    return 0;
  }
  if (parsed_length < value_string.length() && report_warnings)
    ReportViewportWarning(document, kTruncatedViewportArgumentValueError,
                          value_string, key_string);
  if (ok)
    *ok = true;
  return value;
}

Length HTMLMetaElement::ParseViewportValueAsLength(Document* document,
                                                   bool report_warnings,
                                                   const String& key_string,
                                                   const String& value_string) {
  // 1) Non-negative number values are translated to px lengths.
  // 2) Negative number values are translated to auto.
  // 3) device-width and device-height are used as keywords.
  // 4) Other keywords and unknown values translate to auto.

  if (DeprecatedEqualIgnoringCase(value_string, "device-width"))
    return Length(kDeviceWidth);
  if (DeprecatedEqualIgnoringCase(value_string, "device-height"))
    return Length(kDeviceHeight);

  bool ok;

  float value = ParsePositiveNumber(document, report_warnings, key_string,
                                    value_string, &ok);

  if (!ok)
    return Length();  // auto

  if (value < 0)
    return Length();  // auto

  if (document && document->GetPage()) {
    value =
        document->GetPage()->GetChromeClient().WindowToViewportScalar(value);
  }
  return Length(ClampLengthValue(value), kFixed);
}

float HTMLMetaElement::ParseViewportValueAsZoom(
    Document* document,
    bool report_warnings,
    const String& key_string,
    const String& value_string,
    bool& computed_value_matches_parsed_value,
    bool viewport_meta_zero_values_quirk) {
  // 1) Non-negative number values are translated to <number> values.
  // 2) Negative number values are translated to auto.
  // 3) yes is translated to 1.0.
  // 4) device-width and device-height are translated to 10.0.
  // 5) no and unknown values are translated to 0.0

  computed_value_matches_parsed_value = false;
  if (DeprecatedEqualIgnoringCase(value_string, "yes"))
    return 1;
  if (DeprecatedEqualIgnoringCase(value_string, "no"))
    return 0;
  if (DeprecatedEqualIgnoringCase(value_string, "device-width"))
    return 10;
  if (DeprecatedEqualIgnoringCase(value_string, "device-height"))
    return 10;

  float value =
      ParsePositiveNumber(document, report_warnings, key_string, value_string);

  if (value < 0)
    return ViewportDescription::kValueAuto;

  if (value > 10.0 && report_warnings)
    ReportViewportWarning(document, kMaximumScaleTooLargeError, String(),
                          String());

  if (!value && viewport_meta_zero_values_quirk)
    return ViewportDescription::kValueAuto;

  float clamped_value = ClampScaleValue(value);
  if (clamped_value == value)
    computed_value_matches_parsed_value = true;

  return clamped_value;
}

bool HTMLMetaElement::ParseViewportValueAsUserZoom(
    Document* document,
    bool report_warnings,
    const String& key_string,
    const String& value_string,
    bool& computed_value_matches_parsed_value) {
  // yes and no are used as keywords.
  // Numbers >= 1, numbers <= -1, device-width and device-height are mapped to
  // yes.
  // Numbers in the range <-1, 1>, and unknown values, are mapped to no.

  computed_value_matches_parsed_value = false;
  if (DeprecatedEqualIgnoringCase(value_string, "yes")) {
    computed_value_matches_parsed_value = true;
    return true;
  }
  if (DeprecatedEqualIgnoringCase(value_string, "no")) {
    computed_value_matches_parsed_value = true;
    return false;
  }
  if (DeprecatedEqualIgnoringCase(value_string, "device-width"))
    return true;
  if (DeprecatedEqualIgnoringCase(value_string, "device-height"))
    return true;

  float value =
      ParsePositiveNumber(document, report_warnings, key_string, value_string);
  if (fabs(value) < 1)
    return false;

  return true;
}

float HTMLMetaElement::ParseViewportValueAsDPI(Document* document,
                                               bool report_warnings,
                                               const String& key_string,
                                               const String& value_string) {
  if (DeprecatedEqualIgnoringCase(value_string, "device-dpi"))
    return ViewportDescription::kValueDeviceDPI;
  if (DeprecatedEqualIgnoringCase(value_string, "low-dpi"))
    return ViewportDescription::kValueLowDPI;
  if (DeprecatedEqualIgnoringCase(value_string, "medium-dpi"))
    return ViewportDescription::kValueMediumDPI;
  if (DeprecatedEqualIgnoringCase(value_string, "high-dpi"))
    return ViewportDescription::kValueHighDPI;

  bool ok;
  float value = ParsePositiveNumber(document, report_warnings, key_string,
                                    value_string, &ok);
  if (!ok || value < 70 || value > 400)
    return ViewportDescription::kValueAuto;

  return value;
}

ViewportDescription::ViewportFit HTMLMetaElement::ParseViewportFitValueAsEnum(
    bool& unknown_value,
    const String& value_string) {
  if (DeprecatedEqualIgnoringCase(value_string, "auto"))
    return ViewportDescription::ViewportFit::kAuto;
  if (DeprecatedEqualIgnoringCase(value_string, "contain"))
    return ViewportDescription::ViewportFit::kContain;
  if (DeprecatedEqualIgnoringCase(value_string, "cover"))
    return ViewportDescription::ViewportFit::kCover;

  unknown_value = true;
  return ViewportDescription::ViewportFit::kAuto;
}

void HTMLMetaElement::ProcessViewportKeyValuePair(
    Document* document,
    bool report_warnings,
    const String& key_string,
    const String& value_string,
    bool viewport_meta_zero_values_quirk,
    void* data) {
  ViewportDescription* description = static_cast<ViewportDescription*>(data);

  if (key_string == "width") {
    const Length& width = ParseViewportValueAsLength(document, report_warnings,
                                                     key_string, value_string);
    if (!width.IsAuto()) {
      description->min_width = Length(kExtendToZoom);
      description->max_width = width;
    }
  } else if (key_string == "height") {
    const Length& height = ParseViewportValueAsLength(document, report_warnings,
                                                      key_string, value_string);
    if (!height.IsAuto()) {
      description->min_height = Length(kExtendToZoom);
      description->max_height = height;
    }
  } else if (key_string == "initial-scale") {
    description->zoom = ParseViewportValueAsZoom(
        document, report_warnings, key_string, value_string,
        description->zoom_is_explicit, viewport_meta_zero_values_quirk);
  } else if (key_string == "minimum-scale") {
    description->min_zoom = ParseViewportValueAsZoom(
        document, report_warnings, key_string, value_string,
        description->min_zoom_is_explicit, viewport_meta_zero_values_quirk);
  } else if (key_string == "maximum-scale") {
    description->max_zoom = ParseViewportValueAsZoom(
        document, report_warnings, key_string, value_string,
        description->max_zoom_is_explicit, viewport_meta_zero_values_quirk);
  } else if (key_string == "user-scalable") {
    description->user_zoom = ParseViewportValueAsUserZoom(
        document, report_warnings, key_string, value_string,
        description->user_zoom_is_explicit);
  } else if (key_string == "target-densitydpi") {
    description->deprecated_target_density_dpi = ParseViewportValueAsDPI(
        document, report_warnings, key_string, value_string);
    if (report_warnings)
      ReportViewportWarning(document, kTargetDensityDpiUnsupported, String(),
                            String());
  } else if (key_string == "minimal-ui") {
    // Ignore vendor-specific argument.
  } else if (key_string == "viewport-fit") {
    if (RuntimeEnabledFeatures::DisplayCutoutViewportFitEnabled()) {
      bool unknown_value = false;
      description->SetViewportFit(
          ParseViewportFitValueAsEnum(unknown_value, value_string));

      // If we got an unknown value then report a warning.
      if (unknown_value) {
        ReportViewportWarning(document, kViewportFitUnsupported, value_string,
                              String());
      }
    }
  } else if (key_string == "shrink-to-fit") {
    // Ignore vendor-specific argument.
  } else if (report_warnings) {
    ReportViewportWarning(document, kUnrecognizedViewportArgumentKeyError,
                          key_string, String());
  }
}

static const char* ViewportErrorMessageTemplate(ViewportErrorCode error_code) {
  static const char* const kErrors[] = {
      "The key \"%replacement1\" is not recognized and ignored.",
      "The value \"%replacement1\" for key \"%replacement2\" is invalid, and "
      "has been ignored.",
      "The value \"%replacement1\" for key \"%replacement2\" was truncated to "
      "its numeric prefix.",
      "The value for key \"maximum-scale\" is out of bounds and the value has "
      "been clamped.",
      "The key \"target-densitydpi\" is not supported.",
      "The value \"%replacement1\" for key \"viewport-fit\" is not supported.",
  };

  return kErrors[error_code];
}

static MessageLevel ViewportErrorMessageLevel(ViewportErrorCode error_code) {
  switch (error_code) {
    case kTruncatedViewportArgumentValueError:
    case kTargetDensityDpiUnsupported:
    case kUnrecognizedViewportArgumentKeyError:
    case kUnrecognizedViewportArgumentValueError:
    case kMaximumScaleTooLargeError:
    case kViewportFitUnsupported:
      return kWarningMessageLevel;
  }

  NOTREACHED();
  return kErrorMessageLevel;
}

void HTMLMetaElement::ReportViewportWarning(Document* document,
                                            ViewportErrorCode error_code,
                                            const String& replacement1,
                                            const String& replacement2) {
  if (!document || !document->GetFrame())
    return;

  String message = ViewportErrorMessageTemplate(error_code);
  if (!replacement1.IsNull())
    message.Replace("%replacement1", replacement1);
  if (!replacement2.IsNull())
    message.Replace("%replacement2", replacement2);

  // FIXME: This message should be moved off the console once a solution to
  // https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
  document->AddConsoleMessage(ConsoleMessage::Create(
      kRenderingMessageSource, ViewportErrorMessageLevel(error_code), message));
}

void HTMLMetaElement::GetViewportDescriptionFromContentAttribute(
    const String& content,
    ViewportDescription& description,
    Document* document,
    bool viewport_meta_zero_values_quirk) {
  ParseContentAttribute(content, (void*)&description, document,
                        viewport_meta_zero_values_quirk);

  if (description.min_zoom == ViewportDescription::kValueAuto)
    description.min_zoom = 0.25;

  if (description.max_zoom == ViewportDescription::kValueAuto) {
    description.max_zoom = 5;
    description.min_zoom = std::min(description.min_zoom, float(5));
  }
}

void HTMLMetaElement::ProcessViewportContentAttribute(
    const String& content,
    ViewportDescription::Type origin) {
  DCHECK(!content.IsNull());

  if (!GetDocument().ShouldOverrideLegacyDescription(origin))
    return;

  ViewportDescription description_from_legacy_tag(origin);
  if (GetDocument().ShouldMergeWithLegacyDescription(origin))
    description_from_legacy_tag = GetDocument().GetViewportDescription();

  GetViewportDescriptionFromContentAttribute(
      content, description_from_legacy_tag, &GetDocument(),
      GetDocument().GetSettings() &&
          GetDocument().GetSettings()->GetViewportMetaZeroValuesQuirk());

  GetDocument().SetViewportDescription(description_from_legacy_tag);
}

void HTMLMetaElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == http_equivAttr || params.name == contentAttr) {
    Process();
    return;
  }

  if (params.name != nameAttr)
    HTMLElement::ParseAttribute(params);
}

Node::InsertionNotificationRequest HTMLMetaElement::InsertedInto(
    ContainerNode* insertion_point) {
  HTMLElement::InsertedInto(insertion_point);
  return kInsertionShouldCallDidNotifySubtreeInsertions;
}

void HTMLMetaElement::DidNotifySubtreeInsertionsToDocument() {
  Process();
}

static bool InDocumentHead(HTMLMetaElement* element) {
  if (!element->isConnected())
    return false;

  return Traversal<HTMLHeadElement>::FirstAncestor(*element);
}

void HTMLMetaElement::Process() {
  if (!IsInDocumentTree())
    return;

  // All below situations require a content attribute (which can be the empty
  // string).
  const AtomicString& content_value = FastGetAttribute(contentAttr);
  if (content_value.IsNull())
    return;

  const AtomicString& name_value = FastGetAttribute(nameAttr);
  if (!name_value.IsEmpty()) {
    if (DeprecatedEqualIgnoringCase(name_value, "viewport"))
      ProcessViewportContentAttribute(content_value,
                                      ViewportDescription::kViewportMeta);
    else if (DeprecatedEqualIgnoringCase(name_value, "referrer"))
      GetDocument().ParseAndSetReferrerPolicy(
          content_value, true /* support legacy keywords */);
    else if (DeprecatedEqualIgnoringCase(name_value, "handheldfriendly") &&
             DeprecatedEqualIgnoringCase(content_value, "true"))
      ProcessViewportContentAttribute(
          "width=device-width", ViewportDescription::kHandheldFriendlyMeta);
    else if (DeprecatedEqualIgnoringCase(name_value, "mobileoptimized"))
      ProcessViewportContentAttribute(
          "width=device-width, initial-scale=1",
          ViewportDescription::kMobileOptimizedMeta);
    else if (DeprecatedEqualIgnoringCase(name_value, "theme-color") &&
             GetDocument().GetFrame())
      GetDocument().GetFrame()->Client()->DispatchDidChangeThemeColor();
  }

  // Get the document to process the tag, but only if we're actually part of DOM
  // tree (changing a meta tag while it's not in the tree shouldn't have any
  // effect on the document).

  const AtomicString& http_equiv_value = FastGetAttribute(http_equivAttr);
  if (http_equiv_value.IsEmpty())
    return;

  HttpEquiv::Process(GetDocument(), http_equiv_value, content_value,
                     InDocumentHead(this), this);
}

WTF::TextEncoding HTMLMetaElement::ComputeEncoding() const {
  HTMLAttributeList attribute_list;
  for (const Attribute& attr : Attributes())
    attribute_list.push_back(
        std::make_pair(attr.GetName().LocalName(), attr.Value().GetString()));
  return EncodingFromMetaAttributes(attribute_list);
}

const AtomicString& HTMLMetaElement::Content() const {
  return getAttribute(contentAttr);
}

const AtomicString& HTMLMetaElement::HttpEquiv() const {
  return getAttribute(http_equivAttr);
}

const AtomicString& HTMLMetaElement::GetName() const {
  return GetNameAttribute();
}
}
