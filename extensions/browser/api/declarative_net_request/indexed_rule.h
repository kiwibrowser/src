// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_INDEXED_RULE_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_INDEXED_RULE_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/url_pattern_index/flat/url_pattern_index_generated.h"

namespace extensions {

namespace api {
namespace declarative_net_request {
struct Rule;
}  // namespace declarative_net_request
}  // namespace api

namespace declarative_net_request {

enum class ParseResult;

// An intermediate structure to store a Declarative Net Request API rule while
// indexing. This structure aids in the subsequent conversion to a flatbuffer
// UrlRule as specified by the url_pattern_index component.
struct IndexedRule {
  IndexedRule();
  ~IndexedRule();
  IndexedRule(IndexedRule&& other);
  IndexedRule& operator=(IndexedRule&& other);

  static ParseResult CreateIndexedRule(
      std::unique_ptr<extensions::api::declarative_net_request::Rule>
          parsed_rule,
      IndexedRule* indexed_rule);

  // These fields correspond to the attributes of a flatbuffer UrlRule, as
  // specified by the url_pattern_index component.
  uint32_t id = 0;
  uint32_t priority = 0;
  uint8_t options = url_pattern_index::flat::OptionFlag_NONE;
  uint16_t element_types = url_pattern_index::flat::ElementType_NONE;
  uint8_t activation_types = url_pattern_index::flat::ActivationType_NONE;
  url_pattern_index::flat::UrlPatternType url_pattern_type =
      url_pattern_index::flat::UrlPatternType_SUBSTRING;
  url_pattern_index::flat::AnchorType anchor_left =
      url_pattern_index::flat::AnchorType_NONE;
  url_pattern_index::flat::AnchorType anchor_right =
      url_pattern_index::flat::AnchorType_NONE;
  std::string url_pattern;
  // Lower-cased and sorted as required by the url_pattern_index component.
  std::vector<std::string> domains;
  std::vector<std::string> excluded_domains;

  // The redirect url, valid iff this is a redirect rule.
  std::string redirect_url;

  DISALLOW_COPY_AND_ASSIGN(IndexedRule);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_INDEXED_RULE_H_
