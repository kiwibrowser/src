// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_

namespace extensions {
namespace declarative_net_request {

// Permission name.
extern const char kAPIPermission[];

// Minimum valid value of a declarative rule ID.
constexpr int kMinValidID = 1;

// Minimum valid value of a declarative rule priority.
constexpr int kMinValidPriority = 1;

// Default priority used for rules when the priority is not explicity provided
// by an extension.
constexpr int kDefaultPriority = 1;

// Keys used in rules.
extern const char kIDKey[];
extern const char kPriorityKey[];
extern const char kRuleConditionKey[];
extern const char kRuleActionKey[];
extern const char kUrlFilterKey[];
extern const char kIsUrlFilterCaseSensitiveKey[];
extern const char kDomainsKey[];
extern const char kExcludedDomainsKey[];
extern const char kResourceTypesKey[];
extern const char kExcludedResourceTypesKey[];
extern const char kDomainTypeKey[];
extern const char kRuleActionTypeKey[];
extern const char kRedirectUrlKey[];

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
