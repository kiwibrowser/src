// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package org.chromium.customtabsdemos;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.customtabs.CustomTabsIntent;
import android.support.v4.app.NotificationCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

/**
 * This is the Activity that the Notification takes the user to.
 *
 * The Notification sends the user to this activity with an EXTRA_URL. Use it to call Chrome Custom
 * Tabs. The user will return to it when coming back from the web content.
 */
public class NotificationParentActivity extends AppCompatActivity implements View.OnClickListener {
    private static final int NOTIFICATION_ID  = 1;
    public static final String EXTRA_URL = "extra.url";

    private View mMessageTextView;
    private View mCreateNotificationButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_notification_parent);
        mMessageTextView = findViewById(R.id.back_to_app_message);
        mCreateNotificationButton = findViewById(R.id.create_notification);
        mCreateNotificationButton.setOnClickListener(this);
        startChromeCustomTab(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        startChromeCustomTab(intent);
    }

    @Override
    public void onClick(View v) {
        int viewId = v.getId();
        switch (viewId) {
            case R.id.create_notification:
                createAndShowNotification();
                finish();
                break;
            default:
                //Unknown view clicked
        }
    }

    private void createAndShowNotification() {
        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(this)
                        .setSmallIcon(R.drawable.abc_popup_background_mtrl_mult)
                        .setContentTitle(getString(R.string.notification_title))
                        .setContentText(getString(R.string.notification_text));

        Intent resultIntent = new Intent(
                this.getApplicationContext(), NotificationParentActivity.class);

        resultIntent.putExtra(
                NotificationParentActivity.EXTRA_URL, getString(R.string.notification_sample_url));

        resultIntent.setAction(Intent.ACTION_VIEW);

        PendingIntent pendingIntent = PendingIntent.getActivity(
                this.getApplicationContext(), 0, resultIntent, 0);

        mBuilder.setContentIntent(pendingIntent);
        mBuilder.setAutoCancel(true);
        NotificationManager mNotificationManager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        // mId allows you to update the notification later on.
        mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
    }

    private void startChromeCustomTab(Intent intent) {
        String url = intent.getStringExtra(EXTRA_URL);
        if (url != null) {
            Uri uri = Uri.parse(url);

            int tabcolor = getResources().getColor(R.color.primaryColor);
            CustomTabsIntent customTabsIntent = new CustomTabsIntent.Builder()
                    .setToolbarColor(tabcolor)
                    .build();
            CustomTabActivityHelper.openCustomTab(
                    this, customTabsIntent, uri, new WebviewFallback());

            mMessageTextView.setVisibility(View.VISIBLE);
            mCreateNotificationButton.setVisibility(View.GONE);
        } else {
            mMessageTextView.setVisibility(View.GONE);
            mCreateNotificationButton.setVisibility(View.VISIBLE);
        }
    }
}
