// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestion.h"

namespace contextual_suggestions {

ContextualSuggestion::ContextualSuggestion() = default;

ContextualSuggestion::ContextualSuggestion(const ContextualSuggestion& other) =
    default;

// MSVC doesn't support defaulted move constructors, so we have to define it
// ourselves.
ContextualSuggestion::ContextualSuggestion(
    ContextualSuggestion&& other) noexcept
    : id(std::move(other.id)),
      title(std::move(other.title)),
      url(std::move(other.url)),
      publisher_name(std::move(other.publisher_name)),
      snippet(std::move(other.snippet)),
      image_id(std::move(other.image_id)),
      favicon_image_id(std::move(other.favicon_image_id)),
      favicon_image_url(std::move(other.favicon_image_url)) {}

ContextualSuggestion::~ContextualSuggestion() = default;

SuggestionBuilder::SuggestionBuilder(const GURL& url) {
  suggestion_.url = url;
  suggestion_.id = url.spec();
}

SuggestionBuilder& SuggestionBuilder::Title(const std::string& title) {
  suggestion_.title = title;
  return *this;
}

SuggestionBuilder& SuggestionBuilder::PublisherName(
    const std::string& publisher_name) {
  suggestion_.publisher_name = publisher_name;
  return *this;
}

SuggestionBuilder& SuggestionBuilder::Snippet(const std::string& snippet) {
  suggestion_.snippet = snippet;
  return *this;
}

SuggestionBuilder& SuggestionBuilder::ImageId(const std::string& image_id) {
  suggestion_.image_id = image_id;
  return *this;
}

SuggestionBuilder& SuggestionBuilder::FaviconImageId(
    const std::string& favicon_image_id) {
  suggestion_.favicon_image_id = favicon_image_id;
  return *this;
}

SuggestionBuilder& SuggestionBuilder::FaviconImageUrl(
    const std::string& favicon_image_url) {
  suggestion_.favicon_image_url = favicon_image_url;
  return *this;
}

ContextualSuggestion SuggestionBuilder::Build() {
  return std::move(suggestion_);
}

}  // namespace contextual_suggestions
