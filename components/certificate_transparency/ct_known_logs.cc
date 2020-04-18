// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/ct_known_logs.h"

#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <iterator>

#include "base/logging.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "crypto/sha2.h"

namespace certificate_transparency {

namespace {
#include "components/certificate_transparency/data/log_list-inc.cc"
}  // namespace

std::vector<CTLogInfo> GetKnownLogs() {
  // Add all qualified logs.
  std::vector<CTLogInfo> logs(std::begin(kCTLogList), std::end(kCTLogList));

  // Add all disqualified logs. Callers are expected to filter verified SCTs
  // via IsLogDisqualified().
  for (const auto& disqualified_log : kDisqualifiedCTLogList) {
    logs.push_back(disqualified_log.log_info);
  }

  return logs;
}

bool IsLogOperatedByGoogle(base::StringPiece log_id) {
  CHECK_EQ(log_id.size(), crypto::kSHA256Length);

  return std::binary_search(std::begin(kGoogleLogIDs), std::end(kGoogleLogIDs),
                            log_id.data(), [](const char* a, const char* b) {
                              return memcmp(a, b, crypto::kSHA256Length) < 0;
                            });
}

bool IsLogDisqualified(base::StringPiece log_id,
                       base::Time* disqualification_date) {
  CHECK_EQ(log_id.size(), base::size(kDisqualifiedCTLogList[0].log_id) - 1);

  auto* p = std::lower_bound(
      std::begin(kDisqualifiedCTLogList), std::end(kDisqualifiedCTLogList),
      log_id.data(),
      [](const DisqualifiedCTLogInfo& disqualified_log, const char* log_id) {
        return memcmp(disqualified_log.log_id, log_id, crypto::kSHA256Length) <
               0;
      });
  if (p == std::end(kDisqualifiedCTLogList) ||
      memcmp(p->log_id, log_id.data(), crypto::kSHA256Length) != 0) {
    return false;
  }

  *disqualification_date = base::Time::UnixEpoch() + p->disqualification_date;
  return true;
}

}  // namespace certificate_transparency
