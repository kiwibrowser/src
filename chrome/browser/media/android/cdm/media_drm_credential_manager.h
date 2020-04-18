// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_CREDENTIAL_MANAGER_H_
#define CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_CREDENTIAL_MANAGER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "media/base/android/media_drm_bridge.h"
#include "media/base/provision_fetcher.h"

// This class resets the media DRM credentials on Android.
//
// Implementation Note: We create a MediaDrmBridge to reset the credentials.
// If MediaDrmBridge creation failed, it's likely that the key system or
// security level is not supported, hence no credentials need to be reset.
// We treat this as a success. There's a slight chance that MediaDrmBridge
// creation failed due to other reasons, but that should not happen in normal
// cases.
class MediaDrmCredentialManager {
 public:
  static MediaDrmCredentialManager* GetInstance();

  typedef base::Callback<void(bool)> ResetCredentialsCB;

  // Called to reset the DRM credentials. (for Java)
  // Only clears credentials for Widevine.
  // TODO(ddorwin): This should accept a key system parameter so that this is
  // clear to the caller, which can call it repeatedly as necessary.
  // http://crbug.com/459400
  static void ResetCredentials(JNIEnv* env, jclass clazz, jobject callback);

  // Called to reset the DRM credentials. The result is returned in the
  // |reset_credentials_cb|.
  void ResetCredentials(const ResetCredentialsCB& reset_credentials_cb);

 private:
  friend struct base::DefaultSingletonTraits<MediaDrmCredentialManager>;
  typedef media::MediaDrmBridge::SecurityLevel SecurityLevel;

  MediaDrmCredentialManager();
  ~MediaDrmCredentialManager();

  // Callback function passed to MediaDrmBridge. It is called when credentials
  // reset is completed.
  void OnResetCredentialsCompleted(SecurityLevel security_level, bool success);

  // Resets DRM credentials for a particular |security_level|. The result is
  // returned asynchronously in OnResetCredentialsCompleted() function.
  void ResetCredentialsInternal(SecurityLevel security_level);

  // The MediaDrmBridge object used to perform the credential reset.
  scoped_refptr<media::MediaDrmBridge> media_drm_bridge_;

  // The callback provided by the caller.
  ResetCredentialsCB reset_credentials_cb_;

  DISALLOW_COPY_AND_ASSIGN(MediaDrmCredentialManager);
};

#endif  // CHROME_BROWSER_MEDIA_ANDROID_CDM_MEDIA_DRM_CREDENTIAL_MANAGER_H_
