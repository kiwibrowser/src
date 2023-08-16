// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/search_suggestion_parser.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "base/i18n/icu_string_conversions.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/omnibox/browser/autocomplete_i18n.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/url_prefix.h"
#include "components/url_formatter/url_fixer.h"
#include "components/url_formatter/url_formatter.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "ui/base/device_form_factor.h"
#include "url/url_constants.h"

namespace {

AutocompleteMatchType::Type GetAutocompleteMatchType(const std::string& type) {
  if (type == "CALCULATOR")
    return AutocompleteMatchType::CALCULATOR;
  if (type == "ENTITY")
    return AutocompleteMatchType::SEARCH_SUGGEST_ENTITY;
  if (type == "TAIL")
    return AutocompleteMatchType::SEARCH_SUGGEST_TAIL;
  if (type == "PERSONALIZED_QUERY")
    return AutocompleteMatchType::SEARCH_SUGGEST_PERSONALIZED;
  if (type == "PROFILE")
    return AutocompleteMatchType::SEARCH_SUGGEST_PROFILE;
  if (type == "NAVIGATION")
    return AutocompleteMatchType::NAVSUGGEST;
  if (type == "PERSONALIZED_NAVIGATION")
    return AutocompleteMatchType::NAVSUGGEST_PERSONALIZED;
  return AutocompleteMatchType::SEARCH_SUGGEST;
}

}  // namespace

// SearchSuggestionParser::Result ----------------------------------------------

SearchSuggestionParser::Result::Result(bool from_keyword_provider,
                                       int relevance,
                                       bool relevance_from_server,
                                       AutocompleteMatchType::Type type,
                                       int subtype_identifier,
                                       const std::string& deletion_url)
    : from_keyword_provider_(from_keyword_provider),
      type_(type),
      subtype_identifier_(subtype_identifier),
      relevance_(relevance),
      relevance_from_server_(relevance_from_server),
      received_after_last_keystroke_(true),
      deletion_url_(deletion_url) {}

SearchSuggestionParser::Result::Result(const Result& other) = default;

SearchSuggestionParser::Result::~Result() {}

// SearchSuggestionParser::SuggestResult ---------------------------------------

SearchSuggestionParser::SuggestResult::SuggestResult(
    const base::string16& suggestion,
    AutocompleteMatchType::Type type,
    int subtype_identifier,
    const base::string16& match_contents,
    const base::string16& match_contents_prefix,
    const base::string16& annotation,
    const base::string16& answer_contents,
    const base::string16& answer_type,
    std::unique_ptr<SuggestionAnswer> answer,
    const std::string& suggest_query_params,
    const std::string& deletion_url,
    const std::string& image_dominant_color,
    const std::string& image_url,
    bool from_keyword_provider,
    int relevance,
    bool relevance_from_server,
    bool should_prefetch,
    const base::string16& input_text)
    : Result(from_keyword_provider,
             relevance,
             relevance_from_server,
             type,
             subtype_identifier,
             deletion_url),
      suggestion_(suggestion),
      match_contents_prefix_(match_contents_prefix),
      annotation_(annotation),
      suggest_query_params_(suggest_query_params),
      answer_contents_(answer_contents),
      answer_type_(answer_type),
      answer_(std::move(answer)),
      image_dominant_color_(image_dominant_color),
      image_url_(image_url),
      should_prefetch_(should_prefetch) {
  match_contents_ = match_contents;
  DCHECK(!match_contents_.empty());
  ClassifyMatchContents(true, input_text);
}

SearchSuggestionParser::SuggestResult::SuggestResult(
    const SuggestResult& result)
    : Result(result),
      suggestion_(result.suggestion_),
      match_contents_prefix_(result.match_contents_prefix_),
      annotation_(result.annotation_),
      suggest_query_params_(result.suggest_query_params_),
      answer_contents_(result.answer_contents_),
      answer_type_(result.answer_type_),
      answer_(SuggestionAnswer::copy(result.answer_.get())),
      image_dominant_color_(result.image_dominant_color_),
      image_url_(result.image_url_),
      should_prefetch_(result.should_prefetch_) {}

SearchSuggestionParser::SuggestResult::~SuggestResult() {}

SearchSuggestionParser::SuggestResult&
    SearchSuggestionParser::SuggestResult::operator=(const SuggestResult& rhs) {
  if (this == &rhs)
    return *this;

  // Assign via parent class first.
  Result::operator=(rhs);

  suggestion_ = rhs.suggestion_;
  match_contents_prefix_ = rhs.match_contents_prefix_;
  annotation_ = rhs.annotation_;
  suggest_query_params_ = rhs.suggest_query_params_;
  answer_contents_ = rhs.answer_contents_;
  answer_type_ = rhs.answer_type_;
  answer_ = SuggestionAnswer::copy(rhs.answer_.get());
  image_dominant_color_ = rhs.image_dominant_color_;
  image_url_ = rhs.image_url_;
  should_prefetch_ = rhs.should_prefetch_;

  return *this;
}

void SearchSuggestionParser::SuggestResult::ClassifyMatchContents(
    const bool allow_bolding_all,
    const base::string16& input_text) {
  if (input_text.empty()) {
    // In case of zero-suggest results, do not highlight matches.
    match_contents_class_.push_back(
        ACMatchClassification(0, ACMatchClassification::NONE));
    return;
  }

  base::string16 lookup_text = input_text;
  if (type_ == AutocompleteMatchType::SEARCH_SUGGEST_TAIL) {
    const size_t contents_index =
        suggestion_.length() - match_contents_.length();
    // Ensure the query starts with the input text, and ends with the match
    // contents, and the input text has an overlap with contents.
    if (base::StartsWith(suggestion_, input_text,
                         base::CompareCase::SENSITIVE) &&
        base::EndsWith(suggestion_, match_contents_,
                       base::CompareCase::SENSITIVE) &&
        (input_text.length() > contents_index)) {
      lookup_text = input_text.substr(contents_index);
    }
  }
  // Do a case-insensitive search for |lookup_text|.
  base::string16::const_iterator lookup_position = std::search(
      match_contents_.begin(), match_contents_.end(), lookup_text.begin(),
      lookup_text.end(), SimpleCaseInsensitiveCompareUCS2());
  if (!allow_bolding_all && (lookup_position == match_contents_.end())) {
    // Bail if the code below to update the bolding would bold the whole
    // string.  Note that the string may already be entirely bolded; if
    // so, leave it as is.
    return;
  }
  match_contents_class_.clear();
  // We do intra-string highlighting for suggestions - the suggested segment
  // will be highlighted, e.g. for input_text = "you" the suggestion may be
  // "youtube", so we'll bold the "tube" section: you*tube*.
  if (input_text != match_contents_) {
    if (lookup_position == match_contents_.end()) {
      // The input text is not a substring of the query string, e.g. input
      // text is "slasdot" and the query string is "slashdot", so we bold the
      // whole thing.
      match_contents_class_.push_back(
          ACMatchClassification(0, ACMatchClassification::MATCH));
    } else {
      // We don't iterate over the string here annotating all matches because
      // it looks odd to have every occurrence of a substring that may be as
      // short as a single character highlighted in a query suggestion result,
      // e.g. for input text "s" and query string "southwest airlines", it
      // looks odd if both the first and last s are highlighted.
      const size_t lookup_index = lookup_position - match_contents_.begin();
      if (lookup_index != 0) {
        match_contents_class_.push_back(
            ACMatchClassification(0, ACMatchClassification::MATCH));
      }
      match_contents_class_.push_back(
          ACMatchClassification(lookup_index, ACMatchClassification::NONE));
      size_t next_fragment_position = lookup_index + lookup_text.length();
      if (next_fragment_position < match_contents_.length()) {
        match_contents_class_.push_back(ACMatchClassification(
            next_fragment_position, ACMatchClassification::MATCH));
      }
    }
  } else {
    // Otherwise, match_contents_ is a verbatim (what-you-typed) match, either
    // for the default provider or a keyword search provider.
    match_contents_class_.push_back(
        ACMatchClassification(0, ACMatchClassification::NONE));
  }
}

int SearchSuggestionParser::SuggestResult::CalculateRelevance(
    const AutocompleteInput& input,
    bool keyword_provider_requested) const {
  if (!from_keyword_provider_ && keyword_provider_requested)
    return 100;
  return ((input.type() == metrics::OmniboxInputType::URL) ? 300 : 600);
}

// SearchSuggestionParser::NavigationResult ------------------------------------

SearchSuggestionParser::NavigationResult::NavigationResult(
    const AutocompleteSchemeClassifier& scheme_classifier,
    const GURL& url,
    AutocompleteMatchType::Type type,
    int subtype_identifier,
    const base::string16& description,
    const std::string& deletion_url,
    bool from_keyword_provider,
    int relevance,
    bool relevance_from_server,
    const base::string16& input_text)
    : Result(from_keyword_provider,
             relevance,
             relevance_from_server,
             type,
             subtype_identifier,
             deletion_url),
      url_(url),
      formatted_url_(AutocompleteInput::FormattedStringWithEquivalentMeaning(
          url,
          url_formatter::FormatUrl(url,
                                   url_formatter::kFormatUrlOmitDefaults &
                                       ~url_formatter::kFormatUrlOmitHTTP,
                                   net::UnescapeRule::SPACES,
                                   nullptr,
                                   nullptr,
                                   nullptr),
          scheme_classifier,
          nullptr)),
      description_(description) {
  DCHECK(url_.is_valid());
  CalculateAndClassifyMatchContents(true, input_text);
}

SearchSuggestionParser::NavigationResult::~NavigationResult() {}

void
SearchSuggestionParser::NavigationResult::CalculateAndClassifyMatchContents(
    const bool allow_bolding_nothing,
    const base::string16& input_text) {
  if (input_text.empty()) {
    // In case of zero-suggest results, do not highlight matches.
    match_contents_class_.push_back(
        ACMatchClassification(0, ACMatchClassification::NONE));
    return;
  }

  // First look for the user's input inside the formatted url as it would be
  // without trimming the scheme, so we can find matches at the beginning of the
  // scheme.
  const URLPrefix* prefix =
      URLPrefix::BestURLPrefix(formatted_url_, input_text);
  size_t match_start = (prefix == nullptr) ? formatted_url_.find(input_text)
                                           : prefix->prefix.length();

  bool match_in_scheme = false;
  bool match_in_subdomain = false;
  bool match_after_host = false;
  AutocompleteMatch::GetMatchComponents(
      GURL(formatted_url_), {{match_start, match_start + input_text.length()}},
      &match_in_scheme, &match_in_subdomain, &match_after_host);
  auto format_types = AutocompleteMatch::GetFormatTypes(
      GURL(input_text).has_scheme() || match_in_scheme, match_in_subdomain,
      match_after_host);

  base::string16 match_contents =
      url_formatter::FormatUrl(url_, format_types, net::UnescapeRule::SPACES,
                               nullptr, nullptr, &match_start);
  // If the first match in the untrimmed string was inside a scheme that we
  // trimmed, look for a subsequent match.
  if (match_start == base::string16::npos)
    match_start = match_contents.find(input_text);
  // Update |match_contents_| and |match_contents_class_| if it's allowed.
  if (allow_bolding_nothing || (match_start != base::string16::npos)) {
    match_contents_ = match_contents;
    // Safe if |match_start| is npos; also safe if the input is longer than the
    // remaining contents after |match_start|.
    AutocompleteMatch::ClassifyLocationInString(match_start,
        input_text.length(), match_contents_.length(),
        ACMatchClassification::URL, &match_contents_class_);
  }
}

int SearchSuggestionParser::NavigationResult::CalculateRelevance(
    const AutocompleteInput& input,
    bool keyword_provider_requested) const {
  return (from_keyword_provider_ || !keyword_provider_requested) ? 800 : 150;
}

// SearchSuggestionParser::Results ---------------------------------------------

SearchSuggestionParser::Results::Results()
    : verbatim_relevance(-1),
      field_trial_triggered(false),
      relevances_from_server(false) {}

SearchSuggestionParser::Results::~Results() {}

void SearchSuggestionParser::Results::Clear() {
  suggest_results.clear();
  navigation_results.clear();
  verbatim_relevance = -1;
  metadata.clear();
}

bool SearchSuggestionParser::Results::HasServerProvidedScores() const {
  if (verbatim_relevance >= 0)
    return true;

  // Right now either all results of one type will be server-scored or they will
  // all be locally scored, but in case we change this later, we'll just check
  // them all.
  for (SuggestResults::const_iterator i(suggest_results.begin());
       i != suggest_results.end(); ++i) {
    if (i->relevance_from_server())
      return true;
  }
  for (NavigationResults::const_iterator i(navigation_results.begin());
       i != navigation_results.end(); ++i) {
    if (i->relevance_from_server())
      return true;
  }

  return false;
}

// SearchSuggestionParser ------------------------------------------------------

// static
std::string SearchSuggestionParser::ExtractJsonData(
    const net::URLFetcher* source) {
  const net::HttpResponseHeaders* const response_headers =
      source->GetResponseHeaders();
  std::string json_data;
  source->GetResponseAsString(&json_data);

  // JSON is supposed to be UTF-8, but some suggest service providers send
  // JSON files in non-UTF-8 encodings.  The actual encoding is usually
  // specified in the Content-Type header field.
  if (response_headers) {
    std::string charset;
    if (response_headers->GetCharset(&charset)) {
      base::string16 data_16;
      // TODO(jungshik): Switch to CodePageToUTF8 after it's added.
      if (base::CodepageToUTF16(json_data, charset.c_str(),
                                base::OnStringConversionError::FAIL,
                                &data_16))
        json_data = base::UTF16ToUTF8(data_16);
    }
  }
  return json_data;
}

// static
std::unique_ptr<base::Value> SearchSuggestionParser::DeserializeJsonData(
    base::StringPiece json_data) {
  LOG(INFO) << "Received answer from DeserializeJsonData: <"<< json_data << ">";
  // The JSON response should be an array.
  for (size_t response_start_index = json_data.find("["), i = 0;
       response_start_index != base::StringPiece::npos && i < 5;
       response_start_index = json_data.find("[", 1), i++) {
    // Remove any XSSI guards to allow for JSON parsing.
    json_data.remove_prefix(response_start_index);

    JSONStringValueDeserializer deserializer(json_data,
                                             base::JSON_ALLOW_TRAILING_COMMAS);
    int error_code = 0;
    std::unique_ptr<base::Value> data =
        deserializer.Deserialize(&error_code, nullptr);
    if (error_code == 0)
      return data;
  }
  return std::unique_ptr<base::Value>();
}

// static
bool SearchSuggestionParser::ParseSuggestResults(
    const base::Value& root_val,
    const AutocompleteInput& input,
    const AutocompleteSchemeClassifier& scheme_classifier,
    int default_result_relevance,
    bool is_keyword_result,
    Results* results) {
  base::string16 query;
  const base::ListValue* root_list = nullptr;
  const base::ListValue* results_list = nullptr;

  if (!root_val.GetAsList(&root_list) || !root_list->GetString(0, &query) ||
      query != input.text() || !root_list->GetList(1, &results_list))
    return false;

  // 3rd element: Description list.
  const base::ListValue* descriptions = nullptr;
  root_list->GetList(2, &descriptions);

  // 4th element: Disregard the query URL list for now.

  // Reset suggested relevance information.
  results->verbatim_relevance = -1;

  // 5th element: Optional key-value pairs from the Suggest server.
  const base::ListValue* types = nullptr;
  const base::ListValue* relevances = nullptr;
  const base::ListValue* suggestion_details = nullptr;
  const base::ListValue* subtype_identifiers = nullptr;
  const base::DictionaryValue* extras = nullptr;
  int prefetch_index = -1;
  if (root_list->GetDictionary(4, &extras)) {
    extras->GetList("google:suggesttype", &types);

    // Discard this list if its size does not match that of the suggestions.
    if (extras->GetList("google:suggestrelevance", &relevances) &&
        (relevances->GetSize() != results_list->GetSize()))
      relevances = nullptr;
    extras->GetInteger("google:verbatimrelevance",
                       &results->verbatim_relevance);

    // Check if the active suggest field trial (if any) has triggered either
    // for the default provider or keyword provider.
    results->field_trial_triggered = false;
    extras->GetBoolean("google:fieldtrialtriggered",
                       &results->field_trial_triggered);

    const base::DictionaryValue* client_data = nullptr;
    if (extras->GetDictionary("google:clientdata", &client_data) && client_data)
      client_data->GetInteger("phi", &prefetch_index);

    if (extras->GetList("google:suggestdetail", &suggestion_details) &&
        suggestion_details->GetSize() != results_list->GetSize())
      suggestion_details = nullptr;

    // Get subtype identifiers.
    if (extras->GetList("google:subtypeid", &subtype_identifiers) &&
        subtype_identifiers->GetSize() != results_list->GetSize()) {
      subtype_identifiers = nullptr;
    }

    // Store the metadata that came with the response in case we need to pass it
    // along with the prefetch query to Instant.
    JSONStringValueSerializer json_serializer(&results->metadata);
    json_serializer.Serialize(*extras);
  }

  // Clear the previous results now that new results are available.
  results->suggest_results.clear();
  results->navigation_results.clear();
  results->prefetch_image_urls.clear();

  base::string16 suggestion;
  std::string type;
  int relevance = default_result_relevance;
  const base::string16& trimmed_input =
      base::CollapseWhitespace(input.text(), false);
  for (size_t index = 0; results_list->GetString(index, &suggestion); ++index) {
    // Google search may return empty suggestions for weird input characters,
    // they make no sense at all and can cause problems in our code.
    if (suggestion.empty())
      continue;

    // Apply valid suggested relevance scores; discard invalid lists.
    if (relevances != nullptr && !relevances->GetInteger(index, &relevance))
      relevances = nullptr;
    AutocompleteMatchType::Type match_type =
        AutocompleteMatchType::SEARCH_SUGGEST;
    int subtype_identifier = 0;
    if (subtype_identifiers) {
      subtype_identifiers->GetInteger(index, &subtype_identifier);
    }
    if (types && types->GetString(index, &type))
      match_type = GetAutocompleteMatchType(type);
    const base::DictionaryValue* suggestion_detail = nullptr;
    std::string deletion_url;

    if (suggestion_details &&
        suggestion_details->GetDictionary(index, &suggestion_detail)) {
      suggestion_detail->GetString("du", &deletion_url);
    }

    if ((match_type == AutocompleteMatchType::NAVSUGGEST) ||
        (match_type == AutocompleteMatchType::NAVSUGGEST_PERSONALIZED)) {
      // Do not blindly trust the URL coming from the server to be valid.
      GURL url(url_formatter::FixupURL(base::UTF16ToUTF8(suggestion),
                                       std::string()));
      if (url.is_valid()) {
        base::string16 title;
        if (descriptions != nullptr)
          descriptions->GetString(index, &title);
        results->navigation_results.push_back(NavigationResult(
            scheme_classifier, url, match_type, subtype_identifier, title,
            deletion_url, is_keyword_result, relevance, relevances != nullptr,
            input.text()));
      }
    } else {
      base::string16 match_contents = suggestion;
      if ((match_type == AutocompleteMatchType::CALCULATOR) &&
          !suggestion.compare(0, 2, base::UTF8ToUTF16("= "))) {
        // Calculator results include a "= " prefix but we don't want to include
        // this in the search terms.
        suggestion.erase(0, 2);
        // Additionally, on larger (non-phone) form factors, we don't want to
        // display it in the suggestion contents either, because those devices
        // display a suggestion type icon that looks like a '='.
        if (ui::GetDeviceFormFactor() != ui::DEVICE_FORM_FACTOR_PHONE)
          match_contents.erase(0, 2);
      }

      base::string16 match_contents_prefix;
      base::string16 annotation;
      base::string16 answer_contents;
      base::string16 answer_type_str;
      std::unique_ptr<SuggestionAnswer> answer;
      std::string image_dominant_color;
      std::string image_url;
      std::string suggest_query_params;

      if (suggestion_details) {
        suggestion_details->GetDictionary(index, &suggestion_detail);
        if (suggestion_detail) {
          suggestion_detail->GetString("t", &match_contents);
          suggestion_detail->GetString("mp", &match_contents_prefix);
          // Error correction for bad data from server.
          if (match_contents.empty())
            match_contents = suggestion;
          suggestion_detail->GetString("a", &annotation);
          suggestion_detail->GetString("dc", &image_dominant_color);
          suggestion_detail->GetString("i", &image_url);
          suggestion_detail->GetString("q", &suggest_query_params);

          if (!image_url.empty()) {
            results->prefetch_image_urls.push_back(GURL(image_url));
          }

          // Extract the Answer, if provided.
          const base::DictionaryValue* answer_json = nullptr;
          if (suggestion_detail->GetDictionary("ansa", &answer_json) &&
              suggestion_detail->GetString("ansb", &answer_type_str)) {
            bool answer_parsed_successfully = false;
            answer = SuggestionAnswer::ParseAnswer(answer_json);
            int answer_type = 0;
            if (answer && base::StringToInt(answer_type_str, &answer_type)) {
              base::UmaHistogramSparse("Omnibox.AnswerParseType", answer_type);
              answer_parsed_successfully = true;

              answer->set_type(answer_type);
              answer->AddImageURLsTo(&results->prefetch_image_urls);

              std::string contents;
              base::JSONWriter::Write(*answer_json, &contents);
              answer_contents = base::UTF8ToUTF16(contents);
            } else {
              answer_type_str = base::string16();
            }
            UMA_HISTOGRAM_BOOLEAN("Omnibox.AnswerParseSuccess",
                                  answer_parsed_successfully);
          }
        }
      }

      bool should_prefetch = static_cast<int>(index) == prefetch_index;
      results->suggest_results.push_back(SuggestResult(
          base::CollapseWhitespace(suggestion, false), match_type,
          subtype_identifier, base::CollapseWhitespace(match_contents, false),
          match_contents_prefix, annotation, answer_contents, answer_type_str,
          std::move(answer), suggest_query_params, deletion_url,
          image_dominant_color, image_url, is_keyword_result, relevance,
          relevances != nullptr, should_prefetch, trimmed_input));
    }
  }
  results->relevances_from_server = relevances != nullptr;
  return true;
}
