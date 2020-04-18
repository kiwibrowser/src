// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.text.TextUtils;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * WebAPK's share handler Activity.
 */
public class ShareActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        long activityStartTime = SystemClock.elapsedRealtime();
        super.onCreate(savedInstanceState);

        String startUrl = extractShareTarget();
        if (TextUtils.isEmpty(startUrl)) {
            finish();
            return;
        }

        HostBrowserLauncher launcher = new HostBrowserLauncher(this, getIntent(), startUrl,
                WebApkConstants.SHORTCUT_SOURCE_SHARE, true /* forceNavigation */,
                activityStartTime);
        launcher.selectHostBrowserAndLaunch(() -> finish());
    }

    private String extractShareTarget() {
        ActivityInfo ai;
        try {
            ai = getPackageManager().getActivityInfo(
                    getComponentName(), PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
        if (ai == null || ai.metaData == null) {
            return null;
        }

        String shareTemplate = ai.metaData.getString(WebApkMetaDataKeys.SHARE_TEMPLATE);
        if (TextUtils.isEmpty(shareTemplate)) {
            return null;
        }

        String text = getIntent().getStringExtra(Intent.EXTRA_TEXT);
        String shareUrl = getIntent().getStringExtra(Intent.EXTRA_SUBJECT);
        Uri shareTemplateUri = Uri.parse(shareTemplate);
        return shareTemplateUri.buildUpon()
                .encodedQuery(
                        replacePlaceholders(shareTemplateUri.getEncodedQuery(), text, shareUrl))
                .encodedFragment(
                        replacePlaceholders(shareTemplateUri.getEncodedFragment(), text, shareUrl))
                .build()
                .toString();
    }

    /**
     * Replace {} placeholders in {@link toFill}. "{text}" and "{title}" are replaced with the
     * supplied strings. All other placeholders are deleted.
     */
    private String replacePlaceholders(String toFill, String text, String title) {
        if (toFill == null) {
            return null;
        }

        Pattern placeholder = Pattern.compile("\\{.*?\\}");
        Matcher matcher = placeholder.matcher(toFill);
        StringBuffer buffer = new StringBuffer();
        while (matcher.find()) {
            String replacement = "";
            if (matcher.group().equals("{text}")) {
                replacement = text;
            } else if (matcher.group().equals("{title}")) {
                replacement = title;
            }

            String encodedReplacement = "";
            if (replacement != null) {
                try {
                    encodedReplacement = URLEncoder.encode(replacement, "UTF-8");
                } catch (UnsupportedEncodingException e) {
                    // Should not be reached.
                }
            }
            matcher.appendReplacement(buffer, encodedReplacement);
        }
        matcher.appendTail(buffer);
        return buffer.toString();
    }
}
