// Copyright 2017 Google Inc. All Rights Reserved.
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

package org.chromium.twa.svgomg;

import android.net.Uri;
import android.os.Bundle;
import android.support.customtabs.CustomTabsIntent;
import android.support.v7.app.AppCompatActivity;

public class TwaLauncherActivity extends AppCompatActivity
        implements TwaSessionHelper.TwaSessionCallback {

    private static final String TWA_ORIGIN = "https://svgomg.firebaseapp.com";
    private static final String TARGET_URL = TWA_ORIGIN;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_twa_launcher);

        Uri originUri = Uri.parse(TWA_ORIGIN);
        TwaSessionHelper twaSessionHelper = TwaSessionHelper.getInstance();
        twaSessionHelper.setTwaSessionCallback(this);
        twaSessionHelper.bindService(this, originUri);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        TwaSessionHelper twaSessionHelper = TwaSessionHelper.getInstance();
        twaSessionHelper.setTwaSessionCallback(null);
    }

    public void openTwa() {
        TwaSessionHelper twaSessionHelper = TwaSessionHelper.getInstance();

        // Set an empty transition from TwaLauncherActivity to the TWA splash screen.
        CustomTabsIntent customTabsIntent = twaSessionHelper.createIntentBuilder()
                .setStartAnimations(this, 0, 0)
                .build();

        Uri openUri = Uri.parse(TARGET_URL);
        twaSessionHelper.openTwa(this, customTabsIntent, openUri);
    }

    @Override
    public void onTwaSessionReady() {
        openTwa();
    }

    @Override
    public void onTwaSessionDestroyed() {
    }

    @Override
    public void onTwaOpened() {
        finishAndRemoveTask();
    }
}
