// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_POLICY_LOADER_IOS_H_
#define COMPONENTS_POLICY_CORE_COMMON_POLICY_LOADER_IOS_H_

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "components/policy/core/common/async_policy_loader.h"
#include "components/policy/policy_export.h"

namespace policy {

// A policy loader that loads policy from the managed app configuration
// introduced in iOS 7.
class POLICY_EXPORT PolicyLoaderIOS : public AsyncPolicyLoader {
 public:
  explicit PolicyLoaderIOS(
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~PolicyLoaderIOS() override;

  // AsyncPolicyLoader implementation.
  void InitOnBackgroundThread() override;
  std::unique_ptr<PolicyBundle> Load() override;
  base::Time LastModificationTime() override;

 private:
  void UserDefaultsChanged();

  // Loads the Chrome policies in |dictionary| into the given |bundle|.
  static void LoadNSDictionaryToPolicyBundle(NSDictionary* dictionary,
                                             PolicyBundle* bundle);

  // Used to manage the registration for NSNotificationCenter notifications.
  base::scoped_nsobject<id> notification_observer_;

  // Timestamp of the last notification.
  // Used to coalesce repeated notifications into a single Load() call.
  base::Time last_notification_time_;

  // Used to Bind() a WeakPtr to |this| for the callback passed to the
  // |notification_observer_|.
  base::WeakPtrFactory<PolicyLoaderIOS> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PolicyLoaderIOS);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_POLICY_LOADER_IOS_H_
