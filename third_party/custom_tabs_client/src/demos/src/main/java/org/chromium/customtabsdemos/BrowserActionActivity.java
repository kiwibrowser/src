// Copyright 2018 Google Inc. All Rights Reserved.
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

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.customtabs.CustomTabsIntent;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;

import java.util.ArrayList;

import androidx.browser.browseractions.BrowserActionItem;
import androidx.browser.browseractions.BrowserActionsIntent;

public class BrowserActionActivity extends AppCompatActivity {

    private final static String URL = "https://www.google.com/";
    private final static String ALT_URL = "https://example.com/";

    private CustomTabActivityHelper mCustomTabActivityHelper;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_browser_action);

        mCustomTabActivityHelper = new CustomTabActivityHelper();

        final Button openLinkButton = findViewById(R.id.open_link);
        openLinkButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openCustomTabsLink(Uri.parse(URL));
            }
        });
        openLinkButton.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                openBrowserActionsLink(Uri.parse(URL), Uri.parse(ALT_URL));
                return true;
            }
        });
        openIntentLink(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        openIntentLink(intent);
        super.onNewIntent(intent);
        setIntent(intent);
    }

    private void openIntentLink(final Intent intent) {
        if (Intent.ACTION_VIEW.equals(intent.getAction())) {
            final Uri url = intent.getData();
            if (url != null) {
                openCustomTabsLink(url);
            }
        }
    }

    protected void openCustomTabsLink(final Uri url) {
        final CustomTabsIntent intent =
                new CustomTabsIntent.Builder(mCustomTabActivityHelper.getSession()).build();
        intent.launchUrl(BrowserActionActivity.this, url);
    }

    protected void openBrowserActionsLink(final Uri url, final Uri altUrl) {
        final Intent viewAltUrlIntent = new Intent(Intent.ACTION_VIEW, altUrl)
                .setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP)
                .setComponent(
                        new ComponentName(
                                this,
                                BrowserActionActivity.class));

        final BrowserActionItemList actions = new BrowserActionItemList(this)
                .add(R.string.label_open_alternate_link,
                        PendingIntent.getActivity(this,
                                0,
                                viewAltUrlIntent,
                                0),
                        R.drawable.ic_chrome_reader_mode_black_24dp);


        BrowserActionsIntent.openBrowserAction(
                BrowserActionActivity.this,
                url,
                BrowserActionsIntent.URL_TYPE_NONE,
                actions.items(),
                null);
    }

    public static class BrowserActionItemList {
        private final ArrayList<BrowserActionItem> mItems = new ArrayList<>();
        private final Context context;

        public BrowserActionItemList(@NonNull Context context) {
            this.context = context.getApplicationContext();
        }

        public ArrayList<BrowserActionItem> items() {
            return mItems;
        }

        public BrowserActionItemList add(@StringRes int titleId, @NonNull PendingIntent action,
                @DrawableRes int iconId) {
            mItems.add(new BrowserActionItem(context.getString(titleId), action, iconId));
            return this;
        }

        public BrowserActionItemList add(@StringRes int titleId, @NonNull PendingIntent action) {
            mItems.add(new BrowserActionItem(context.getString(titleId), action));
            return this;
        }

    }
}
