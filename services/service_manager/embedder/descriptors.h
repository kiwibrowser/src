// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_DESCRIPTORS_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_DESCRIPTORS_H_

namespace service_manager {

// This is a list of global descriptor keys to be used with the
// base::GlobalDescriptors object (see base/posix/global_descriptors.h)
enum {
  kCrashDumpSignal = 0,
  kSandboxIPCChannel,  // https://chromium.googlesource.com/chromium/src/+/master/docs/linux_sandbox_ipc.md
  kMojoIPCChannel,
  kFieldTrialDescriptor,

  // The first key that embedders can use to register descriptors (see
  // base/posix/global_descriptors.h).
  kFirstEmbedderDescriptor,
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_DESCRIPTORS_H_