// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.util.Pair;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.payments.AndroidPaymentAppFactory;
import org.chromium.chrome.browser.payments.ServiceWorkerPaymentAppBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.chrome.browser.preferences.TextMessagePreference;

import java.util.Map;

/**
 * Preference fragment to allow users to control use of the Android payment apps on device.
 */
public class AndroidPaymentAppsFragment extends PreferenceFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceUtils.addPreferencesFromResource(
                this, R.xml.autofill_and_payments_preference_fragment_screen);
        getActivity().setTitle(R.string.payment_apps_title);
    }

    @Override
    public void onResume() {
        super.onResume();
        rebuildPaymentAppsList();
    }

    private void rebuildPaymentAppsList() {
        getPreferenceScreen().removeAll();
        getPreferenceScreen().setOrderingAsAdded(true);

        ServiceWorkerPaymentAppBridge.getServiceWorkerPaymentAppsInfo(
                new ServiceWorkerPaymentAppBridge.GetServiceWorkerPaymentAppsInfoCallback() {
                    @Override
                    public void onGetServiceWorkerPaymentAppsInfo(
                            Map<String, Pair<String, Bitmap>> appsInfo) {
                        addPaymentAppsPreference(
                                AndroidPaymentAppFactory.getAndroidPaymentAppsInfo(), appsInfo);
                    }
                });
    }

    private void addPaymentAppsPreference(Map<String, Pair<String, Drawable>> androidAppsInfo,
            Map<String, Pair<String, Bitmap>> serviceWorkerAppsInfo) {
        if (androidAppsInfo.isEmpty() && serviceWorkerAppsInfo.isEmpty()) return;

        AndroidPaymentAppPreference pref = null;
        for (Map.Entry<String, Pair<String, Drawable>> app : androidAppsInfo.entrySet()) {
            pref = new AndroidPaymentAppPreference(getActivity());
            pref.setTitle(app.getValue().first);
            pref.setIcon(app.getValue().second);
            getPreferenceScreen().addPreference(pref);
        }
        for (Map.Entry<String, Pair<String, Bitmap>> app : serviceWorkerAppsInfo.entrySet()) {
            pref = new AndroidPaymentAppPreference(getActivity());
            pref.setTitle(app.getValue().first);
            pref.setSummary(app.getKey());
            pref.setIcon(app.getValue().second == null
                            ? new ColorDrawable(Color.TRANSPARENT)
                            : new BitmapDrawable(getResources(), app.getValue().second));
            getPreferenceScreen().addPreference(pref);
        }
        // Add a divider line at the bottom of the last preference to separate it from below
        // TextMessagePreference.
        if (pref != null) pref.setDrawDivider(true);

        TextMessagePreference textPreference = new TextMessagePreference(getActivity(), null);
        textPreference.setTitle(getActivity().getString(R.string.payment_apps_usage_message));
        getPreferenceScreen().addPreference(textPreference);
    }
}
