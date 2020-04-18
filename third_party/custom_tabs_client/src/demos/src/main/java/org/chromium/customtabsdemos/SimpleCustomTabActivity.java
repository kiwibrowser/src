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

import android.net.Uri;
import android.os.Bundle;
import android.support.customtabs.CustomTabsIntent;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.EditText;

/**
 * The simplest way to use Chrome Custom Tabs. Without any customization or speeding process.
 */
public class SimpleCustomTabActivity extends AppCompatActivity implements View.OnClickListener {
    private EditText mUrlEditText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_custom_tab);

        findViewById(R.id.start_custom_tab).setOnClickListener(this);

        mUrlEditText = (EditText)findViewById(R.id.url);
    }

    @Override
    public void onClick(View v) {
        int viewId = v.getId();

        switch (viewId) {
            case R.id.start_custom_tab:
                String url = mUrlEditText.getText().toString();
                CustomTabsIntent customTabsIntent = new CustomTabsIntent.Builder().build();
                CustomTabActivityHelper.openCustomTab(
                        this, customTabsIntent, Uri.parse(url), new WebviewFallback());
                break;
            default:
                //Unknown View Clicked
        }
    }
}
