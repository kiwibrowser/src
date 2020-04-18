# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Fields for use when working with a physical linux device connected locally
linux_device_hostname = "192.168.42.32"
linux_device_user = "potat"

linux_out_dir = "out/default"
fuchsia_out_dir = "out/fuchsia"

# A map of test target names to a list of test filters to be passed to
# --gtest_filter. Stick to *_perftests. Also, whoo implicit string
# joining!
test_targets = {
    "base:base_perftests": "-WaitableEventPerfTest.Throughput"
        ":MessageLoopPerfTest.PostTaskRate/1_Posting_Thread",
    "net:net_perftests": "",
}
