// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/media/media_device_id_salt.h"

#include "base/base64.h"
#include "base/rand_util.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

MediaDeviceIDSalt::MediaDeviceIDSalt(PrefService* pref_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  media_device_id_salt_.Init(prefs::kMediaDeviceIdSalt, pref_service);
  if (media_device_id_salt_.GetValue().empty()) {
    media_device_id_salt_.SetValue(
        content::BrowserContext::CreateRandomMediaDeviceIDSalt());
  }
}

MediaDeviceIDSalt::~MediaDeviceIDSalt() {
}

std::string MediaDeviceIDSalt::GetSalt() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return media_device_id_salt_.GetValue();
}

void MediaDeviceIDSalt::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kMediaDeviceIdSalt, std::string());
}

void MediaDeviceIDSalt::Reset(PrefService* pref_service) {
  pref_service->SetString(
      prefs::kMediaDeviceIdSalt,
      content::BrowserContext::CreateRandomMediaDeviceIDSalt());
}
