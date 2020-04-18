// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_JID_UTIL_H_
#define REMOTING_SIGNALING_JID_UTIL_H_

#include <string>

namespace remoting {

// Normalizes the |jid| by converting case-insensitive parts (node and domain)
// to lower-case.
std::string NormalizeJid(const std::string& jid);

// Splits a JID into a bare JID and a resource suffix.  Either |full_jid|,
// |resource|, or both may be null.  If |full_jid| is already
// a bare JID, |resource| is set to the empty string.  Returns true if
// |full_jid| has a resource, false if not.
//
// e.g. "user@domain/resource" -> "user@domain", "resource", true
//      "user@domain"          -> "user@domain", "",         false
bool SplitJidResource(const std::string& full_jid,
                      std::string* bare_jid,
                      std::string* resource);

}  // namespace remoting

#endif  // REMOTING_SIGNALING_JID_UTIL_H_
