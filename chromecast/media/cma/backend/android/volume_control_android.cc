// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/android/volume_control_android.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/no_destructor.h"
#include "chromecast/base/init_command_line_shlib.h"
#include "chromecast/base/serializers.h"
#include "chromecast/chromecast_buildflags.h"
#include "jni/VolumeControl_jni.h"
#if BUILDFLAG(ENABLE_VOLUME_TABLES_ACCESS)
#include "jni/VolumeMap_jni.h"
#endif

namespace chromecast {
namespace media {

namespace {

#if BUILDFLAG(ENABLE_VOLUME_TABLES_ACCESS)
float VolumeToDbFSByContentType(AudioContentType type, float volume) {
  return Java_VolumeMap_volumeToDbFs(base::android::AttachCurrentThread(),
                                     static_cast<int>(type), volume);
}

float DbFSToVolumeByContentType(AudioContentType type, float db) {
  return Java_VolumeMap_dbFsToVolume(base::android::AttachCurrentThread(),
                                     static_cast<int>(type), db);
}

#else  // Dummy versions.
float VolumeToDbFSByContentType(AudioContentType type, float volume) {
  return 1.0f;
}

float DbFSToVolumeByContentType(AudioContentType type, float db) {
  return 100;
}
#endif

// For the user of the VolumeControl, all volume values are in the volume table
// domain of kMedia (MUSIC). For volume types other than media, VolumeControl
// converts them internally into their proper volume table domains.
float MapIntoDifferentVolumeTableDomain(AudioContentType from_type,
                                        AudioContentType to_type,
                                        float level) {
  if (from_type == to_type) {
    return level;
  }
  float from_db = VolumeToDbFSByContentType(from_type, level);
  return DbFSToVolumeByContentType(to_type, from_db);
}

}  // namespace

VolumeControlAndroid& GetVolumeControl() {
  static base::NoDestructor<VolumeControlAndroid> volume_control;
  return *volume_control;
}

VolumeControlAndroid::VolumeControlAndroid()
    : thread_("VolumeControl"),
      initialize_complete_event_(
          base::WaitableEvent::ResetPolicy::MANUAL,
          base::WaitableEvent::InitialState::NOT_SIGNALED) {
  DCHECK(j_volume_control_.is_null());
  j_volume_control_.Reset(Java_VolumeControl_createVolumeControl(
      base::android::AttachCurrentThread(), reinterpret_cast<intptr_t>(this)));

  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  thread_.StartWithOptions(options);

  thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VolumeControlAndroid::InitializeOnThread,
                                base::Unretained(this)));
  initialize_complete_event_.Wait();
}

VolumeControlAndroid::~VolumeControlAndroid() {}

void VolumeControlAndroid::AddVolumeObserver(VolumeObserver* observer) {
  base::AutoLock lock(observer_lock_);
  volume_observers_.push_back(observer);
}

void VolumeControlAndroid::RemoveVolumeObserver(VolumeObserver* observer) {
  base::AutoLock lock(observer_lock_);
  volume_observers_.erase(
      std::remove(volume_observers_.begin(), volume_observers_.end(), observer),
      volume_observers_.end());
}

float VolumeControlAndroid::GetVolume(AudioContentType type) {
  base::AutoLock lock(volume_lock_);
  // The return level needs to be in the kMedia (MUSIC) volume table domain.
  return MapIntoDifferentVolumeTableDomain(type, AudioContentType::kMedia,
                                           volumes_[type]);
}

void VolumeControlAndroid::SetVolume(AudioContentType type, float level) {
  if (type == AudioContentType::kOther) {
    NOTREACHED() << "Can't set volume for content type kOther";
    return;
  }

  level = std::max(0.0f, std::min(level, 1.0f));
  // The input level value is in the kMedia (MUSIC) volume table domain.
  float mapped_level =
      MapIntoDifferentVolumeTableDomain(AudioContentType::kMedia, type, level);
  thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VolumeControlAndroid::SetVolumeOnThread,
                                base::Unretained(this), type, mapped_level,
                                false /* from_android */));
}

bool VolumeControlAndroid::IsMuted(AudioContentType type) {
  base::AutoLock lock(volume_lock_);
  return muted_[type];
}

void VolumeControlAndroid::SetMuted(AudioContentType type, bool muted) {
  if (type == AudioContentType::kOther) {
    NOTREACHED() << "Can't set mute state for content type kOther";
    return;
  }

  thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VolumeControlAndroid::SetMutedOnThread,
                                base::Unretained(this), type, muted,
                                false /* from_android */));
}

void VolumeControlAndroid::SetOutputLimit(AudioContentType type, float limit) {
  if (type == AudioContentType::kOther) {
    NOTREACHED() << "Can't set output limit for content type kOther";
    return;
  }

  // The input limit is in the kMedia (MUSIC) volume table domain.
  limit = std::max(0.0f, std::min(limit, 1.0f));
  float limit_db = VolumeToDbFSByContentType(AudioContentType::kMedia, limit);
  AudioSinkManager::Get()->SetOutputLimitDb(type, limit_db);
}

void VolumeControlAndroid::OnVolumeChange(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jint type,
    jfloat level) {
  thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&VolumeControlAndroid::ReportVolumeChangeOnThread,
                     base::Unretained(this), (AudioContentType)type, level));
}

void VolumeControlAndroid::OnMuteChange(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jint type,
    jboolean muted) {
  thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&VolumeControlAndroid::ReportMuteChangeOnThread,
                     base::Unretained(this), (AudioContentType)type, muted));
}

void VolumeControlAndroid::InitializeOnThread() {
  DCHECK(thread_.task_runner()->BelongsToCurrentThread());

  for (auto type : {AudioContentType::kMedia, AudioContentType::kAlarm,
                    AudioContentType::kCommunication}) {
#if BUILDFLAG(ENABLE_VOLUME_TABLES_ACCESS)
    Java_VolumeMap_dumpVolumeTables(base::android::AttachCurrentThread(),
                                    static_cast<int>(type));
#endif
    volumes_[type] =
        Java_VolumeControl_getVolume(base::android::AttachCurrentThread(),
                                     j_volume_control_, static_cast<int>(type));
    float volume_db = VolumeToDbFSByContentType(type, volumes_[type]);
    AudioSinkManager::Get()->SetTypeVolumeDb(type, volume_db);
    muted_[type] =
        Java_VolumeControl_isMuted(base::android::AttachCurrentThread(),
                                   j_volume_control_, static_cast<int>(type));
    LOG(INFO) << __func__ << ": Initial values for"
              << " type=" << static_cast<int>(type) << ": "
              << " volume=" << volumes_[type] << " (" << volume_db << ")"
              << " mute=" << muted_[type];
  }

  // The kOther content type should not have any type-wide volume control or
  // mute (volume control for kOther is per-stream only). Therefore, ensure
  // that the global volume and mute state fo kOther is initialized correctly
  // (100% volume, and not muted).
  SetVolumeOnThread(AudioContentType::kOther, 1.0f, false /* from_android */);
  SetMutedOnThread(AudioContentType::kOther, false, false /* from_android */);

  initialize_complete_event_.Signal();
}

void VolumeControlAndroid::SetVolumeOnThread(AudioContentType type,
                                             float level,
                                             bool from_android) {
  // Note: |level| is in the |type| volume table domain.
  DCHECK(thread_.task_runner()->BelongsToCurrentThread());
  {
    base::AutoLock lock(volume_lock_);
    if (level == volumes_[type]) {
      return;
    }
    volumes_[type] = level;
  }

  float level_db = VolumeToDbFSByContentType(type, level);
  LOG(INFO) << __func__ << ": level=" << level << " (" << level_db << ")";
  // Provide the type volume to the sink manager so it can properly calculate
  // the limiter multiplier. The volume is *not* applied by the sink though.
  AudioSinkManager::Get()->SetTypeVolumeDb(type, level_db);

  // Set proper volume in Android OS.
  if (!from_android) {
    Java_VolumeControl_setVolume(base::android::AttachCurrentThread(),
                                 j_volume_control_, static_cast<int>(type),
                                 level);
  }

  // Report new volume level to observers. Note that the reported value needs
  // to be in the kMedia (MUSIC) volume table domain.
  float media_level =
      MapIntoDifferentVolumeTableDomain(type, AudioContentType::kMedia, level);
  {
    base::AutoLock lock(observer_lock_);
    for (VolumeObserver* observer : volume_observers_) {
      observer->OnVolumeChange(type, media_level);
    }
  }
}

void VolumeControlAndroid::SetMutedOnThread(AudioContentType type,
                                            bool muted,
                                            bool from_android) {
  DCHECK(thread_.task_runner()->BelongsToCurrentThread());
  {
    base::AutoLock lock(volume_lock_);
    if (muted == muted_[type]) {
      return;
    }
    muted_[type] = muted;
  }

  if (!from_android) {
    Java_VolumeControl_setMuted(base::android::AttachCurrentThread(),
                                j_volume_control_, static_cast<int>(type),
                                muted);
  }

  {
    base::AutoLock lock(observer_lock_);
    for (VolumeObserver* observer : volume_observers_) {
      observer->OnMuteChange(type, muted);
    }
  }
}

void VolumeControlAndroid::ReportVolumeChangeOnThread(AudioContentType type,
                                                      float level) {
  DCHECK(thread_.task_runner()->BelongsToCurrentThread());
  if (type == AudioContentType::kOther) {
    // Volume for AudioContentType::kOther should stay at 1.0.
    Java_VolumeControl_setVolume(base::android::AttachCurrentThread(),
                                 j_volume_control_, static_cast<int>(type),
                                 1.0f);
    return;
  }

  SetVolumeOnThread(type, level, true /* from android */);
}

void VolumeControlAndroid::ReportMuteChangeOnThread(AudioContentType type,
                                                    bool muted) {
  DCHECK(thread_.task_runner()->BelongsToCurrentThread());
  if (type == AudioContentType::kOther) {
    // Mute state for AudioContentType::kOther should always be false.
    Java_VolumeControl_setMuted(base::android::AttachCurrentThread(),
                                j_volume_control_, static_cast<int>(type),
                                false);
    return;
  }

  SetMutedOnThread(type, muted, true /* from_android */);
}

//
// Implementation of VolumeControl as defined in public/volume_control.h
//

// static
void VolumeControl::Initialize(const std::vector<std::string>& argv) {
  // Nothing to do.
}

// static
void VolumeControl::Finalize() {
  // Nothing to do.
}

// static
void VolumeControl::AddVolumeObserver(VolumeObserver* observer) {
  GetVolumeControl().AddVolumeObserver(observer);
}

// static
void VolumeControl::RemoveVolumeObserver(VolumeObserver* observer) {
  GetVolumeControl().RemoveVolumeObserver(observer);
}

// static
float VolumeControl::GetVolume(AudioContentType type) {
  return GetVolumeControl().GetVolume(type);
}

// static
void VolumeControl::SetVolume(AudioContentType type, float level) {
  GetVolumeControl().SetVolume(type, level);
}

// static
bool VolumeControl::IsMuted(AudioContentType type) {
  return GetVolumeControl().IsMuted(type);
}

// static
void VolumeControl::SetMuted(AudioContentType type, bool muted) {
  GetVolumeControl().SetMuted(type, muted);
}

// static
void VolumeControl::SetOutputLimit(AudioContentType type, float limit) {
  GetVolumeControl().SetOutputLimit(type, limit);
}

// static
float VolumeControl::VolumeToDbFS(float volume) {
  // The volume value is the kMedia (MUSIC) volume table domain.
  return VolumeToDbFSByContentType(AudioContentType::kMedia, volume);
}

// static
float VolumeControl::DbFSToVolume(float db) {
  // The db value is the kMedia (MUSIC) volume table domain.
  return DbFSToVolumeByContentType(AudioContentType::kMedia, db);
}

}  // namespace media
}  // namespace chromecast
