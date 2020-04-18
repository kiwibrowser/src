// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.searchwidget;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.chrome.browser.widget.FadingBackgroundView;

/**
 * The FadingBackgroundView isn't used here, but is still animated by the LocationBarLayout.
 * This interferes with the animation we need to show the widget moving to the right place.
 */
public class SearchActivityFadingBackgroundView extends FadingBackgroundView {
    public SearchActivityFadingBackgroundView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void showFadingOverlay() {}

    @Override
    public void hideFadingOverlay(boolean fadeOut) {}
}