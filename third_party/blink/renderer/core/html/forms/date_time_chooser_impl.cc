/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/date_time_chooser_impl.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/forms/date_time_chooser_client.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page_popup.h"
#include "third_party/blink/renderer/platform/date_components.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/text/platform_locale.h"

namespace blink {

DateTimeChooserImpl::DateTimeChooserImpl(
    ChromeClient* chrome_client,
    DateTimeChooserClient* client,
    const DateTimeChooserParameters& parameters)
    : chrome_client_(chrome_client),
      client_(client),
      popup_(nullptr),
      parameters_(parameters),
      locale_(Locale::Create(parameters.locale)) {
  DCHECK(RuntimeEnabledFeatures::InputMultipleFieldsUIEnabled());
  DCHECK(chrome_client_);
  DCHECK(client_);
  popup_ = chrome_client_->OpenPagePopup(this);
}

DateTimeChooserImpl* DateTimeChooserImpl::Create(
    ChromeClient* chrome_client,
    DateTimeChooserClient* client,
    const DateTimeChooserParameters& parameters) {
  return new DateTimeChooserImpl(chrome_client, client, parameters);
}

DateTimeChooserImpl::~DateTimeChooserImpl() = default;

void DateTimeChooserImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(chrome_client_);
  visitor->Trace(client_);
  DateTimeChooser::Trace(visitor);
}

void DateTimeChooserImpl::EndChooser() {
  if (!popup_)
    return;
  chrome_client_->ClosePagePopup(popup_);
}

AXObject* DateTimeChooserImpl::RootAXObject() {
  return popup_ ? popup_->RootAXObject() : nullptr;
}

static String ValueToDateTimeString(double value, AtomicString type) {
  DateComponents components;
  if (type == InputTypeNames::date)
    components.SetMillisecondsSinceEpochForDate(value);
  else if (type == InputTypeNames::datetime_local)
    components.SetMillisecondsSinceEpochForDateTimeLocal(value);
  else if (type == InputTypeNames::month)
    components.SetMonthsSinceEpoch(value);
  else if (type == InputTypeNames::time)
    components.SetMillisecondsSinceMidnight(value);
  else if (type == InputTypeNames::week)
    components.SetMillisecondsSinceEpochForWeek(value);
  else
    NOTREACHED();
  return components.GetType() == DateComponents::kInvalid
             ? String()
             : components.ToString();
}

void DateTimeChooserImpl::WriteDocument(SharedBuffer* data) {
  String step_string = String::Number(parameters_.step);
  String step_base_string = String::Number(parameters_.step_base, 11);
  String today_label_string;
  String other_date_label_string;
  if (parameters_.type == InputTypeNames::month) {
    today_label_string =
        GetLocale().QueryString(WebLocalizedString::kThisMonthButtonLabel);
    other_date_label_string =
        GetLocale().QueryString(WebLocalizedString::kOtherMonthLabel);
  } else if (parameters_.type == InputTypeNames::week) {
    today_label_string =
        GetLocale().QueryString(WebLocalizedString::kThisWeekButtonLabel);
    other_date_label_string =
        GetLocale().QueryString(WebLocalizedString::kOtherWeekLabel);
  } else {
    today_label_string =
        GetLocale().QueryString(WebLocalizedString::kCalendarToday);
    other_date_label_string =
        GetLocale().QueryString(WebLocalizedString::kOtherDateLabel);
  }

  AddString("<!DOCTYPE html><head><meta charset='UTF-8'><style>\n", data);
  data->Append(Platform::Current()->GetDataResource("pickerCommon.css"));
  data->Append(Platform::Current()->GetDataResource("pickerButton.css"));
  data->Append(Platform::Current()->GetDataResource("suggestionPicker.css"));
  data->Append(Platform::Current()->GetDataResource("calendarPicker.css"));
  AddString(
      "</style></head><body><div id=main>Loading...</div><script>\n"
      "window.dialogArguments = {\n",
      data);
  AddProperty("anchorRectInScreen", parameters_.anchor_rect_in_screen, data);
  float scale_factor = chrome_client_->WindowToViewportScalar(1.0f);
  AddProperty("zoomFactor", ZoomFactor() / scale_factor, data);
  AddProperty("min",
              ValueToDateTimeString(parameters_.minimum, parameters_.type),
              data);
  AddProperty("max",
              ValueToDateTimeString(parameters_.maximum, parameters_.type),
              data);
  AddProperty("step", step_string, data);
  AddProperty("stepBase", step_base_string, data);
  AddProperty("required", parameters_.required, data);
  AddProperty("currentValue",
              ValueToDateTimeString(parameters_.double_value, parameters_.type),
              data);
  AddProperty("locale", parameters_.locale.GetString(), data);
  AddProperty("todayLabel", today_label_string, data);
  AddProperty("clearLabel",
              GetLocale().QueryString(WebLocalizedString::kCalendarClear),
              data);
  AddProperty("weekLabel",
              GetLocale().QueryString(WebLocalizedString::kWeekNumberLabel),
              data);
  AddProperty(
      "axShowMonthSelector",
      GetLocale().QueryString(WebLocalizedString::kAXCalendarShowMonthSelector),
      data);
  AddProperty(
      "axShowNextMonth",
      GetLocale().QueryString(WebLocalizedString::kAXCalendarShowNextMonth),
      data);
  AddProperty(
      "axShowPreviousMonth",
      GetLocale().QueryString(WebLocalizedString::kAXCalendarShowPreviousMonth),
      data);
  AddProperty("weekStartDay", locale_->FirstDayOfWeek(), data);
  AddProperty("shortMonthLabels", locale_->ShortMonthLabels(), data);
  AddProperty("dayLabels", locale_->WeekDayShortLabels(), data);
  AddProperty("isLocaleRTL", locale_->IsRTL(), data);
  AddProperty("isRTL", parameters_.is_anchor_element_rtl, data);
  AddProperty("mode", parameters_.type.GetString(), data);
  if (parameters_.suggestions.size()) {
    Vector<String> suggestion_values;
    Vector<String> localized_suggestion_values;
    Vector<String> suggestion_labels;
    for (unsigned i = 0; i < parameters_.suggestions.size(); i++) {
      suggestion_values.push_back(ValueToDateTimeString(
          parameters_.suggestions[i].value, parameters_.type));
      localized_suggestion_values.push_back(
          parameters_.suggestions[i].localized_value);
      suggestion_labels.push_back(parameters_.suggestions[i].label);
    }
    AddProperty("suggestionValues", suggestion_values, data);
    AddProperty("localizedSuggestionValues", localized_suggestion_values, data);
    AddProperty("suggestionLabels", suggestion_labels, data);
    AddProperty(
        "inputWidth",
        static_cast<unsigned>(parameters_.anchor_rect_in_screen.Width()), data);
    AddProperty(
        "showOtherDateEntry",
        LayoutTheme::GetTheme().SupportsCalendarPicker(parameters_.type), data);
    AddProperty("otherDateLabel", other_date_label_string, data);
    AddProperty("suggestionHighlightColor",
                LayoutTheme::GetTheme()
                    .ActiveListBoxSelectionBackgroundColor()
                    .Serialized(),
                data);
    AddProperty("suggestionHighlightTextColor",
                LayoutTheme::GetTheme()
                    .ActiveListBoxSelectionForegroundColor()
                    .Serialized(),
                data);
  }
  AddString("}\n", data);

  data->Append(Platform::Current()->GetDataResource("pickerCommon.js"));
  data->Append(Platform::Current()->GetDataResource("suggestionPicker.js"));
  data->Append(Platform::Current()->GetDataResource("calendarPicker.js"));
  AddString("</script></body>\n", data);
}

Element& DateTimeChooserImpl::OwnerElement() {
  return client_->OwnerElement();
}

Locale& DateTimeChooserImpl::GetLocale() {
  return *locale_;
}

void DateTimeChooserImpl::SetValueAndClosePopup(int num_value,
                                                const String& string_value) {
  if (num_value >= 0)
    SetValue(string_value);
  EndChooser();
}

void DateTimeChooserImpl::SetValue(const String& value) {
  client_->DidChooseValue(value);
}

void DateTimeChooserImpl::ClosePopup() {
  EndChooser();
}

void DateTimeChooserImpl::DidClosePopup() {
  DCHECK(client_);
  popup_ = nullptr;
  client_->DidEndChooser();
}

}  // namespace blink
