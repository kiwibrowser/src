// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_COUNTERS_MEDIA_LICENSES_COUNTER_H_
#define CHROME_BROWSER_BROWSING_DATA_COUNTERS_MEDIA_LICENSES_COUNTER_H_

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"
#include "url/gurl.h"

class Profile;

// MediaLicensesCounter is used to determine the number of origins that
// have persistent data (used by EME).
class MediaLicensesCounter : public browsing_data::BrowsingDataCounter {
 public:
  // MediaLicenseResult is the result of counting the number of origins
  // that have persistent data. It also contains one of the origins so
  // that a message can displayed providing an example of the origins that
  // hold content licenses.
  class MediaLicenseResult : public FinishedResult {
   public:
    MediaLicenseResult(const MediaLicensesCounter* source,
                       const std::set<GURL>& origins);
    ~MediaLicenseResult() override;

    const std::string& GetOneOrigin() const;

   private:
    std::string one_origin_;
  };

  static std::unique_ptr<MediaLicensesCounter> Create(Profile* profile);

  ~MediaLicensesCounter() override;

  const char* GetPrefName() const override;

 protected:
  explicit MediaLicensesCounter(Profile* profile);

  Profile* profile_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaLicensesCounter);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_COUNTERS_MEDIA_LICENSES_COUNTER_H_
