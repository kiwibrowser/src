// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_CSP_VALIDATOR_H_
#define EXTENSIONS_COMMON_CSP_VALIDATOR_H_

#include <string>

#include "extensions/common/manifest.h"

namespace extensions {

namespace csp_validator {

// Checks whether the given |policy| is legal for use in the extension system.
// This check just ensures that the policy doesn't contain any characters that
// will cause problems when we transmit the policy in an HTTP header.
bool ContentSecurityPolicyIsLegal(const std::string& policy);

// This specifies options for configuring which CSP directives are permitted in
// extensions.
enum Options {
  OPTIONS_NONE = 0,
  // Allows 'unsafe-eval' to be specified as a source in a directive.
  OPTIONS_ALLOW_UNSAFE_EVAL = 1 << 0,
  // Allow an object-src to be specified with any sources (i.e. it may contain
  // wildcards or http sources). Specifying this requires the CSP to contain
  // a plugin-types directive which restricts the plugins that can be loaded
  // to those which are fully sandboxed.
  OPTIONS_ALLOW_INSECURE_OBJECT_SRC = 1 << 1,
};

// Checks whether the given |policy| meets the minimum security requirements
// for use in the extension system.
//
// Ideally, we would like to say that an XSS vulnerability in the extension
// should not be able to execute script, even in the precense of an active
// network attacker.
//
// However, we found that it broke too many deployed extensions to limit
// 'unsafe-eval' in the script-src directive, so that is allowed as a special
// case for extensions. Platform apps disallow it.
//
// |options| is a bitmask of Options.
//
// If |warnings| is not NULL, any validation errors are appended to |warnings|.
// Returns the sanitized policy.
std::string SanitizeContentSecurityPolicy(
    const std::string& policy,
    int options,
    std::vector<InstallWarning>* warnings);

// Given the Content Security Policy of an app sandbox page, returns the
// effective CSP for that sandbox page.
//
// The effective policy restricts the page from loading external web content
// (frames and scripts) within the page. This is done through adding 'self'
// directive source to relevant CSP directive names.
//
// If |warnings| is not nullptr, any validation errors are appended to
// |warnings|.
std::string GetEffectiveSandoxedPageCSP(const std::string& policy,
                                        std::vector<InstallWarning>* warnings);

// Checks whether the given |policy| enforces a unique origin sandbox as
// defined by http://www.whatwg.org/specs/web-apps/current-work/multipage/
// the-iframe-element.html#attr-iframe-sandbox. The policy must have the
// "sandbox" directive, and the sandbox tokens must not include
// "allow-same-origin". Additional restrictions may be imposed depending on
// |type|.
bool ContentSecurityPolicyIsSandboxed(
    const std::string& policy, Manifest::Type type);

}  // namespace csp_validator

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_CSP_VALIDATOR_H_
