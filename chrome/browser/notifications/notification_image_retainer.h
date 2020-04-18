// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_IMAGE_RETAINER_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_IMAGE_RETAINER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"

class GURL;

namespace gfx {
class Image;
}  // namespace gfx

// The purpose of this class is to take data from memory, store it to disk as
// temp files and keep them alive long enough to hand over to external entities,
// such as the Action Center on Windows. The Action Center will read the files
// at some point in the future, which is why we can't do:
//   [write file] -> [show notification] -> [delete file].
//
// Also, on Windows, temp file deletion is not guaranteed and, since the images
// can potentially be large, this presents a problem because Chrome might then
// be leaving chunks of dead bits lying around on user’s computers during
// unclean shutdowns.
class NotificationImageRetainer {
 public:
  explicit NotificationImageRetainer(
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  virtual ~NotificationImageRetainer();

  // Stores an |image| from a particular profile (|profile_id|) and |origin| on
  // disk in a temporary (short-lived) file. Returns the path to the file
  // created, which will be valid for a few seconds only. It will be deleted
  // either after a short timeout or after a restart of Chrome (the next time
  // this function is called). The function returns an empty FilePath if file
  // creation fails.
  virtual base::FilePath RegisterTemporaryImage(const gfx::Image& image,
                                                const std::string& profile_id,
                                                const GURL& origin);

  // Sets whether to override temp file destruction time. If set to |true|, the
  // temp files will be scheduled for deletion right after their creation. If
  // |false|, the standard deletion delay will apply.
  static void OverrideTempFileLifespanForTesting(bool override);

 private:
  // Returns the temporary directory within the user data directory. The
  // regular temporary directory is not used to minimize the risk of files
  // getting deleted by accident. It is also not profile-bound because the
  // notification bridge handles images for multiple profiles and the separation
  // is handled by RegisterTemporaryImage.
  base::FilePath DetermineImageDirectory();

  // The path to where to store the temporary files.
  base::FilePath image_directory_;

  // Whether this class has initialized.
  bool initialized_ = false;

  // Whether to override the time to wait before deleting the temp files. For
  // testing use only.
  static bool override_file_destruction_;

  // The task runner to use.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(NotificationImageRetainer);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_IMAGE_RETAINER_H_
