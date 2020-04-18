// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test.util;

import android.support.test.InstrumentationRegistry;

import org.chromium.android_webview.AwQuotaManagerBridge;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;

/**
 * This class provides common methods for AwQuotaManagerBridge related tests
 */
public class AwQuotaManagerBridgeTestUtil {
    public static AwQuotaManagerBridge getQuotaManagerBridge() throws Exception {
        return ThreadUtils.runOnUiThreadBlocking(() -> AwQuotaManagerBridge.getInstance());
    }

    private static class GetOriginsCallbackHelper extends CallbackHelper {
        private AwQuotaManagerBridge.Origins mOrigins;

        public void notifyCalled(AwQuotaManagerBridge.Origins origins) {
            mOrigins = origins;
            notifyCalled();
        }

        public AwQuotaManagerBridge.Origins getOrigins() {
            assert getCallCount() > 0;
            return mOrigins;
        }
    }

    public static AwQuotaManagerBridge.Origins getOrigins() throws Exception {
        final GetOriginsCallbackHelper callbackHelper = new GetOriginsCallbackHelper();
        final AwQuotaManagerBridge bridge = getQuotaManagerBridge();

        int callCount = callbackHelper.getCallCount();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> bridge.getOrigins(origins -> callbackHelper.notifyCalled(origins)));
        callbackHelper.waitForCallback(callCount);

        return callbackHelper.getOrigins();
    }

}
