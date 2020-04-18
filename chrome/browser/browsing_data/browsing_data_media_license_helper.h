// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_MEDIA_LICENSE_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_MEDIA_LICENSE_HELPER_H_

#include <stdint.h>
#include <list>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace storage {
class FileSystemContext;
}

// Defines an interface for classes that deal with aggregating and deleting
// media licenses.
// BrowsingDataMediaLicenseHelper instances for a specific profile should
// be created via the static Create method. Each instance will lazily fetch
// data when a client calls StartFetching from the UI thread, and will
// notify the client via a supplied callback when the data is available.
//
// The client's callback is passed a list of MediaLicenseInfo objects
// containing usage information for each origin's media licenses.
class BrowsingDataMediaLicenseHelper
    : public base::RefCountedThreadSafe<BrowsingDataMediaLicenseHelper> {
 public:
  // Detailed information about a media license, including it's origin GURL
  // and the amount of data (in bytes).
  struct MediaLicenseInfo {
    MediaLicenseInfo(const GURL& origin,
                     int64_t size,
                     base::Time last_modified_time);
    MediaLicenseInfo(const MediaLicenseInfo& other);
    ~MediaLicenseInfo();

    // The origin for which the information is relevant.
    GURL origin;
    // Size (in bytes).
    int64_t size;
    // Last modified time.
    base::Time last_modified_time;
  };

  using FetchCallback =
      base::Callback<void(const std::list<MediaLicenseInfo>&)>;

  // Creates a BrowsingDataMediaLicenseHelper instance for the media
  // licenses stored in |profile|'s user data directory. The
  // BrowsingDataMediaLicenseHelper object will hold a reference to the
  // Profile that's passed in, but is not responsible for destroying it.
  //
  // The BrowsingDataMediaLicenseHelper will not change the profile itself,
  // but can modify data it contains (by removing media licenses).
  static BrowsingDataMediaLicenseHelper* Create(
      storage::FileSystemContext* file_system_context);

  // Starts the process of fetching media license data, which will call
  // |callback| upon completion, passing it a constant list of
  // MediaLicenseInfo objects. StartFetching must be called only in the UI
  // thread; the provided Callback will likewise be executed asynchronously
  // on the UI thread. Obtaining the data will occur asynchronously on the
  // FILE thread.
  virtual void StartFetching(const FetchCallback& callback) = 0;

  // Deletes any media licenses associated with |origin| from the disk.
  // Deletion will occur asynchronously on the FILE thread, but this function
  // must be called only on the UI thread.
  virtual void DeleteMediaLicenseOrigin(const GURL& origin) = 0;

 protected:
  friend class base::RefCountedThreadSafe<BrowsingDataMediaLicenseHelper>;

  BrowsingDataMediaLicenseHelper() {}
  virtual ~BrowsingDataMediaLicenseHelper() {}
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_MEDIA_LICENSE_HELPER_H_
