/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/external_date_time_chooser.h"

#include "third_party/blink/public/web/web_date_time_chooser_completion.h"
#include "third_party/blink/public/web/web_date_time_chooser_params.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/html/forms/date_time_chooser_client.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class WebDateTimeChooserCompletionImpl : public WebDateTimeChooserCompletion {
 public:
  WebDateTimeChooserCompletionImpl(ExternalDateTimeChooser* chooser)
      : chooser_(chooser) {}

 private:
  void DidChooseValue(double value) override {
    chooser_->DidChooseValue(value);
    delete this;
  }

  void DidCancelChooser() override {
    chooser_->DidCancelChooser();
    delete this;
  }

  Persistent<ExternalDateTimeChooser> chooser_;
};

ExternalDateTimeChooser::~ExternalDateTimeChooser() = default;

void ExternalDateTimeChooser::Trace(blink::Visitor* visitor) {
  visitor->Trace(client_);
  DateTimeChooser::Trace(visitor);
}

ExternalDateTimeChooser::ExternalDateTimeChooser(DateTimeChooserClient* client)
    : client_(client) {
  DCHECK(!RuntimeEnabledFeatures::InputMultipleFieldsUIEnabled());
  DCHECK(client);
}

ExternalDateTimeChooser* ExternalDateTimeChooser::Create(
    ChromeClient* chrome_client,
    WebViewClient* web_view_client,
    DateTimeChooserClient* client,
    const DateTimeChooserParameters& parameters) {
  DCHECK(chrome_client);
  ExternalDateTimeChooser* chooser = new ExternalDateTimeChooser(client);
  if (!chooser->OpenDateTimeChooser(chrome_client, web_view_client, parameters))
    chooser = nullptr;
  return chooser;
}

static WebDateTimeInputType ToWebDateTimeInputType(const AtomicString& source) {
  if (source == InputTypeNames::date)
    return kWebDateTimeInputTypeDate;
  if (source == InputTypeNames::datetime)
    return kWebDateTimeInputTypeDateTime;
  if (source == InputTypeNames::datetime_local)
    return kWebDateTimeInputTypeDateTimeLocal;
  if (source == InputTypeNames::month)
    return kWebDateTimeInputTypeMonth;
  if (source == InputTypeNames::time)
    return kWebDateTimeInputTypeTime;
  if (source == InputTypeNames::week)
    return kWebDateTimeInputTypeWeek;
  return kWebDateTimeInputTypeNone;
}

bool ExternalDateTimeChooser::OpenDateTimeChooser(
    ChromeClient* chrome_client,
    WebViewClient* web_view_client,
    const DateTimeChooserParameters& parameters) {
  if (!web_view_client)
    return false;

  WebDateTimeChooserParams web_params;
  web_params.type = ToWebDateTimeInputType(parameters.type);
  web_params.anchor_rect_in_screen = parameters.anchor_rect_in_screen;
  web_params.double_value = parameters.double_value;
  web_params.suggestions = parameters.suggestions;
  web_params.minimum = parameters.minimum;
  web_params.maximum = parameters.maximum;
  web_params.step = parameters.step;
  web_params.step_base = parameters.step_base;
  web_params.is_required = parameters.required;
  web_params.is_anchor_element_rtl = parameters.is_anchor_element_rtl;

  WebDateTimeChooserCompletion* completion =
      new WebDateTimeChooserCompletionImpl(this);
  if (web_view_client->OpenDateTimeChooser(web_params, completion))
    return true;
  // We can't open a chooser. Calling
  // WebDateTimeChooserCompletionImpl::didCancelChooser to delete the
  // WebDateTimeChooserCompletionImpl object and deref this.
  completion->DidCancelChooser();
  return false;
}

void ExternalDateTimeChooser::DidChooseValue(const WebString& value) {
  if (client_)
    client_->DidChooseValue(value);
  // didChooseValue might run JavaScript code, and endChooser() might be
  // called. However DateTimeChooserCompletionImpl still has one reference to
  // this object.
  if (client_)
    client_->DidEndChooser();
}

void ExternalDateTimeChooser::DidChooseValue(double value) {
  if (client_)
    client_->DidChooseValue(value);
  // didChooseValue might run JavaScript code, and endChooser() might be
  // called. However DateTimeChooserCompletionImpl still has one reference to
  // this object.
  if (client_)
    client_->DidEndChooser();
}

void ExternalDateTimeChooser::DidCancelChooser() {
  if (client_)
    client_->DidEndChooser();
}

void ExternalDateTimeChooser::EndChooser() {
  DateTimeChooserClient* client = client_;
  client_ = nullptr;
  client->DidEndChooser();
}

AXObject* ExternalDateTimeChooser::RootAXObject() {
  return nullptr;
}

}  // namespace blink
