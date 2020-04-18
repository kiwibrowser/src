// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_SECURE_ORIGIN_WHITELIST_H_
#define CHROME_COMMON_SECURE_ORIGIN_WHITELIST_H_

#include <set>
#include <string>
#include <vector>

class PrefRegistrySimple;

namespace secure_origin_whitelist {

// Return a whitelist of origins and hostname patterns that need to be
// considered trustworthy.  The whitelist is given by
// kUnsafelyTreatInsecureOriginAsSecure command-line option. See
// https://www.w3.org/TR/powerful-features/#is-origin-trustworthy.
//
// The whitelist can contain origins and wildcard hostname patterns up to
// eTLD+1. For example, the list may contain "http://foo.com",
// "http://foo.com:8000", "*.foo.com", "*.foo.*.bar.com", and
// "http://*.foo.bar.com", but not "*.co.uk", "*.com", or "test.*.com". Hostname
// patterns must contain a wildcard somewhere (so "test.com" is not a valid
// pattern) and wildcards can only replace full components ("test*.foo.com" is
// not valid).
//
// Plain origins ("http://foo.com") are canonicalized when they are inserted
// into this list by converting to url::Origin and serializing. For hostname
// patterns, each component is individually canonicalized.
std::vector<std::string> GetWhitelist();

// Returns a whitelist of schemes that should bypass the Is Privileged Context
// check. See http://www.w3.org/TR/powerful-features/#settings-privileged.
std::set<std::string> GetSchemesBypassingSecureContextCheck();

// Register preferences for Secure Origin Whitelists.
void RegisterProfilePrefs(PrefRegistrySimple*);

}  // namespace secure_origin_whitelist

#endif  // CHROME_COMMON_SECURE_ORIGIN_WHITELIST_H_
