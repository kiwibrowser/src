// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_WEEKLY_ACTIVITY_STORAGE_H_
#define COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_WEEKLY_ACTIVITY_STORAGE_H_

#include <string>
#include <unordered_map>

#include "base/macros.h"

namespace contextual_search {

// An abstract class that stores weekly user interaction data in device-specific
// integer storage. Only a limited storage window is supported, set through the
// constructor. Allows callers to read and write user actions to persistent
// storage on the device by overriding the ReadStorage and WriteStorage calls.
// A user view of some UX is an "Impression", and user interaction is considered
// a "Click" even if the triggering gesture was something else.  Together they
// produce the Click-Through-Rate, or CTR.
class WeeklyActivityStorage {
 public:
  // Constructs an instance that will manage at least |weeks_needed| weeks of
  // data.
  WeeklyActivityStorage(int weeks_needed);
  virtual ~WeeklyActivityStorage();

  // Advances the accessible storage range to end at the given |week_number|.
  // Since only a limited number of storage weeks are supported, advancing to
  // a different week makes data from weeks than the range size inaccessible.
  // This must be called for each week before reading or writing any data
  // for that week.
  // HasData will return true for all the weeks that still have accessible data.
  void AdvanceToWeek(int week_number);

  // Returns the number of clicks for the given week.
  int ReadClicks(int week_number);
  // Writes |value| into the number of clicks for the given |week_number|.
  void WriteClicks(int week_number, int value);

  // Returns the number of impressions for the given week.
  int ReadImpressions(int week_number);
  // Writes |value| into the number of impressions for the given |week_number|.
  void WriteImpressions(int week_number, int value);

  // Returns whether the given |week_number| has data, based on whether
  // InitData has ever been called for that week.
  bool HasData(int week_number);
  // Clears the click and impression counters for the given |week_number|.
  void ClearData(int week_number);

  // Reads and returns the value keyed by |storage_bucket|.
  // If there is no stored value associated with the given bucket then 0 is
  // returned.
  virtual int ReadStorage(std::string storage_bucket) = 0;
  // Overwrites the |value| to the storage bucket keyed by |storage_bucket|,
  // regardless of whether there is an existing value in the given bucket.
  virtual void WriteStorage(std::string storage_bucket, int value) = 0;

 private:
  // Returns the string key of the storage bin for the given week |which_week|.
  std::string GetWeekKey(int which_week);
  // Returns the string key for the "clicks" storage bin for the given week
  // |which_week|.
  std::string GetWeekClicksKey(int which_week);
  // Returns the string key for the "impressions" storage bin for the given week
  // |which_week|.
  std::string GetWeekImpressionsKey(int which_week);

  // Reads and returns the integer keyed by |storage_key|.
  // If there is no value for the given key then 0 is returned.
  int ReadInt(std::string storage_key);
  // Writes the integer |value| to the storage bucket keyed by |storage_key|.
  void WriteInt(std::string storage_key, int value);

  // Ensures that activity data is initialized for the given week |which_week|.
  void EnsureHasActivity(int which_week);

  // The number of weeks of data that this instance needs to support.
  int weeks_needed_;

  DISALLOW_COPY_AND_ASSIGN(WeeklyActivityStorage);
};

}  // namespace contextual_search

#endif  // COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_WEEKLY_ACTIVITY_STORAGE_H_
