// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import android.app.Notification;
import android.support.test.InstrumentationRegistry;

import org.junit.Assert;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.preferences.website.ContentSetting;
import org.chromium.chrome.browser.preferences.website.NotificationInfo;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.util.browser.notifications.MockNotificationManagerProxy;
import org.chromium.chrome.test.util.browser.notifications.MockNotificationManagerProxy.NotificationEntry;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Base class for instrumentation tests using Web Notifications on Android.
 *
 * Web Notifications are only supported on Android JellyBean and beyond.
 */
public class NotificationTestRule extends ChromeActivityTestRule<ChromeTabbedActivity> {
    /** The maximum time to wait for a criteria to become valid. */
    private static final long MAX_TIME_TO_POLL_MS = scaleTimeout(6000);

    /** The polling interval to wait between checking for a satisfied criteria. */
    private static final long POLLING_INTERVAL_MS = 50;

    private MockNotificationManagerProxy mMockNotificationManager;
    private EmbeddedTestServer mTestServer;

    public NotificationTestRule() {
        super(ChromeTabbedActivity.class);
    }

    private void setUp() throws Exception {
        // The NotificationPlatformBridge must be overriden prior to the browser process starting.
        mMockNotificationManager = new MockNotificationManagerProxy();
        NotificationPlatformBridge.overrideNotificationManagerForTesting(mMockNotificationManager);
        startMainActivityFromLauncher();
        mTestServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());
    }

    private void tearDown() throws Exception {
        NotificationPlatformBridge.overrideNotificationManagerForTesting(null);
        mTestServer.stopAndDestroyServer();
    }

    /** Returns the test server. */
    public EmbeddedTestServer getTestServer() {
        return mTestServer;
    }

    /**
     * Returns the origin of the HTTP server the test is being ran on.
     */
    public String getOrigin() {
        return mTestServer.getURL("/");
    }

    /**
     * Sets the permission to use Web Notifications for the test HTTP server's origin to |setting|.
     */
    public void setNotificationContentSettingForCurrentOrigin(final ContentSetting setting)
            throws InterruptedException, TimeoutException {
        final String origin = getOrigin();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                // The notification content setting does not consider the embedder origin.
                NotificationInfo notificationInfo = new NotificationInfo(origin, "", false);
                notificationInfo.setContentSetting(setting);
            }
        });

        String permission = runJavaScriptCodeInCurrentTab("Notification.permission");
        if (setting == ContentSetting.ALLOW) {
            Assert.assertEquals("\"granted\"", permission);
        } else if (setting == ContentSetting.BLOCK) {
            Assert.assertEquals("\"denied\"", permission);
        } else {
            Assert.assertEquals("\"default\"", permission);
        }
    }

    /**
     * Shows a notification with |title| and |options|, waits until it has been displayed and then
     * returns the Notification object to the caller. Requires that only a single notification is
     * being displayed in the notification manager.
     *
     * @param title Title of the Web Notification to show.
     * @param options Optional map of options to include when showing the notification.
     * @return The Android Notification object, as shown in the framework.
     */
    public Notification showAndGetNotification(String title, String options)
            throws InterruptedException, TimeoutException {
        runJavaScriptCodeInCurrentTab("showNotification(\"" + title + "\", " + options + ");");
        return waitForNotification().notification;
    }

    /**
     * Waits until a notification has been displayed and then returns a NotificationEntry object to
     * the caller. Requires that only a single notification is displayed.
     *
     * @return The NotificationEntry object tracked by the MockNotificationManagerProxy.
     */
    public NotificationEntry waitForNotification() {
        waitForNotificationManagerMutation();
        List<NotificationEntry> notifications = getNotificationEntries();
        Assert.assertEquals(1, notifications.size());
        return notifications.get(0);
    }

    public List<NotificationEntry> getNotificationEntries() {
        return mMockNotificationManager.getNotifications();
    }

    /**
     * Waits for a mutation to occur in the mocked notification manager. This indicates that Chrome
     * called into Android to notify or cancel a notification.
     */
    public void waitForNotificationManagerMutation() {
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mMockNotificationManager.getMutationCountAndDecrement() > 0;
            }
        }, MAX_TIME_TO_POLL_MS, POLLING_INTERVAL_MS);
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        return super.apply(new Statement() {
            @Override
            public void evaluate() throws Throwable {
                setUp();
                base.evaluate();
                tearDown();
            }
        }, description);
    }
}
