// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUGGESTIONS_BLACKLIST_STORE_H_
#define COMPONENTS_SUGGESTIONS_BLACKLIST_STORE_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "components/suggestions/proto/suggestions.pb.h"
#include "url/gurl.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace suggestions {

// A helper class for reading, writing, modifying and applying a small URL
// blacklist, pending upload to the server. The class has a concept of time
// duration before which a blacklisted URL becomes candidate for upload to the
// server. Keep in mind most of the operations involve interaction with the disk
// (the profile's preferences). Note that the class should be used as a
// singleton for the upload candidacy to work properly.
class BlacklistStore {
 public:
  BlacklistStore(
      PrefService* profile_prefs,
      const base::TimeDelta& upload_delay = base::TimeDelta::FromSeconds(15));
  virtual ~BlacklistStore();

  // Returns true if successful or |url| was already in the blacklist. If |url|
  // was already in the blacklist, its blacklisting timestamp gets updated.
  virtual bool BlacklistUrl(const GURL& url);

  // Clears any blacklist data from the profile's preferences.
  virtual void ClearBlacklist();

  // Gets the time until any URL is ready for upload. Returns false if the
  // blacklist is empty.
  virtual bool GetTimeUntilReadyForUpload(base::TimeDelta* delta);

  // Gets the time until |url| is ready for upload. Returns false if |url| is
  // not part of the blacklist.
  virtual bool GetTimeUntilURLReadyForUpload(const GURL& url,
                                             base::TimeDelta* delta);

  // Sets |url| to a URL from the blacklist that is candidate for upload.
  // Returns false if there is no candidate for upload.
  virtual bool GetCandidateForUpload(GURL* url);

  // Removes |url| from the stored blacklist. Returns true if successful, false
  // on failure or if |url| was not in the blacklist. Note that this function
  // does not enforce a minimum time since blacklist before removal.
  virtual bool RemoveUrl(const GURL& url);

  // Applies the blacklist to |suggestions|.
  virtual void FilterSuggestions(SuggestionsProfile* suggestions);

  // Register BlacklistStore related prefs in the Profile prefs.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 protected:
  // Test seam. For simplicity of mock creation.
  BlacklistStore();

  // Loads the blacklist data from the Profile preferences into
  // |blacklist|. If there is a problem with loading, the pref value is
  // cleared, false is returned and |blacklist| is cleared. If successful,
  // |blacklist| will contain the loaded data and true is returned.
  bool LoadBlacklist(SuggestionsBlacklist* blacklist);

  // Stores the provided |blacklist| to the Profile preferences, using
  // a base64 encoding of its protobuf serialization.
  bool StoreBlacklist(const SuggestionsBlacklist& blacklist);

 private:
  // The pref service used to persist the suggestions blacklist.
  PrefService* pref_service_;

  // Delay after which a URL becomes candidate for upload, measured from the
  // last time the URL was added.
  base::TimeDelta upload_delay_;

  // The times at which URLs were blacklisted. Used to determine when a URL is
  // valid for server upload. Guaranteed to contain URLs that are not ready for
  // upload. Might not contain URLs that are ready for upload.
  std::map<std::string, base::TimeTicks> blacklist_times_;

  DISALLOW_COPY_AND_ASSIGN(BlacklistStore);
};

}  // namespace suggestions

#endif  // COMPONENTS_SUGGESTIONS_BLACKLIST_STORE_H_
