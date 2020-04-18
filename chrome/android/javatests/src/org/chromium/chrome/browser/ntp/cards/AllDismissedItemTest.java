// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.test.filters.MediumTest;
import android.widget.FrameLayout;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ntp.cards.AllDismissedItem.ViewHolder;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.RenderTestRule;
import org.chromium.chrome.test.util.browser.ChromeModernDesign;

import java.io.IOException;


/**
 * Tests for the appearance and behaviour of AllDismissedItem.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AllDismissedItemTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Rule
    public RenderTestRule mRenderTestRule = new RenderTestRule();

    @Rule
    public TestRule mChromeModernDesignStateRule = new ChromeModernDesign.Processor();

    private FrameLayout mContentView;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mContentView = new FrameLayout(mActivityTestRule.getActivity());
            mActivityTestRule.getActivity().setContentView(mContentView);
        });
    }

    @Test
    @MediumTest
    @Feature({"Cards", "RenderTest"})
    @ChromeModernDesign.Disable
    public void testNewTabPageAppearance_Normal() throws IOException {
        SectionList sectionList = null;  // The SectionList is only used if the item is clicked on.
        ViewHolder viewHolder = new ViewHolder(mContentView, sectionList);

        renderAtHour(viewHolder, 9, "morning");
        renderAtHour(viewHolder, 14, "afternoon");
        renderAtHour(viewHolder, 20, "evening");
    }

    @Test
    @MediumTest
    @Feature({"Cards", "RenderTest"})
    @ChromeModernDesign.Enable
    public void testNewTabPageAppearance_Modern() throws IOException {
        SectionList sectionList = null; // The SectionList is only used if the item is clicked on.
        ViewHolder viewHolder = new ViewHolder(mContentView, sectionList);

        renderAtHour(viewHolder, 9, "modern-morning");
        renderAtHour(viewHolder, 14, "modern-afternoon");
        renderAtHour(viewHolder, 20, "modern-evening");
    }

    @Test
    //@MediumTest
    //@Feature({"Cards", "RenderTest"})
    // https://crbug.com/780555, re-enable with https://crbug.com/816922
    @DisabledTest
    @ChromeModernDesign.Enable
    public void testChromeHomeAppearance() throws IOException {
        renderAtHour(new ViewHolder(mContentView, null), 0, "modern");
    }

    private void renderAtHour(ViewHolder viewHolder, int hour, String renderId) throws IOException {
        // TODO(peconn): Extract common code between this and ArticleSnippetsTest for rendering
        // views in isolation.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            viewHolder.onBindViewHolder(hour, null);
            mContentView.addView(viewHolder.itemView);
        });
        mRenderTestRule.render(viewHolder.itemView, renderId);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mContentView.removeView(viewHolder.itemView);
            viewHolder.recycle();
        });
    }
}
