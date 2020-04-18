// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.services.gcm;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;

import com.google.android.gms.gcm.GcmListenerService;
import com.google.ipc.invalidation.ticl.android2.channel.AndroidGcmController;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.ProcessInitializationHandler;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;
import org.chromium.components.gcm_driver.GCMDriver;
import org.chromium.components.gcm_driver.GCMMessage;

/**
 * Receives Downstream messages and status of upstream messages from GCM.
 */
public class ChromeGcmListenerService extends GcmListenerService {
    private static final String TAG = "ChromeGcmListener";

    @Override
    public void onCreate() {
        ProcessInitializationHandler.getInstance().initializePreNative();
        super.onCreate();
    }

    @Override
    public void onMessageReceived(final String from, final Bundle data) {
        boolean hasCollapseKey = !TextUtils.isEmpty(data.getString("collapse_key"));
        GcmUma.recordDataMessageReceived(getApplicationContext(), hasCollapseKey);

        String invalidationSenderId = AndroidGcmController.get(this).getSenderId();
        if (from.equals(invalidationSenderId)) {
            AndroidGcmController.get(this).onMessageReceived(data);
            return;
        }

        final Context applicationContext = getApplicationContext();

        // Dispatch the message to the GCM Driver for native features.
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                GCMMessage message = null;
                try {
                    message = new GCMMessage(from, data);
                } catch (IllegalArgumentException e) {
                    Log.e(TAG, "Received an invalid GCM Message", e);
                    return;
                }

                scheduleOrDispatchMessageToDriver(applicationContext, message);
            }
        });
    }

    @Override
    public void onMessageSent(String msgId) {
        Log.d(TAG, "Message sent successfully. Message id: " + msgId);
        GcmUma.recordGcmUpstreamHistogram(getApplicationContext(), GcmUma.UMA_UPSTREAM_SUCCESS);
    }

    @Override
    public void onSendError(String msgId, String error) {
        Log.w(TAG, "Error in sending message. Message id: " + msgId + " Error: " + error);
        GcmUma.recordGcmUpstreamHistogram(getApplicationContext(), GcmUma.UMA_UPSTREAM_SEND_FAILED);
    }

    @Override
    public void onDeletedMessages() {
        // TODO(johnme): Ask GCM to include the subtype in this event.
        Log.w(TAG, "Push messages were deleted, but we can't tell the Service Worker as we don't"
                + "know what subtype (app ID) it occurred for.");
        GcmUma.recordDeletedMessages(getApplicationContext());
    }

    /**
     * Either schedules |message| to be dispatched through the Job Scheduler, which we use on
     * Android N and beyond, or immediately dispatches the message on other versions of Android.
     * Must be called on the UI thread both for the BackgroundTaskScheduler and for dispatching
     * the |message| to the GCMDriver.
     */
    static void scheduleOrDispatchMessageToDriver(Context context, GCMMessage message) {
        ThreadUtils.assertOnUiThread();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            Bundle extras = message.toBundle();

            // TODO(peter): Add UMA for measuring latency introduced by the BackgroundTaskScheduler.
            TaskInfo backgroundTask = TaskInfo.createOneOffTask(TaskIds.GCM_BACKGROUND_TASK_JOB_ID,
                                                      GCMBackgroundTask.class, 0 /* immediately */)
                                              .setExtras(extras)
                                              .build();

            BackgroundTaskSchedulerFactory.getScheduler().schedule(context, backgroundTask);

        } else {
            dispatchMessageToDriver(context, message);
        }
    }

    /**
     * To be called when a GCM message is ready to be dispatched. Will initialise the native code
     * of the browser process, and forward the message to the GCM Driver. Must be called on the UI
     * thread.
     */
    static void dispatchMessageToDriver(Context applicationContext, GCMMessage message) {
        ThreadUtils.assertOnUiThread();

        try {
            ChromeBrowserInitializer.getInstance(applicationContext).handleSynchronousStartup();
            GCMDriver.dispatchMessage(message);

        } catch (ProcessInitException e) {
            Log.e(TAG, "ProcessInitException while starting the browser process");

            // Since the library failed to initialize nothing in the application can work, so kill
            // the whole application as opposed to just this service.
            System.exit(-1);
        }
    }
}
