// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_STATUS_UPLOADER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_STATUS_UPLOADER_H_

#include <memory>

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/proto/device_management_backend.pb.h"

namespace base {
class SequencedTaskRunner;
}

namespace policy {

class CloudPolicyClient;
class DeviceStatusCollector;

// Class responsible for periodically uploading device status from the
// passed DeviceStatusCollector.
class StatusUploader : public MediaCaptureDevicesDispatcher::Observer {
 public:
  // Constructor. |client| must be registered and must stay
  // valid and registered through the lifetime of this StatusUploader
  // object.
  StatusUploader(CloudPolicyClient* client,
                 std::unique_ptr<DeviceStatusCollector> collector,
                 const scoped_refptr<base::SequencedTaskRunner>& task_runner,
                 base::TimeDelta default_upload_frequency);

  ~StatusUploader() override;

  // Returns the time of the last successful upload, or Time(0) if no upload
  // has ever happened.
  base::Time last_upload() const { return last_upload_; }

  // Returns true if session data upload (screenshots, logs, etc) is allowed.
  // This checks to ensure that the current session is a kiosk session, and
  // that no user input (keyboard, mouse, touch, audio/video) has been received.
  bool IsSessionDataUploadAllowed();

  // MediaCaptureDevicesDispatcher::Observer implementation
  void OnRequestUpdate(int render_process_id,
                       int render_frame_id,
                       content::MediaStreamType stream_type,
                       const content::MediaRequestState state) override;

  void ScheduleNextStatusUploadImmediately();

 private:
  // Callback invoked periodically to upload the device status from the
  // DeviceStatusCollector.
  void UploadStatus();

  // Called asynchronously by DeviceStatusCollector when status arrives
  void OnStatusReceived(
      std::unique_ptr<enterprise_management::DeviceStatusReportRequest>
          device_status,
      std::unique_ptr<enterprise_management::SessionStatusReportRequest>
          session_status);

  // Invoked once a status upload has completed.
  void OnUploadCompleted(bool success);

  // Helper method that figures out when the next status upload should
  // be scheduled.
  void ScheduleNextStatusUpload(bool immediately = false);

  // Updates the upload frequency from settings and schedules a new upload
  // if appropriate.
  void RefreshUploadFrequency();

  // CloudPolicyClient used to issue requests to the server.
  CloudPolicyClient* client_;

  // DeviceStatusCollector that provides status for uploading.
  std::unique_ptr<DeviceStatusCollector> collector_;

  // TaskRunner used for scheduling upload tasks.
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // How long to wait between status uploads.
  base::TimeDelta upload_frequency_;

  // Observer to changes in the upload frequency.
  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      upload_frequency_observer_;

  // The time the last upload was performed.
  base::Time last_upload_;

  // Callback invoked via a delay to upload device status.
  base::CancelableClosure upload_callback_;

  // True if there has been any captured media in this session.
  bool has_captured_media_;

  // Used to prevent a race condition where two status uploads are being
  // executed in parallel.
  bool status_upload_in_progress_ = false;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<StatusUploader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StatusUploader);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_STATUS_UPLOADER_H_
