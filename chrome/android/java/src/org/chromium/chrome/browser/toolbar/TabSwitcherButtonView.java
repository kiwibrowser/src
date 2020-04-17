// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.widget.ImageView;

import org.chromium.chrome.R;

/**
 * The Button used for switching tabs. Currently this class is only being used for the bottom
 * toolbar tab switcher button.
 */
public class TabSwitcherButtonView extends ImageView {
    /**
     * A drawable for the tab switcher icon.
     */
    private TabSwitcherDrawable mTabSwitcherButtonButtonDrawable;

    public TabSwitcherButtonView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        Resources resources = getResources();
        mTabSwitcherButtonButtonDrawable =
                TabSwitcherDrawable.createTabSwitcherDrawable(resources, false);
        setImageDrawable(mTabSwitcherButtonButtonDrawable);
    }

    /**
     * @param numberOfTabs The number of open tabs.
     */
    public void updateTabCountVisuals(int numberOfTabs) {
        setEnabled(numberOfTabs >= 1);
        setContentDescription(getResources().getQuantityString(
                R.plurals.accessibility_toolbar_btn_tabswitcher_toggle, numberOfTabs,
                numberOfTabs));
        mTabSwitcherButtonButtonDrawable.updateForTabCount(numberOfTabs, false);
    }
}
