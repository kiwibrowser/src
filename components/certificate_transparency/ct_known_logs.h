// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_CT_KNOWN_LOGS_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_CT_KNOWN_LOGS_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "build/build_config.h"

namespace base {
class Time;
}  // namespace base

namespace certificate_transparency {

struct CTLogInfo {
  // The DER-encoded SubjectPublicKeyInfo for the log.
  const char* log_key;
  // The length, in bytes, of |log_key|.
  size_t log_key_length;
  // The user-friendly log name.
  // Note: This will not be translated.
  const char* log_name;
  // The DNS API endpoint for the log.
  // This is used as the parent domain for all queries about the log.
  // https://github.com/google/certificate-transparency-rfcs/blob/master/dns/draft-ct-over-dns.md.
  const char* log_dns_domain;
};

// Returns information about all known logs, which includes those that are
// presently qualified for inclusion and logs which were previously qualified,
// but have since been disqualified. To determine the status of a given log
// (via its log ID), use |IsLogDisqualified()|.
std::vector<CTLogInfo> GetKnownLogs();

// Returns true if the log identified by |log_id| (the SHA-256 hash of the
// log's DER-encoded SPKI) is operated by Google.
bool IsLogOperatedByGoogle(base::StringPiece log_id);

// Returns true if the log identified by |log_id| (the SHA-256 hash of the
// log's DER-encoded SPKI) has been disqualified, and sets
// |*disqualification_date| to the date of disqualification. Any SCTs that
// are embedded in certificates issued after |*disqualification_date| should
// not be trusted, nor contribute to any uniqueness or freshness
// requirements.
bool IsLogDisqualified(base::StringPiece log_id,
                       base::Time* disqualification_date);

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_CT_KNOWN_LOGS_H_
