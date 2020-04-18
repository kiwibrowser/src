// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.support.v7.app.ActionBar;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;

/**
 * An {@link ActionModeController.ActionBarDelegate} that shifts a view as the action bar appears.
 */
public class ViewShiftingActionBarDelegate implements ActionModeController.ActionBarDelegate {
    /** The view that will be shifted as the action bar appears. */
    private final View mShiftingView;

    /** A handle to a {@link ChromeActivity}. */
    private final ChromeActivity mActivity;

    /**
     * @param activity A handle to the {@link ChromeActivity}.
     * @param shiftingView The view that will shift when the action bar appears.
     */
    public ViewShiftingActionBarDelegate(ChromeActivity activity, View shiftingView) {
        mActivity = activity;
        mShiftingView = shiftingView;
    }

    @Override
    public void setControlTopMargin(int margin) {
        ViewGroup.MarginLayoutParams lp =
                (ViewGroup.MarginLayoutParams) mShiftingView.getLayoutParams();
        lp.topMargin = margin;
        mShiftingView.setLayoutParams(lp);
    }

    @Override
    public int getControlTopMargin() {
        ViewGroup.MarginLayoutParams lp =
                (ViewGroup.MarginLayoutParams) mShiftingView.getLayoutParams();
        return lp.topMargin;
    }

    @Override
    public ActionBar getSupportActionBar() {
        return mActivity.getSupportActionBar();
    }

    @Override
    public void setActionBarBackgroundVisibility(boolean visible) {
        int visibility = visible ? View.VISIBLE : View.GONE;
        mActivity.findViewById(R.id.action_bar_black_background).setVisibility(visibility);
        // TODO(tedchoc): Add support for changing the color based on the brand color.
    }
}