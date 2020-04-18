// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTION_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTION_H_

#include "url/gurl.h"

namespace contextual_suggestions {

// Struct containing the data for a single contextual content suggestion.
struct ContextualSuggestion {
  ContextualSuggestion();
  ContextualSuggestion(const ContextualSuggestion&);
  ContextualSuggestion(ContextualSuggestion&&) noexcept;
  ~ContextualSuggestion();

  // The ID identifying the suggestion.
  std::string id;

  // Title of the suggestion.
  std::string title;

  // The URL of the suggested page.
  GURL url;

  // The name of the content's publisher.
  std::string publisher_name;

  // Summary or relevant extract from the content.
  std::string snippet;

  // Identifier of the image to display alongside the suggestion. This id can
  // be used to construct a URL to the image.
  std::string image_id;

  // As above, but for identifying the favicon for the site the suggestion
  // resides on.
  std::string favicon_image_id;

  // The favicon URL for the suggestion.
  std::string favicon_image_url;
};

// Allows compact, precise construction of a ContextualSuggestion. Its main
// purpose is to avoid using a confusing 6 param helper whose argument
// order has to be guessed at.
class SuggestionBuilder {
 public:
  SuggestionBuilder(const GURL& url);

  SuggestionBuilder& Title(const std::string& title);
  SuggestionBuilder& PublisherName(const std::string& publisher_name);
  SuggestionBuilder& Snippet(const std::string& snippet);
  SuggestionBuilder& ImageId(const std::string& image_id);
  SuggestionBuilder& FaviconImageId(const std::string& favicon_image_id);
  SuggestionBuilder& FaviconImageUrl(const std::string& favicon_image_url);
  ContextualSuggestion Build();

 private:
  ContextualSuggestion suggestion_;
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTION_H_
