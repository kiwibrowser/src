// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_virtual_file_provider_client.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

FakeVirtualFileProviderClient::FakeVirtualFileProviderClient() = default;
FakeVirtualFileProviderClient::~FakeVirtualFileProviderClient() = default;

void FakeVirtualFileProviderClient::Init(dbus::Bus* bus) {}

void FakeVirtualFileProviderClient::OpenFile(int64_t size,
                                             OpenFileCallback callback) {
  std::string id;
  base::ScopedFD fd;
  if (size != expected_size_) {
    LOG(ERROR) << "Unexpected size " << size << " vs " << expected_size_;
  } else {
    id = result_id_;
    fd = std::move(result_fd_);
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), id, std::move(fd)));
}

}  // namespace chromeos
