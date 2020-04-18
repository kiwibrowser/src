// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media;

import android.app.Dialog;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentManager;
import android.view.View;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.ArrayList;
import java.util.concurrent.Callable;

/**
 * Test utils shared by MediaRouter and MediaRemote.
 */
public class RouterTestUtils {
    private static final String TAG = "RouterTestUtils";

    public static View waitForRouteButton(
            final ChromeActivity activity, final String chromecastName,
            int maxTimeoutMs, int intervalMs) {
        return waitForView(new Callable<View>() {
                @Override
                public View call() {
                    Dialog mediaRouteListDialog = getDialog(activity);
                    if (mediaRouteListDialog == null) {
                        Log.w(TAG, "Cannot find device selection dialog");
                        return null;
                    }
                    View mediaRouteList =
                            mediaRouteListDialog.findViewById(R.id.mr_chooser_list);
                    if (mediaRouteList == null) {
                        Log.w(TAG, "Cannot find device list");
                        return null;
                    }
                    ArrayList<View> routesWanted = new ArrayList<View>();
                    mediaRouteList.findViewsWithText(routesWanted, chromecastName,
                                                     View.FIND_VIEWS_WITH_TEXT);
                    if (routesWanted.size() == 0) {
                        Log.w(TAG, "Cannot find wanted device");
                        return null;
                    }
                    Log.i(TAG, "Found wanted device");
                    return routesWanted.get(0);
                }
            }, maxTimeoutMs, intervalMs);
    }

    public static Dialog waitForDialog(
            final ChromeActivity activity,
            int maxTimeoutMs, int intervalMs) {
        try {
            CriteriaHelper.pollUiThread(new Criteria() {
                    @Override
                    public boolean isSatisfied() {
                        try {
                            if (getDialog(activity) != null) {
                                Log.i(TAG, "Found device selection dialog");
                                return true;
                            } else {
                                Log.w(TAG, "Cannot find device selection dialog");
                                return false;
                            }
                        } catch (Exception e) {
                            return false;
                        }
                    }
                }, maxTimeoutMs, intervalMs);
            return getDialog(activity);
        } catch (Exception e) {
            return null;
        }
    }

    public static Dialog getDialog(ChromeActivity activity) {
        FragmentManager fm = activity.getSupportFragmentManager();
        if (fm == null) return null;
        return ((DialogFragment) fm.findFragmentByTag(
            "android.support.v7.mediarouter:MediaRouteChooserDialogFragment")).getDialog();
    }

    public static View waitForView(
            final Callable<View> getViewCallable, int maxTimeoutMs, int intervalMs) {
        try {
            CriteriaHelper.pollUiThread(new Criteria() {
                    @Override
                    public boolean isSatisfied() {
                        try {
                            return getViewCallable.call() != null;
                        } catch (Exception e) {
                            return false;
                        }
                    }
                }, maxTimeoutMs, intervalMs);
            return getViewCallable.call();
        } catch (Exception e) {
            return null;
        }
    }
}
