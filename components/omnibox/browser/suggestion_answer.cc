// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/suggestion_answer.h"

#include <stddef.h>

#include <memory>

#include "base/feature_list.h"
#include "base/i18n/rtl.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/values.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "net/base/escape.h"
#include "url/url_constants.h"

namespace {

// All of these are defined here (even though most are only used once each) so
// the format details are easy to locate and update or compare to the spec doc.
static constexpr char kAnswerJsonLines[] = "l";
static constexpr char kAnswerJsonImageLine[] = "il";
static constexpr char kAnswerJsonText[] = "t";
static constexpr char kAnswerJsonAdditionalText[] = "at";
static constexpr char kAnswerJsonStatusText[] = "st";
static constexpr char kAnswerJsonTextType[] = "tt";
static constexpr char kAnswerJsonNumLines[] = "ln";
static constexpr char kAnswerJsonImage[] = "i";
static constexpr char kAnswerJsonImageData[] = "i.d";

void AppendWithSpace(const SuggestionAnswer::TextField* text,
                     base::string16* output) {
  if (!text)
    return;
  if (!output->empty() && !text->text().empty())
    *output += ' ';
  *output += text->text();
}

}  // namespace

// SuggestionAnswer::TextField -------------------------------------------------

SuggestionAnswer::TextField::TextField()
    : type_(-1), has_num_lines_(false), num_lines_(1) {}
SuggestionAnswer::TextField::~TextField() {}

// static
bool SuggestionAnswer::TextField::ParseTextField(
    const base::DictionaryValue* field_json, TextField* text_field) {
  bool parsed = field_json->GetString(kAnswerJsonText, &text_field->text_) &&
      !text_field->text_.empty() &&
      field_json->GetInteger(kAnswerJsonTextType, &text_field->type_);
  if (parsed) {
    text_field->text_ = net::UnescapeForHTML(text_field->text_);
    text_field->has_num_lines_ =
        field_json->GetInteger(kAnswerJsonNumLines, &text_field->num_lines_);
  }
  return parsed;
}

bool SuggestionAnswer::TextField::Equals(const TextField& field) const {
  return type_ == field.type_ && text_ == field.text_ &&
         has_num_lines_ == field.has_num_lines_ &&
         (!has_num_lines_ || num_lines_ == field.num_lines_);
}

size_t SuggestionAnswer::TextField::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(text_);
}

// SuggestionAnswer::ImageLine -------------------------------------------------

SuggestionAnswer::ImageLine::ImageLine()
    : num_text_lines_(1) {}
SuggestionAnswer::ImageLine::ImageLine(const ImageLine& line)
    : text_fields_(line.text_fields_),
      num_text_lines_(line.num_text_lines_),
      additional_text_(line.additional_text_ ?
                       new TextField(*line.additional_text_) : nullptr),
      status_text_(line.status_text_ ?
                   new TextField(*line.status_text_) : nullptr),
      image_url_(line.image_url_) {}

SuggestionAnswer::ImageLine::~ImageLine() {}

// static
bool SuggestionAnswer::ImageLine::ParseImageLine(
    const base::DictionaryValue* line_json, ImageLine* image_line) {
  const base::DictionaryValue* inner_json;
  if (!line_json->GetDictionary(kAnswerJsonImageLine, &inner_json))
    return false;

  const base::ListValue* fields_json;
  if (!inner_json->GetList(kAnswerJsonText, &fields_json) ||
      fields_json->GetSize() == 0)
    return false;

  bool found_num_lines = false;
  for (size_t i = 0; i < fields_json->GetSize(); ++i) {
    const base::DictionaryValue* field_json;
    TextField text_field;
    if (!fields_json->GetDictionary(i, &field_json) ||
        !TextField::ParseTextField(field_json, &text_field))
      return false;
    image_line->text_fields_.push_back(text_field);
    if (!found_num_lines && text_field.has_num_lines()) {
      found_num_lines = true;
      image_line->num_text_lines_ = text_field.num_lines();
    }
  }

  if (inner_json->HasKey(kAnswerJsonAdditionalText)) {
    image_line->additional_text_.reset(new TextField());
    const base::DictionaryValue* field_json;
    if (!inner_json->GetDictionary(kAnswerJsonAdditionalText, &field_json) ||
        !TextField::ParseTextField(field_json,
                                   image_line->additional_text_.get()))
      return false;
  }

  if (inner_json->HasKey(kAnswerJsonStatusText)) {
    image_line->status_text_.reset(new TextField());
    const base::DictionaryValue* field_json;
    if (!inner_json->GetDictionary(kAnswerJsonStatusText, &field_json) ||
        !TextField::ParseTextField(field_json, image_line->status_text_.get()))
      return false;
  }

  if (inner_json->HasKey(kAnswerJsonImage)) {
    base::string16 url_string;
    if (!inner_json->GetString(kAnswerJsonImageData, &url_string) ||
        url_string.empty())
      return false;
    // If necessary, concatenate scheme and host/path using only ':' as
    // separator. This is due to the results delivering strings of the form
    // "//host/path", which is web-speak for "use the enclosing page's scheme",
    // but not a valid path of an URL.  The GWS frontend commonly (always?)
    // redirects to HTTPS so we just default to that here.
    image_line->image_url_ =
        GURL(base::StartsWith(url_string, base::ASCIIToUTF16("//"),
                              base::CompareCase::SENSITIVE)
                 ? (base::ASCIIToUTF16(url::kHttpsScheme) +
                    base::ASCIIToUTF16(":") + url_string)
                 : url_string);

    if (!image_line->image_url_.is_valid())
      return false;
  }

  return true;
}

bool SuggestionAnswer::ImageLine::Equals(const ImageLine& line) const {
  if (text_fields_.size() != line.text_fields_.size())
    return false;
  for (size_t i = 0; i < text_fields_.size(); ++i) {
    if (!text_fields_[i].Equals(line.text_fields_[i]))
      return false;
  }

  if (num_text_lines_ != line.num_text_lines_)
    return false;

  if (additional_text_ || line.additional_text_) {
    if (!additional_text_ || !line.additional_text_)
      return false;
    if (!additional_text_->Equals(*line.additional_text_))
      return false;
  }

  if (status_text_ || line.status_text_) {
    if (!status_text_ || !line.status_text_)
      return false;
    if (!status_text_->Equals(*line.status_text_))
      return false;
  }

  return image_url_ == line.image_url_;
}

// TODO(jdonnelly): When updating the display of answers in RTL languages,
// modify this to be consistent.
base::string16 SuggestionAnswer::ImageLine::AccessibleText() const {
  base::string16 result;
  for (const TextField& text_field : text_fields_)
    AppendWithSpace(&text_field, &result);
  AppendWithSpace(additional_text_.get(), &result);
  AppendWithSpace(status_text_.get(), &result);
  return result;
}

size_t SuggestionAnswer::ImageLine::EstimateMemoryUsage() const {
  size_t res = 0;

  res += base::trace_event::EstimateMemoryUsage(text_fields_);
  res += base::trace_event::EstimateMemoryUsage(additional_text_);
  res += base::trace_event::EstimateMemoryUsage(status_text_);
  res += base::trace_event::EstimateMemoryUsage(image_url_);

  return res;
}

// SuggestionAnswer ------------------------------------------------------------

SuggestionAnswer::SuggestionAnswer() : type_(-1) {}

SuggestionAnswer::SuggestionAnswer(const SuggestionAnswer& answer) = default;

SuggestionAnswer::~SuggestionAnswer() = default;

// static
std::unique_ptr<SuggestionAnswer> SuggestionAnswer::ParseAnswer(
    const base::DictionaryValue* answer_json) {
  auto result = std::make_unique<SuggestionAnswer>();

  const base::ListValue* lines_json;
  if (!answer_json->GetList(kAnswerJsonLines, &lines_json) ||
      lines_json->GetSize() != 2) {
    return nullptr;
  }

  const base::DictionaryValue* first_line_json;
  if (!lines_json->GetDictionary(0, &first_line_json) ||
      !ImageLine::ParseImageLine(first_line_json, &result->first_line_)) {
    return nullptr;
  }

  const base::DictionaryValue* second_line_json;
  if (!lines_json->GetDictionary(1, &second_line_json) ||
      !ImageLine::ParseImageLine(second_line_json, &result->second_line_)) {
    return nullptr;
  }

  std::string image_url;
  const base::DictionaryValue* optional_image;
  if (OmniboxFieldTrial::IsNewAnswerLayoutEnabled() &&
      answer_json->GetDictionary("i", &optional_image) &&
      optional_image->GetString("d", &image_url)) {
    result->image_url_ = GURL(image_url);
  } else {
    result->image_url_ = result->second_line_.image_url();
  }

  return result;
}

bool SuggestionAnswer::Equals(const SuggestionAnswer& answer) const {
  return type_ == answer.type_ && image_url_ == answer.image_url_ &&
         first_line_.Equals(answer.first_line_) &&
         second_line_.Equals(answer.second_line_);
}

void SuggestionAnswer::AddImageURLsTo(std::vector<GURL>* urls) const {
  // Note: first_line_.image_url() is not used in practice (so it's ignored).
  if (image_url_.is_valid())
    urls->push_back(image_url_);
  else if (second_line_.image_url().is_valid())
    urls->push_back(second_line_.image_url());
}

size_t SuggestionAnswer::EstimateMemoryUsage() const {
  size_t res = 0;

  res += base::trace_event::EstimateMemoryUsage(image_url_);
  res += base::trace_event::EstimateMemoryUsage(first_line_);
  res += base::trace_event::EstimateMemoryUsage(second_line_);

  return res;
}
