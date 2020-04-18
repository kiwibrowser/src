#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""run_simpleperf_on_device.py:
    It downloads simpleperf to /data/local/tmp on device, and run it with all given arguments.
    It saves the time downloading simpleperf and using `adb shell` directly.
"""
import subprocess
import sys
from utils import *

def main():
    disable_debug_log()
    adb = AdbHelper()
    device_arch = adb.get_device_arch()
    simpleperf_binary = get_target_binary_path(device_arch, 'simpleperf')
    adb.check_run(['push', simpleperf_binary, '/data/local/tmp'])
    adb.check_run(['shell', 'chmod', 'a+x', '/data/local/tmp/simpleperf'])
    shell_cmd = 'cd /data/local/tmp && ./simpleperf ' + ' '.join(sys.argv[1:])
    sys.exit(subprocess.call([adb.adb_path, 'shell', shell_cmd]))

if __name__ == '__main__':
    main()