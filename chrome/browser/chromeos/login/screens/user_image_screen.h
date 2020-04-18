// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_USER_IMAGE_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_USER_IMAGE_SCREEN_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/camera_presence_notifier.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/users/avatar/user_image_sync_observer.h"
#include "chrome/browser/image_decoder.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"

namespace base {
class Timer;
class Value;
}  // namespace base

namespace policy {
class PolicyChangeRegistrar;
}

namespace chromeos {

class ScreenManager;
class UserImageManager;
class UserImageView;

class UserImageScreen : public BaseScreen,
                        public CameraPresenceNotifier::Observer,
                        public ImageDecoder::ImageRequest,
                        public user_manager::UserManager::Observer,
                        public UserImageSyncObserver::Observer {
 public:
  static UserImageScreen* Get(ScreenManager* manager);

  UserImageScreen(BaseScreenDelegate* base_screen_delegate,
                  UserImageView* view);
  ~UserImageScreen() override;

  // Called when the UI ready to be shown.
  void OnScreenReady();

  // Called when the user accepts a photo as their login user image.
  void OnPhotoTaken(const std::string& raw_data);

  // Called when some image was selected. |is_user_selection| indicates if
  // it was user selection or image was selected programmatically.
  void OnImageSelected(const std::string& image_type,
                       const std::string& image_url,
                       bool is_user_selection);

  // Called when user accepts currently selected image.
  void OnImageAccepted();

  // Called when |view| is being destroyed.
  void OnViewDestroyed(UserImageView* view);

  bool user_selected_image() const { return user_has_selected_image_; }

 private:
  // Must be kept synced with |NewUserPriorityPrefsSyncResult| enum from
  // histograms.xml.
  enum class SyncResult {
    SUCCEEDED,
    TIMED_OUT,
    // Keeps a number of different sync results. Should be the last in the list.
    COUNT
  };

  // BaseScreen implementation:
  void Show() override;
  void Hide() override;

  // CameraPresenceNotifier::Observer implementation:
  void OnCameraPresenceCheckDone(bool is_camera_present) override;

  // ImageDecoder::ImageRequest implementation:
  void OnImageDecoded(const SkBitmap& decoded_image) override;
  void OnDecodeImageFailed() override;

  // user_manager::UserManager::Observer implementation:
  void OnUserImageChanged(const user_manager::User& user) override;
  void OnUserProfileImageUpdateFailed(const user_manager::User& user) override;
  void OnUserProfileImageUpdated(const user_manager::User& user,
                                 const gfx::ImageSkia& profile_image) override;

  // UserImageSyncObserver::Observer implementation:
  void OnInitialSync(bool local_image_updated) override;

  // Called when whaiting for sync timed out.
  void OnSyncTimeout();

  bool IsWaitingForSync() const;

  // Called when the policy::key::kUserAvatarImage policy changes while the
  // screen is being shown. If the policy is set, closes the screen because the
  // user is not allowed to override a policy-set image.
  void OnUserImagePolicyChanged(const base::Value* previous,
                                const base::Value* current);

  // Returns current user.
  const user_manager::User* GetUser();

  // Returns UserImageManager for the current user.
  UserImageManager* GetUserImageManager();

  // Returns UserImageSyncObserver for the current user.
  UserImageSyncObserver* GetSyncObserver();

  // Called when it's decided not to skip the screen.
  void HideCurtain();

  // Closes the screen.
  void ExitScreen();

  // Reports sync duration and result to UMA.
  void ReportSyncResult(SyncResult timed_out) const;

  std::unique_ptr<policy::PolicyChangeRegistrar> policy_registrar_;

  UserImageView* view_ = nullptr;

  // Last user photo, if taken.
  gfx::ImageSkia user_photo_;

  // Data for |user_photo_|.
  scoped_refptr<base::RefCountedBytes> user_photo_data_;

  // If |true|, decoded photo should be immediately accepted (i.e., both
  // HandleTakePhoto and HandleImageAccepted have already been called but we're
  // still waiting for  photo image decoding to finish.
  bool accept_photo_after_decoding_ = false;

  // Index of the selected user image.
  int selected_image_ = user_manager::User::USER_IMAGE_INVALID;

  // If screen ready to be shown.
  bool is_screen_ready_ = false;

  // True if user has explicitly selected some image.
  bool user_has_selected_image_ = false;

  // Timer used for waiting for user image sync.
  std::unique_ptr<base::Timer> sync_timer_;

  // The time when we started wait for user image sync.
  base::Time sync_waiting_start_time_;

  DISALLOW_COPY_AND_ASSIGN(UserImageScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_USER_IMAGE_SCREEN_H_
