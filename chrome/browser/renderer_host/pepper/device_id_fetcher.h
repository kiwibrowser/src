// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_DEVICE_ID_FETCHER_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_DEVICE_ID_FETCHER_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_instance.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

// This class allows asynchronously fetching a unique device ID. The callback
// passed in when calling Start() will be called when the ID has been fetched
// or on error.
class DeviceIDFetcher : public base::RefCountedThreadSafe<DeviceIDFetcher> {
 public:
  typedef base::Callback<void(const std::string&, int32_t)> IDCallback;

  explicit DeviceIDFetcher(int render_process_id);

  // Schedules the request operation. Returns false if a request is in progress,
  // true otherwise.
  bool Start(const IDCallback& callback);

  // Called to register the |kEnableDRM| and |kDRMSalt| preferences.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* prefs);

  // Return the path where the legacy device ID is stored (for ChromeOS only).
  static base::FilePath GetLegacyDeviceIDPath(
      const base::FilePath& profile_path);

 private:
  ~DeviceIDFetcher();

  // Checks the preferences for DRM (whether DRM is enabled and getting the drm
  // salt) on the UI thread.
  void CheckPrefsOnUIThread();

  // Compute the device ID on the UI thread with the given salt and machine ID.
  void ComputeOnUIThread(const std::string& salt,
                         const std::string& machine_id);

  // Legacy method used to get the device ID for ChromeOS.
  void LegacyComputeAsync(const base::FilePath& profile_path,
                          const std::string& salt);

  // Runs the callback passed into Start() on the IO thread with the device ID
  // or the empty string on failure.
  void RunCallbackOnIOThread(const std::string& id, int32_t result);

  friend class base::RefCountedThreadSafe<DeviceIDFetcher>;

  // The callback to run when the ID has been fetched.
  IDCallback callback_;

  // Whether a request is in progress.
  bool in_progress_;

  int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(DeviceIDFetcher);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_DEVICE_ID_FETCHER_H_
