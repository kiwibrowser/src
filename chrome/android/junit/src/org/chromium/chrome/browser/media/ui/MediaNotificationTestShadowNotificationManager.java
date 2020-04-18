// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.ui;

import static org.mockito.Mockito.mock;

import android.app.Notification;
import android.app.NotificationManager;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowNotificationManager;

/**
 * Dummy Robolectric shadow for Android NotificationManager for MediaNotification tests.
 */
@Implements(NotificationManager.class)
public class MediaNotificationTestShadowNotificationManager extends ShadowNotificationManager {
    public static final NotificationManager sMockObserver;

    static {
        sMockObserver = mock(NotificationManager.class);
    }

    @Implementation
    @Override
    public void notify(int id, Notification notification) {
        sMockObserver.notify(id, notification);
        super.notify(id, notification);
    }
}
