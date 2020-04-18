/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/loader/form_submission.h"

#include "third_party/blink/public/platform/web_insecure_request_policy.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/forms/form_data.h"
#include "third_party/blink/renderer/core/html/forms/html_form_control_element.h"
#include "third_party/blink/renderer/core/html/forms/html_form_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/network/form_data_encoder.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

using namespace HTMLNames;

static int64_t GenerateFormDataIdentifier() {
  // Initialize to the current time to reduce the likelihood of generating
  // identifiers that overlap with those from past/future browser sessions.
  static int64_t next_identifier =
      static_cast<int64_t>(CurrentTime() * 1000000.0);
  return ++next_identifier;
}

static void AppendMailtoPostFormDataToURL(KURL& url,
                                          const EncodedFormData& data,
                                          const String& encoding_type) {
  String body = data.FlattenToString();

  if (DeprecatedEqualIgnoringCase(encoding_type, "text/plain")) {
    // Convention seems to be to decode, and s/&/\r\n/. Also, spaces are encoded
    // as %20.
    body = DecodeURLEscapeSequences(
        body.Replace('&', "\r\n").Replace('+', ' ') + "\r\n");
  }

  Vector<char> body_data;
  body_data.Append("body=", 5);
  FormDataEncoder::EncodeStringAsFormData(body_data, body.Utf8(),
                                          FormDataEncoder::kNormalizeCRLF);
  body = String(body_data.data(), body_data.size()).Replace('+', "%20");

  StringBuilder query;
  query.Append(url.Query());
  if (!query.IsEmpty())
    query.Append('&');
  query.Append(body);
  url.SetQuery(query.ToString());
}

void FormSubmission::Attributes::ParseAction(const String& action) {
  // m_action cannot be converted to KURL (bug https://crbug.com/388664)
  action_ = StripLeadingAndTrailingHTMLSpaces(action);
}

AtomicString FormSubmission::Attributes::ParseEncodingType(const String& type) {
  if (DeprecatedEqualIgnoringCase(type, "multipart/form-data"))
    return AtomicString("multipart/form-data");
  if (DeprecatedEqualIgnoringCase(type, "text/plain"))
    return AtomicString("text/plain");
  return AtomicString("application/x-www-form-urlencoded");
}

void FormSubmission::Attributes::UpdateEncodingType(const String& type) {
  encoding_type_ = ParseEncodingType(type);
  is_multi_part_form_ = (encoding_type_ == "multipart/form-data");
}

FormSubmission::SubmitMethod FormSubmission::Attributes::ParseMethodType(
    const String& type) {
  if (DeprecatedEqualIgnoringCase(type, "post"))
    return FormSubmission::kPostMethod;
  if (DeprecatedEqualIgnoringCase(type, "dialog"))
    return FormSubmission::kDialogMethod;
  return FormSubmission::kGetMethod;
}

void FormSubmission::Attributes::UpdateMethodType(const String& type) {
  method_ = ParseMethodType(type);
}

String FormSubmission::Attributes::MethodString(SubmitMethod method) {
  switch (method) {
    case kGetMethod:
      return "get";
    case kPostMethod:
      return "post";
    case kDialogMethod:
      return "dialog";
  }
  NOTREACHED();
  return g_empty_string;
}

void FormSubmission::Attributes::CopyFrom(const Attributes& other) {
  method_ = other.method_;
  is_multi_part_form_ = other.is_multi_part_form_;

  action_ = other.action_;
  target_ = other.target_;
  encoding_type_ = other.encoding_type_;
  accept_charset_ = other.accept_charset_;
}

inline FormSubmission::FormSubmission(SubmitMethod method,
                                      const KURL& action,
                                      const AtomicString& target,
                                      const AtomicString& content_type,
                                      HTMLFormElement* form,
                                      scoped_refptr<EncodedFormData> data,
                                      const String& boundary,
                                      Event* event)
    : method_(method),
      action_(action),
      target_(target),
      content_type_(content_type),
      form_(form),
      form_data_(std::move(data)),
      boundary_(boundary),
      event_(event) {}

inline FormSubmission::FormSubmission(const String& result)
    : method_(kDialogMethod), result_(result) {}

FormSubmission* FormSubmission::Create(HTMLFormElement* form,
                                       const Attributes& attributes,
                                       Event* event,
                                       HTMLFormControlElement* submit_button) {
  DCHECK(form);

  FormSubmission::Attributes copied_attributes;
  copied_attributes.CopyFrom(attributes);
  if (submit_button) {
    AtomicString attribute_value;
    if (!(attribute_value = submit_button->FastGetAttribute(formactionAttr))
             .IsNull())
      copied_attributes.ParseAction(attribute_value);
    if (!(attribute_value = submit_button->FastGetAttribute(formenctypeAttr))
             .IsNull())
      copied_attributes.UpdateEncodingType(attribute_value);
    if (!(attribute_value = submit_button->FastGetAttribute(formmethodAttr))
             .IsNull())
      copied_attributes.UpdateMethodType(attribute_value);
    if (!(attribute_value = submit_button->FastGetAttribute(formtargetAttr))
             .IsNull())
      copied_attributes.SetTarget(attribute_value);
  }

  if (copied_attributes.Method() == kDialogMethod) {
    if (submit_button)
      return new FormSubmission(submit_button->ResultForDialogSubmit());
    return new FormSubmission("");
  }

  Document& document = form->GetDocument();
  KURL action_url = document.CompleteURL(copied_attributes.Action().IsEmpty()
                                             ? document.Url().GetString()
                                             : copied_attributes.Action());

  if (document.GetInsecureRequestPolicy() & kUpgradeInsecureRequests &&
      action_url.ProtocolIs("http")) {
    UseCounter::Count(document,
                      WebFeature::kUpgradeInsecureRequestsUpgradedRequest);
    action_url.SetProtocol("https");
    if (action_url.Port() == 80)
      action_url.SetPort(443);
  }

  bool is_mailto_form = action_url.ProtocolIs("mailto");
  bool is_multi_part_form = false;
  AtomicString encoding_type = copied_attributes.EncodingType();

  if (copied_attributes.Method() == kPostMethod) {
    is_multi_part_form = copied_attributes.IsMultiPartForm();
    if (is_multi_part_form && is_mailto_form) {
      encoding_type = AtomicString("application/x-www-form-urlencoded");
      is_multi_part_form = false;
    }
  }
  WTF::TextEncoding data_encoding =
      is_mailto_form
          ? UTF8Encoding()
          : FormDataEncoder::EncodingFromAcceptCharset(
                copied_attributes.AcceptCharset(), document.Encoding());
  FormData* dom_form_data =
      FormData::Create(data_encoding.EncodingForFormSubmission());
  form->ConstructFormDataSet(submit_button, *dom_form_data);

  scoped_refptr<EncodedFormData> form_data;
  String boundary;

  if (is_multi_part_form) {
    form_data = dom_form_data->EncodeMultiPartFormData();
    boundary = form_data->Boundary().data();
  } else {
    form_data = dom_form_data->EncodeFormData(
        attributes.Method() == kGetMethod
            ? EncodedFormData::kFormURLEncoded
            : EncodedFormData::ParseEncodingType(encoding_type));
    if (copied_attributes.Method() == kPostMethod && is_mailto_form) {
      // Convert the form data into a string that we put into the URL.
      AppendMailtoPostFormDataToURL(action_url, *form_data, encoding_type);
      form_data = EncodedFormData::Create();
    }
  }

  form_data->SetIdentifier(GenerateFormDataIdentifier());
  form_data->SetContainsPasswordData(dom_form_data->ContainsPasswordData());
  AtomicString target_or_base_target = copied_attributes.Target().IsEmpty()
                                           ? document.BaseTarget()
                                           : copied_attributes.Target();
  return new FormSubmission(copied_attributes.Method(), action_url,
                            target_or_base_target, encoding_type, form,
                            std::move(form_data), boundary, event);
}

void FormSubmission::Trace(blink::Visitor* visitor) {
  visitor->Trace(form_);
  visitor->Trace(event_);
}

KURL FormSubmission::RequestURL() const {
  if (method_ == FormSubmission::kPostMethod)
    return action_;

  KURL request_url(action_);
  request_url.SetQuery(form_data_->FlattenToString());
  return request_url;
}

FrameLoadRequest FormSubmission::CreateFrameLoadRequest(
    Document* origin_document) {
  FrameLoadRequest frame_request(origin_document);

  if (!target_.IsEmpty())
    frame_request.SetFrameName(target_);

  if (method_ == FormSubmission::kPostMethod) {
    frame_request.GetResourceRequest().SetHTTPMethod(HTTPNames::POST);
    frame_request.GetResourceRequest().SetHTTPBody(form_data_);

    // construct some user headers if necessary
    if (boundary_.IsEmpty()) {
      frame_request.GetResourceRequest().SetHTTPContentType(content_type_);
    } else {
      frame_request.GetResourceRequest().SetHTTPContentType(
          content_type_ + "; boundary=" + boundary_);
    }
  }

  frame_request.GetResourceRequest().SetURL(RequestURL());

  frame_request.SetTriggeringEvent(event_);
  frame_request.SetForm(form_);

  return frame_request;
}

}  // namespace blink
