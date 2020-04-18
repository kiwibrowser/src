// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test;

import android.content.Context;

import org.chromium.base.test.BaseTestResult.PreTestHook;
import org.chromium.content.browser.ChildProcessLauncherHelper;

import java.lang.reflect.Method;

/**
 * PreTestHook used to register the ChildProcessAllocatorSettings annotation.
 *
 * TODO(yolandyan): convert this to TestRule once content tests are changed JUnit4
 * */
public final class ChildProcessAllocatorSettingsHook implements PreTestHook {
    @Override
    public void run(Context targetContext, Method testMethod) {
        ChildProcessAllocatorSettings annotation =
                testMethod.getAnnotation(ChildProcessAllocatorSettings.class);
        if (annotation != null) {
            ChildProcessLauncherHelper.setSandboxServicesSettingsForTesting(null /* factory */,
                    annotation.sandboxedServiceCount(), annotation.sandboxedServiceName());
        }
    }
}
