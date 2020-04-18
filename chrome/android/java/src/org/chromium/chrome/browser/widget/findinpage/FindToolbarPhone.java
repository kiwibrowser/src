// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.findinpage;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.FeatureUtilities;

/**
 * A phone specific version of the {@link FindToolbar}.
 */
public class FindToolbarPhone extends FindToolbar {
    /**
     * Creates an instance of a {@link FindToolbarPhone}.
     * @param context The Context to create the {@link FindToolbarPhone} under.
     * @param attrs The AttributeSet used to create the {@link FindToolbarPhone}.
     */
    public FindToolbarPhone(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void handleActivate() {
        assert isWebContentAvailable();
        setVisibility(View.VISIBLE);
        super.handleActivate();
    }

    @Override
    protected void handleDeactivation(boolean clearSelection) {
        setVisibility(View.GONE);
        super.handleDeactivation(clearSelection);
    }

    @Override
    protected void updateVisualsForTabModel(boolean isIncognito) {
        int queryTextColorId;
        if (isIncognito) {
            setBackgroundColor(ColorUtils.getDefaultThemeColor(
                    getResources(), FeatureUtilities.isChromeModernDesignEnabled(), true));
            ColorStateList white = ApiCompatibilityUtils.getColorStateList(getResources(),
                    R.color.light_mode_tint);
            mFindNextButton.setTint(white);
            mFindPrevButton.setTint(white);
            mCloseFindButton.setTint(white);
            queryTextColorId = R.color.find_in_page_query_white_color;
        } else {
            setBackgroundColor(Color.WHITE);
            ColorStateList dark = ApiCompatibilityUtils.getColorStateList(getResources(),
                    R.color.dark_mode_tint);
            mFindNextButton.setTint(dark);
            mFindPrevButton.setTint(dark);
            mCloseFindButton.setTint(dark);
            queryTextColorId = R.color.black_alpha_87;
        }
        mFindQuery.setTextColor(
                ApiCompatibilityUtils.getColor(getContext().getResources(), queryTextColorId));
    }

    @Override
    protected int getStatusColor(boolean failed, boolean incognito) {
        if (!failed && incognito) {
            return ApiCompatibilityUtils.getColor(
                    getContext().getResources(), R.color.white_alpha_50);
        }

        return super.getStatusColor(failed, incognito);
    }
}
