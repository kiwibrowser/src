// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Pair;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.payments.PaymentAppFactory.PaymentAppCreatedCallback;
import org.chromium.chrome.browser.payments.PaymentAppFactory.PaymentAppFactoryAddition;
import org.chromium.components.payments.PaymentManifestDownloader;
import org.chromium.components.payments.PaymentManifestParser;
import org.chromium.content_public.browser.WebContents;
import org.chromium.payments.mojom.PaymentMethodData;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Builds instances of payment apps based on installed third party Android payment apps. */
public class AndroidPaymentAppFactory implements PaymentAppFactoryAddition {
    @Override
    public void create(WebContents webContents, Map<String, PaymentMethodData> methodData,
            boolean mayCrawlUnused, PaymentAppCreatedCallback callback) {
        AndroidPaymentAppFinder.find(webContents, methodData.keySet(),
                new PaymentManifestWebDataService(), new PaymentManifestDownloader(),
                new PaymentManifestParser(), new PackageManagerDelegate(), callback);
    }

    /**
     * Checks whether there are Android payment apps on device.
     *
     * @return True if there are Android payment apps on device.
     */
    public static boolean hasAndroidPaymentApps() {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_PAYMENT_APPS)) return false;

        PackageManagerDelegate packageManagerDelegate = new PackageManagerDelegate();
        // Note that all Android payment apps must support org.chromium.intent.action.PAY action
        // without additional data to be detected.
        Intent payIntent = new Intent(AndroidPaymentApp.ACTION_PAY);
        return !packageManagerDelegate.getActivitiesThatCanRespondToIntent(payIntent).isEmpty();
    }

    /**
     * Gets Android payments apps' information on device.
     *
     * @return Map of Android payment apps' package names to their information. Each entry of the
     *         map represents an app and the value stores its name and icon.
     */
    public static Map<String, Pair<String, Drawable>> getAndroidPaymentAppsInfo() {
        Map<String, Pair<String, Drawable>> paymentAppsInfo = new HashMap<>();

        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_PAYMENT_APPS))
            return paymentAppsInfo;

        PackageManagerDelegate packageManagerDelegate = new PackageManagerDelegate();
        Intent payIntent = new Intent(AndroidPaymentApp.ACTION_PAY);
        List<ResolveInfo> matches =
                packageManagerDelegate.getActivitiesThatCanRespondToIntent(payIntent);
        if (matches.isEmpty()) return paymentAppsInfo;

        for (ResolveInfo match : matches) {
            CharSequence label = packageManagerDelegate.getAppLabel(match);
            if (TextUtils.isEmpty(label)) continue;
            Pair<String, Drawable> appInfo =
                    new Pair<>(label.toString(), packageManagerDelegate.getAppIcon(match));
            paymentAppsInfo.put(match.activityInfo.packageName, appInfo);
        }

        return paymentAppsInfo;
    }
}
