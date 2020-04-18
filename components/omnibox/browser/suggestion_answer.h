// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_SUGGESTION_ANSWER_H_
#define COMPONENTS_OMNIBOX_BROWSER_SUGGESTION_ANSWER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
}

// Structured representation of the JSON payload of a suggestion with an answer.
// An answer has exactly two image lines, so called because they are a
// combination of text and an optional image URL.  Each image line has 1 or more
// text fields, each of which is required to contain a string and an integer
// type.  The text fields are contained in a non-empty vector and two optional
// named properties, referred to as "additional text" and "status text".
//
// When represented in the UI, these elements should be styled and laid out
// according to the specification at https://goto.google.com/ais_api.
//
// Each of the three classes has either an explicit or implicit copy
// constructor to support copying answer values (via SuggestionAnswer::copy) as
// members of SuggestResult and AutocompleteMatch.
class SuggestionAnswer {
 public:
  class TextField;
  typedef std::vector<TextField> TextFields;
  typedef std::vector<GURL> URLs;

  // These values are named and numbered to match a specification at go/ais_api.
  // The values are only used for answer results.
  enum TextType {
    // Deprecated: ANSWER = 1,
    // Deprecated: HEADLINE = 2,
    TOP_ALIGNED = 3,
    // Deprecated: DESCRIPTION = 4,
    DESCRIPTION_NEGATIVE = 5,
    DESCRIPTION_POSITIVE = 6,
    // Deprecated: MORE_INFO = 7,
    SUGGESTION = 8,
    // Deprecated: SUGGESTION_POSITIVE = 9,
    // Deprecated: SUGGESTION_NEGATIVE = 10,
    // Deprecated: SUGGESTION_LINK = 11,
    // Deprecated: STATUS = 12,
    PERSONALIZED_SUGGESTION = 13,
    // Deprecated: IMMERSIVE_DESCRIPTION_TEXT = 14,
    // Deprecated: DATE_TEXT = 15,
    // Deprecated: PREVIEW_TEXT = 16,
    ANSWER_TEXT_MEDIUM = 17,
    ANSWER_TEXT_LARGE = 18,
    SUGGESTION_SECONDARY_TEXT_SMALL = 19,
    SUGGESTION_SECONDARY_TEXT_MEDIUM = 20,
  };

  class TextField {
   public:
    TextField();
    ~TextField();

    // Parses |field_json| and populates |text_field| with the contents.  If any
    // of the required elements is missing, returns false and leaves text_field
    // in a partially populated state.
    static bool ParseTextField(const base::DictionaryValue* field_json,
                               TextField* text_field);

    const base::string16& text() const { return text_; }
    int type() const { return type_; }
    bool has_num_lines() const { return has_num_lines_; }
    int num_lines() const { return num_lines_; }

    bool Equals(const TextField& field) const;

    // Estimates dynamic memory usage.
    // See base/trace_event/memory_usage_estimator.h for more info.
    size_t EstimateMemoryUsage() const;

   private:
    base::string16 text_;
    int type_;
    bool has_num_lines_;
    int num_lines_;

    FRIEND_TEST_ALL_PREFIXES(SuggestionAnswerTest, DifferentValuesAreUnequal);

    // No DISALLOW_COPY_AND_ASSIGN since we depend on the copy constructor in
    // SuggestionAnswer::copy and the assignment operator as values in vector.
  };

  class ImageLine {
   public:
    ImageLine();
    explicit ImageLine(const ImageLine& line);
    ~ImageLine();

    // Parses |line_json| and populates |image_line| with the contents.  If any
    // of the required elements is missing, returns false and leaves text_field
    // in a partially populated state.
    static bool ParseImageLine(const base::DictionaryValue* line_json,
                               ImageLine* image_line);

    const TextFields& text_fields() const { return text_fields_; }
    int num_text_lines() const { return num_text_lines_; }
    const TextField* additional_text() const { return additional_text_.get(); }
    const TextField* status_text() const { return status_text_.get(); }
    const GURL& image_url() const { return image_url_; }

    bool Equals(const ImageLine& line) const;

    // Returns a string appropriate for use as a readable representation of the
    // content of this line.
    base::string16 AccessibleText() const;

    // Estimates dynamic memory usage.
    // See base/trace_event/memory_usage_estimator.h for more info.
    size_t EstimateMemoryUsage() const;

   private:
    // Forbid assignment.
    ImageLine& operator=(const ImageLine&);

    TextFields text_fields_;
    int num_text_lines_;
    std::unique_ptr<TextField> additional_text_;
    std::unique_ptr<TextField> status_text_;
    GURL image_url_;

    FRIEND_TEST_ALL_PREFIXES(SuggestionAnswerTest, DifferentValuesAreUnequal);
  };

  SuggestionAnswer();
  SuggestionAnswer(const SuggestionAnswer& answer);
  ~SuggestionAnswer();

  // Parses |answer_json| and returns a SuggestionAnswer containing the
  // contents.  If the supplied data is not well formed or is missing required
  // elements, returns nullptr instead.
  static std::unique_ptr<SuggestionAnswer> ParseAnswer(
      const base::DictionaryValue* answer_json);

  // TODO(jdonnelly): Once something like std::optional<T> is available in base/
  // (see discussion at http://goo.gl/zN2GNy) remove this in favor of having
  // SuggestResult and AutocompleteMatch use optional<SuggestionAnswer>.
  static std::unique_ptr<SuggestionAnswer> copy(
      const SuggestionAnswer* source) {
    return base::WrapUnique(source ? new SuggestionAnswer(*source) : nullptr);
  }

  const GURL& image_url() const { return image_url_; }
  const ImageLine& first_line() const { return first_line_; }
  const ImageLine& second_line() const { return second_line_; }

  // Answer type accessors.  Valid types are non-negative and defined at
  // https://goto.google.com/visual_element_configuration.
  int type() const { return type_; }
  void set_type(int type) { type_ = type; }

  bool Equals(const SuggestionAnswer& answer) const;

  // Retrieves any image URLs appearing in this answer and adds them to |urls|.
  void AddImageURLsTo(URLs* urls) const;

  // Estimates dynamic memory usage.
  // See base/trace_event/memory_usage_estimator.h for more info.
  size_t EstimateMemoryUsage() const;

 private:
  // Forbid assignment.
  SuggestionAnswer& operator=(const SuggestionAnswer&);

  GURL image_url_;
  ImageLine first_line_;
  ImageLine second_line_;
  int type_;

  FRIEND_TEST_ALL_PREFIXES(SuggestionAnswerTest, DifferentValuesAreUnequal);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_SUGGESTION_ANSWER_H_
