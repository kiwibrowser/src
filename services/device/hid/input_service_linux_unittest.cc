// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <vector>

#include "base/files/file_descriptor_watcher_posix.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "services/device/hid/input_service_linux.h"
#include "services/device/public/mojom/input_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {
void OnGetDevices(base::OnceClosure quit_closure,
                  std::vector<mojom::InputDeviceInfoPtr> devices) {
  for (size_t i = 0; i < devices.size(); ++i)
    ASSERT_TRUE(!devices[i]->id.empty());

  std::move(quit_closure).Run();
}
}  // namespace

TEST(InputServiceLinux, Simple) {
  base::MessageLoopForIO message_loop;
  base::FileDescriptorWatcher file_descriptor_watcher(&message_loop);

  InputServiceLinux* service = InputServiceLinux::GetInstance();
  ASSERT_TRUE(service);
  base::RunLoop run_loop;
  service->GetDevices(base::BindOnce(&OnGetDevices, run_loop.QuitClosure()));
  run_loop.Run();
}

}  // namespace device
