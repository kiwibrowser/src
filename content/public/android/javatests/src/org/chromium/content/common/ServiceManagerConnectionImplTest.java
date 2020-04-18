// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.common;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.content_shell_apk.ContentShellActivity;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;
import org.chromium.echo.mojom.Echo;
import org.chromium.echo.mojom.Echo.EchoStringResponse;
import org.chromium.echo.mojom.EchoConstants;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.services.service_manager.Connector;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Use the Connector and ServiceManagerConnectionImpl to connect echo service.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class ServiceManagerConnectionImplTest {
    @Rule
    public ContentShellActivityTestRule mActivityTestRule = new ContentShellActivityTestRule();

    private static final String TEST_URL = "about://blank";
    private static final String TEST_STRING = "abcdefghijklmnopqrstuvwxyz";

    private CallbackHelper mCallbackHelper;

    private class EchoStringResponseImpl implements EchoStringResponse {
        @Override
        public void call(String str) {
            // Verify they're equal.
            Assert.assertEquals("The sent out string should be same as received", str, TEST_STRING);
            mCallbackHelper.notifyCalled();
        }
    }

    /**
     * Connect Echo service, verify the sent and received data are same.
     * @throws InterruptedException
     * @throws ExecutionException
     */
    @Test
    @SmallTest
    public void testConnectEchoService()
            throws InterruptedException, ExecutionException, TimeoutException {
        final ContentShellActivity activity = mActivityTestRule.launchContentShellWithUrl(TEST_URL);
        mActivityTestRule.waitForActiveShellToBeDoneLoading();

        mCallbackHelper = new CallbackHelper();
        final int callCount = mCallbackHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                MessagePipeHandle handle =
                        ServiceManagerConnectionImpl.getConnectorMessagePipeHandle();
                Core core = CoreImpl.getInstance();
                Pair<Echo.Proxy, InterfaceRequest<Echo>> pair =
                        Echo.MANAGER.getInterfaceRequest(core);

                // Connect the Echo service via Connector.
                Connector connector = new Connector(handle);
                connector.bindInterface(
                        EchoConstants.SERVICE_NAME, Echo.MANAGER.getName(), pair.second);

                // Fire the echoString() mojo call.
                EchoStringResponse callback = new EchoStringResponseImpl();
                pair.first.echoString(TEST_STRING, callback);
            }
        });

        // Wait the response from Echo service.
        mCallbackHelper.waitForCallback(callCount);
    }
}
