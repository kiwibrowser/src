// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.IntentService;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

/**
 * Service that handles the action of clicking on the WebApk disclosure notification.
 */
public class WebApkDisclosureNotificationService extends IntentService {
    private static final String TAG = "WebApkDisclosureNotificationService";

    private static final String ACTION_HIDE_DISCLOSURE =
            "org.chromium.chrome.browser.webapps.HIDE_DISCLOSURE";

    private static final String EXTRA_WEBAPP_ID = "webapp_id";

    static PendingIntent getDeleteIntent(Context context, String webApkPackageName) {
        Intent intent = new Intent(context, WebApkDisclosureNotificationService.class);
        intent.setAction(ACTION_HIDE_DISCLOSURE);
        intent.putExtra(EXTRA_WEBAPP_ID, webApkPackageName);
        return PendingIntent.getService(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    /** Empty public constructor needed by Android. */
    public WebApkDisclosureNotificationService() {
        super(TAG);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String webappId = intent.getStringExtra(EXTRA_WEBAPP_ID);
        WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(webappId);
        if (storage != null) storage.setDismissedDisclosure();
    }
}
