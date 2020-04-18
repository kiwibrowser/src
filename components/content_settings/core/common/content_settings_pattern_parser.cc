// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/common/content_settings_pattern_parser.h"

#include <stddef.h>

#include "base/strings/string_util.h"
#include "url/url_constants.h"

namespace {

const char kDomainWildcard[] = "[*.]";
const size_t kDomainWildcardLength = 4;
const char kHostWildcard[] = "*";
const char kPathWildcard[] = "*";
const char kPortWildcard[] = "*";
const char kSchemeWildcard[] = "*";
const char kUrlPathSeparator[] = "/";
const char kUrlPortSeparator[] = ":";

class Component {
 public:
  Component() : start(0), len(0) {}
  Component(size_t s, size_t l) : start(s), len(l) {}

  bool IsNonEmpty() {
    return len > 0;
  }

  size_t start;
  size_t len;
};

}  // namespace

namespace content_settings {

void PatternParser::Parse(const std::string& pattern_spec,
                          ContentSettingsPattern::BuilderInterface* builder) {
  if (pattern_spec == "*") {
    builder->WithSchemeWildcard();
    builder->WithDomainWildcard();
    builder->WithPortWildcard();
    return;
  }

  // Initialize components for the individual patterns parts to empty
  // sub-strings.
  Component scheme_component;
  Component host_component;
  Component port_component;
  Component path_component;

  size_t start = 0;
  size_t current_pos = 0;

  if (pattern_spec.empty())
    return;

  // Test if a scheme pattern is in the spec.
  const std::string standard_scheme_separator(url::kStandardSchemeSeparator);
  current_pos = pattern_spec.find(standard_scheme_separator, start);
  if (current_pos != std::string::npos) {
    scheme_component = Component(start, current_pos);
    start = current_pos + standard_scheme_separator.size();
    current_pos = start;
  } else {
    current_pos = start;
  }

  if (start >= pattern_spec.size())
    return;  // Bad pattern spec.

  // Jump to the end of domain wildcards or an IPv6 addresses. IPv6 addresses
  // contain ':'. So first move to the end of an IPv6 address befor searching
  // for the ':' that separates the port form the host.
  if (pattern_spec[current_pos] == '[')
    current_pos = pattern_spec.find("]", start);

  if (current_pos == std::string::npos)
    return;  // Bad pattern spec.

  current_pos = pattern_spec.find(std::string(kUrlPortSeparator), current_pos);
  if (current_pos == std::string::npos) {
    // No port spec found
    current_pos = pattern_spec.find(std::string(kUrlPathSeparator), start);
    if (current_pos == std::string::npos) {
      current_pos = pattern_spec.size();
      host_component = Component(start, current_pos - start);
    } else {
      // Pattern has a path spec.
      host_component = Component(start, current_pos - start);
    }
    start = current_pos;
  } else {
    // Port spec found.
    host_component = Component(start, current_pos - start);
    start = current_pos + 1;
    if (start < pattern_spec.size()) {
      current_pos = pattern_spec.find(std::string(kUrlPathSeparator), start);
      if (current_pos == std::string::npos) {
        current_pos = pattern_spec.size();
      }
      port_component = Component(start, current_pos - start);
      start = current_pos;
    }
  }

  current_pos = pattern_spec.size();
  if (start < current_pos) {
    // Pattern has a path spec.
    path_component = Component(start, current_pos - start);
  }

  // Set pattern parts.
  std::string scheme;
  if (scheme_component.IsNonEmpty()) {
    scheme = pattern_spec.substr(scheme_component.start, scheme_component.len);
    if (scheme == kSchemeWildcard) {
      builder->WithSchemeWildcard();
    } else {
      builder->WithScheme(scheme);
    }
  } else {
    builder->WithSchemeWildcard();
  }

  if (host_component.IsNonEmpty()) {
    std::string host = pattern_spec.substr(host_component.start,
                                           host_component.len);
    if (host == kHostWildcard) {
      if (ContentSettingsPattern::IsNonWildcardDomainNonPortScheme(scheme)) {
        builder->Invalid();
        return;
      }

      builder->WithDomainWildcard();
    } else if (base::StartsWith(host, kDomainWildcard,
                                base::CompareCase::SENSITIVE)) {
      if (ContentSettingsPattern::IsNonWildcardDomainNonPortScheme(scheme)) {
        builder->Invalid();
        return;
      }

      host = host.substr(kDomainWildcardLength);
      builder->WithDomainWildcard();
      builder->WithHost(host);
    } else {
      // If the host contains a wildcard symbol then it is invalid.
      if (host.find(kHostWildcard) != std::string::npos) {
        builder->Invalid();
        return;
      }
      builder->WithHost(host);
    }
  }

  if (port_component.IsNonEmpty()) {
    if (ContentSettingsPattern::IsNonWildcardDomainNonPortScheme(scheme)) {
      builder->Invalid();
      return;
    }

    const std::string port = pattern_spec.substr(port_component.start,
                                                 port_component.len);
    if (port == kPortWildcard) {
      builder->WithPortWildcard();
    } else {
      // Check if the port string represents a valid port.
      for (size_t i = 0; i < port.size(); ++i) {
        if (!base::IsAsciiDigit(port[i])) {
          builder->Invalid();
          return;
        }
      }
      // TODO(markusheintz): Check port range.
      builder->WithPort(port);
    }
  } else {
    if (!ContentSettingsPattern::IsNonWildcardDomainNonPortScheme(scheme) &&
        scheme != url::kFileScheme)
      builder->WithPortWildcard();
  }

  if (path_component.IsNonEmpty()) {
    const std::string path = pattern_spec.substr(path_component.start,
                                                 path_component.len);
    if (path.substr(1) == kPathWildcard)
      builder->WithPathWildcard();
    else
      builder->WithPath(path);
  }
}

// static
std::string PatternParser::ToString(
    const ContentSettingsPattern::PatternParts& parts) {
  // Return the most compact form to support legacy code and legacy pattern
  // strings.
  if (parts.is_scheme_wildcard &&
      parts.has_domain_wildcard &&
      parts.host.empty() &&
      parts.is_port_wildcard)
    return "*";

  std::string str;
  if (!parts.is_scheme_wildcard)
    str += parts.scheme + url::kStandardSchemeSeparator;

  if (parts.scheme == url::kFileScheme) {
    if (parts.is_path_wildcard)
      return str + kUrlPathSeparator + kPathWildcard;
    return str + parts.path;
  }

  if (parts.has_domain_wildcard) {
    if (parts.host.empty())
      str += kHostWildcard;
    else
      str += kDomainWildcard;
  }
  str += parts.host;

  if (ContentSettingsPattern::IsNonWildcardDomainNonPortScheme(parts.scheme)) {
    str += parts.path.empty() ? std::string(kUrlPathSeparator) : parts.path;
    return str;
  }

  if (!parts.is_port_wildcard) {
    str += std::string(kUrlPortSeparator) + parts.port;
  }

  return str;
}

}  // namespace content_settings
