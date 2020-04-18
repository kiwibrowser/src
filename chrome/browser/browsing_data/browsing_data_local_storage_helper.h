// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_LOCAL_STORAGE_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_LOCAL_STORAGE_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <map>
#include <set>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "content/public/browser/dom_storage_context.h"
#include "url/gurl.h"

class Profile;

// This class fetches local storage information and provides a
// means to delete the data associated with an origin.
class BrowsingDataLocalStorageHelper
    : public base::RefCounted<BrowsingDataLocalStorageHelper> {
 public:
  // Contains detailed information about local storage.
  struct LocalStorageInfo {
    LocalStorageInfo(const GURL& origin_url,
                     int64_t size,
                     base::Time last_modified);
    ~LocalStorageInfo();

    GURL origin_url;
    int64_t size;
    base::Time last_modified;
  };

  using FetchCallback =
      base::Callback<void(const std::list<LocalStorageInfo>&)>;

  explicit BrowsingDataLocalStorageHelper(Profile* profile);

  // Starts the fetching process, which will notify its completion via
  // callback. This must be called only in the UI thread.
  virtual void StartFetching(const FetchCallback& callback);

  // Deletes the local storage for the |origin_url|. |callback| is called when
  // the deletion is sent to the database and |StartFetching()| doesn't return
  // entries for |origin_url| anymore.
  virtual void DeleteOrigin(const GURL& origin_url, base::OnceClosure callback);

 protected:
  friend class base::RefCounted<BrowsingDataLocalStorageHelper>;
  virtual ~BrowsingDataLocalStorageHelper();

  content::DOMStorageContext* dom_storage_context_;  // Owned by the profile

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowsingDataLocalStorageHelper);
};

// This class is a thin wrapper around BrowsingDataLocalStorageHelper that does
// not fetch its information from the local storage tracker, but gets them
// passed as a parameter during construction.
class CannedBrowsingDataLocalStorageHelper
    : public BrowsingDataLocalStorageHelper {
 public:
  explicit CannedBrowsingDataLocalStorageHelper(Profile* profile);

  // Add a local storage to the set of canned local storages that is returned
  // by this helper.
  void AddLocalStorage(const GURL& origin_url);

  // Clear the list of canned local storages.
  void Reset();

  // True if no local storages are currently stored.
  bool empty() const;

  // Returns the number of local storages currently stored.
  size_t GetLocalStorageCount() const;

  // Returns the set of origins that use local storage.
  const std::set<GURL>& GetLocalStorageInfo() const;

  // BrowsingDataLocalStorageHelper implementation.
  void StartFetching(const FetchCallback& callback) override;
  void DeleteOrigin(const GURL& origin_url,
                    base::OnceClosure callback) override;

 private:
  ~CannedBrowsingDataLocalStorageHelper() override;

  std::set<GURL> pending_local_storage_info_;

  DISALLOW_COPY_AND_ASSIGN(CannedBrowsingDataLocalStorageHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_LOCAL_STORAGE_HELPER_H_
