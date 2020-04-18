/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.support.customtabs.trusted;

import android.app.Notification;
import android.service.notification.StatusBarNotification;

public class TestTrustedWebActivityService extends TrustedWebActivityService {
    public static final int SMALL_ICON_ID = 666;
    public static final StatusBarNotification[] NOTIFICATIONS =
            new StatusBarNotification[] { null, null, null };

    @Override
    protected boolean notifyNotificationWithChannel(String platformTag, int platformId,
            Notification notification, String channelName) {
        return true;
    }

    @Override
    protected void cancelNotification(String platformTag, int platformId) {
    }

    @Override
    protected StatusBarNotification[] getActiveNotifications() {
        return NOTIFICATIONS;
    }

    @Override
    protected int getSmallIconId() {
        return SMALL_ICON_ID;
    }
}
