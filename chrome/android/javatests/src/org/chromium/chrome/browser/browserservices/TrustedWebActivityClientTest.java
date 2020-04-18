// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browserservices;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.rule.ServiceTestRule;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.NotificationBuilderBase;
import org.chromium.chrome.browser.notifications.StandardNotificationBuilder;

import java.util.concurrent.TimeoutException;

/**
 * Tests the TrustedWebActivityClient.
 *
 * The control flow in these tests is a bit complicated since attempting to connect to a
 * test TrustedWebActivityService in the chrome_public_test results in a ClassLoader error (see
 * https://crbug.com/841178#c1). Therefore we must put the test TrustedWebActivityService in
 * chrome_public_test_support.
 *
 * We don't want to open up the TrustedWebActivityService API, so an additional Service (the
 * MessengerService) was created in chrome_public_test_support. This service can freely talk with
 * this test class.
 *
 * The general flow of these tests is as follows:
 * 1. Call a method on TrustedWebActivityClient.
 * 2. This calls through to TestTrustedWebActivityService.
 * 3. This calls a method on MessengerService.
 * 4. This sends a Message to ResponseHandler in this class.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class TrustedWebActivityClientTest {
    private static final Uri SCOPE = Uri.parse("https://www.example.com/notifications");
    private static final Origin ORIGIN = new Origin(SCOPE);
    private static final String NOTIFICATION_TAG = "tag";
    private static final int NOTIFICATION_ID = 123;

    private static final String TEST_SUPPORT_PACKAGE =
            "org.chromium.chrome.tests.support";
    private static final String MESSENGER_SERVICE_NAME =
            "org.chromium.chrome.browser.browserservices.MessengerService";

    @Rule public final ServiceTestRule mServiceTestRule = new ServiceTestRule();
    private ResponseHandler mResponseHandler;

    private TrustedWebActivityClient mClient;
    private Context mTargetContext;

    /**
     * A Handler that MessengerService will send messages to, reporting actions on
     * TestTrustedWebActivityService.
     */
    private static class ResponseHandler extends Handler {
        final CallbackHelper mResponderRegistered = new CallbackHelper();
        final CallbackHelper mGetSmallIconId = new CallbackHelper();
        final CallbackHelper mNotifyNotification = new CallbackHelper();
        final CallbackHelper mCancelNotification = new CallbackHelper();

        String mNotificationTag;
        int mNotificationId;
        String mNotificationChannel;

        public ResponseHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            // For some messages these will be null/zero. It should be fine because the messages
            // we care about these variables for will be called at the end of a sequence.
            mNotificationTag = msg.getData().getString(MessengerService.TAG_KEY);
            mNotificationId = msg.getData().getInt(MessengerService.ID_KEY);
            mNotificationChannel = msg.getData().getString(MessengerService.CHANNEL_KEY);

            switch (msg.what) {
                case MessengerService.MSG_RESPONDER_REGISTERED:
                    mResponderRegistered.notifyCalled();
                    break;
                case MessengerService.MSG_GET_SMALL_ICON_ID:
                    mGetSmallIconId.notifyCalled();
                    break;
                case MessengerService.MSG_NOTIFY_NOTIFICATION:
                    mNotifyNotification.notifyCalled();
                    break;
                case MessengerService.MSG_CANCEL_NOTIFICATION:
                    mCancelNotification.notifyCalled();
                    break;
                default:
                    Assert.fail("Unexpected message: " + msg.what);
            }
        }
    }

    @Before
    public void setUp() throws TimeoutException, RemoteException, InterruptedException {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        mClient = new TrustedWebActivityClient(mTargetContext);

        // TestTrustedWebActivityService is in the test support apk.
        TrustedWebActivityClient.registerClient(mTargetContext, ORIGIN, TEST_SUPPORT_PACKAGE);

        // The MessengerService lives in the same package as the TestTrustedWebActivityService.
        // We use it as a side channel to verify what the TestTrustedWebActivityService does.
        mResponseHandler = new ResponseHandler(ThreadUtils.getUiThreadLooper());

        // Launch the MessengerService.
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(TEST_SUPPORT_PACKAGE, MESSENGER_SERVICE_NAME));

        // Create a Messenger to talk to the MessengerService.
        Messenger messenger = new Messenger(mServiceTestRule.bindService(intent));

        // Create a Messenger for the MessengerService to respond to us.
        Messenger responseMessenger = new Messenger(mResponseHandler);
        Message message = Message.obtain();
        message.replyTo = responseMessenger;
        messenger.send(message);

        mResponseHandler.mResponderRegistered.waitForCallback(0);
    }

    /**
     * Tests that #notifyNotification:
     * - Gets the small icon id from the service (although it doesn't check that it's used).
     * - Gets the service to show the notification.
     * - Uses the provided tag and id and the default channel name.
     */
    @Test
    @SmallTest
    public void testNotifyNotification() throws TimeoutException, InterruptedException {
        Assert.assertEquals(0, mResponseHandler.mGetSmallIconId.getCallCount());
        Assert.assertEquals(0, mResponseHandler.mNotifyNotification.getCallCount());

        ThreadUtils.runOnUiThread(() -> {
            NotificationBuilderBase builder = new StandardNotificationBuilder(mTargetContext);
            mClient.notifyNotification(SCOPE, NOTIFICATION_TAG, NOTIFICATION_ID, builder);
        });

        mResponseHandler.mGetSmallIconId.waitForCallback(0);
        mResponseHandler.mNotifyNotification.waitForCallback(0);

        Assert.assertEquals(mResponseHandler.mNotificationTag, NOTIFICATION_TAG);
        Assert.assertEquals(mResponseHandler.mNotificationId, NOTIFICATION_ID);
        Assert.assertEquals(mResponseHandler.mNotificationChannel,
                mTargetContext.getResources().getString(
                        R.string.notification_category_group_general));
    }

    /**
     * Tests that #cancelNotification gets the service to cancel the notification, using the given
     * id and tag.
     */
    @Test
    @SmallTest
    public void testCancelNotification() throws TimeoutException, InterruptedException {
        Assert.assertEquals(0, mResponseHandler.mCancelNotification.getCallCount());

        ThreadUtils.runOnUiThread(() ->
            mClient.cancelNotification(SCOPE, NOTIFICATION_TAG, NOTIFICATION_ID));

        mResponseHandler.mCancelNotification.waitForCallback(0);

        Assert.assertEquals(mResponseHandler.mNotificationTag, NOTIFICATION_TAG);
        Assert.assertEquals(mResponseHandler.mNotificationId, NOTIFICATION_ID);
    }
}
