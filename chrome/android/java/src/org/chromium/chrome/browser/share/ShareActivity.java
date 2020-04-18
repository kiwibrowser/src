// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.share;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.util.IntentUtils;

import java.lang.ref.WeakReference;
import java.util.List;

/**
 * {@code ShareActivity} is the base class for share options, which
 * are activities that are shown in the share chooser. Activities subclassing
 * ShareActivity override featureIsEnabled, and handleShareAction.
 */
public abstract class ShareActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            Intent intent = getIntent();
            if (intent == null) return;
            if (!Intent.ACTION_SEND.equals(intent.getAction())) return;
            if (!IntentUtils.safeHasExtra(intent, ShareHelper.EXTRA_TASK_ID)) return;

            ChromeActivity triggeringActivity = getTriggeringActivity();
            if (triggeringActivity == null) return;

            handleShareAction(triggeringActivity);
        } finally {
            finish();
        }
    }

    /**
     * Returns the ChromeActivity that called the share intent picker.
     */
    private ChromeActivity getTriggeringActivity() {
        int triggeringTaskId =
                IntentUtils.safeGetIntExtra(getIntent(), ShareHelper.EXTRA_TASK_ID, 0);
        List<WeakReference<Activity>> activities = ApplicationStatus.getRunningActivities();
        ChromeActivity triggeringActivity = null;
        for (int i = 0; i < activities.size(); i++) {
            Activity activity = activities.get(i).get();
            if (activity == null) continue;

            if (activity.getTaskId() == triggeringTaskId && activity instanceof ChromeActivity) {
                return (ChromeActivity) activity;
            }
        }
        return null;
    }

    /**
     * Completes the share action.
     * Override this activity to implement desired share functionality.  The activity
     * will be destroyed immediately after this method is called.
     */
    protected abstract void handleShareAction(ChromeActivity triggeringActivity);
}