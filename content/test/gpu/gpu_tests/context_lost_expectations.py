# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from gpu_tests.gpu_test_expectations import GpuTestExpectations

# See the GpuTestExpectations class for documentation.

class ContextLostExpectations(GpuTestExpectations):
  def SetExpectations(self):
    # Sample Usage:
    # self.Fail('ContextLost_WebGLContextLostFromGPUProcessExit',
    #     ['mac', 'amd', ('nvidia', 0x1234)], bug=123)

    # AMD Radeon 6450
    self.Fail('ContextLost_WebGLContextLostFromGPUProcessExit',
        ['linux', ('amd', 0x6779)], bug=479975)

    # Win7 bots
    self.Flaky('ContextLost_WebGLContextLostFromGPUProcessExit',
               ['win7'], bug=603329)

    # Win8 Release and Debug NVIDIA bots.
    self.Skip('ContextLost_WebGLContextLostFromSelectElement',
              ['win8', 'nvidia'], bug=524808)

    # Flakily timing out on Win x64 Debug bot.
    # Unfortunately we can't identify this separately from the 32-bit bots.
    # Also unfortunately, the flaky retry mechanism doesn't work well in
    # this harness if the test times out. Skip it on this configuration for
    # now.
    self.Skip('ContextLost_WebGLContextLostFromQuantity',
              ['win', 'debug'], bug=628697)

    # Flaky on Mac 10.7 and 10.8 resulting in crashes during browser
    # startup, so skip this test in those configurations.
    self.Skip('ContextLost_WebGLContextLostFromSelectElement',
              ['mountainlion', 'debug'], bug=497411)
    self.Skip('ContextLost_WebGLContextLostFromSelectElement',
              ['lion', 'debug'], bug=498149)

    # 'Browser must support tab control' raised on Android
    self.Skip('GpuCrash_GPUProcessCrashesExactlyOncePerVisitToAboutGpuCrash',
              ['android'], bug=609629)
    self.Skip('ContextLost_WebGLContextLostFromGPUProcessExit',
              ['android'], bug=609629)
    self.Skip('ContextLost_WebGLContextLostInHiddenTab',
              ['android'], bug=609629)

    # Nexus 6
    # The Nexus 6 times out on these tests while waiting for the JS to complete
    self.Fail('ContextLost_WebGLContextLostFromLoseContextExtension',
              ['android', ('qualcomm', 'Adreno (TM) 420')], bug=611906)
    self.Fail('ContextLost_WebGLContextLostFromQuantity',
              ['android', ('qualcomm', 'Adreno (TM) 420')], bug=611906)

    # Android WebGLBlocked/Unblocked
    self.Fail('ContextLost_WebGLBlockedAfterJSNavigation',
              ['android', 'nvidia'], bug=832886)
    self.Flaky('ContextLost_WebGLBlockedAfterJSNavigation',
              ['android', 'qualcomm'], bug=832886)
    self.Fail('ContextLost_WebGLUnblockedAfterUserInitiatedReload',
              ['android', 'nvidia'], bug=832886)
    self.Flaky('ContextLost_WebGLUnblockedAfterUserInitiatedReload',
              ['android', 'qualcomm'], bug=832886)
