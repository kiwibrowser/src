// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.content.DialogInterface;
import android.support.test.filters.SmallTest;
import android.support.v7.app.AlertDialog;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Tests org.chromium.chrome.browser.webapps.AddToHomescreenDialog by verifying
 * that the calling the show() method actually shows the dialog and checks that
 * some expected elements inside the dialog are present.
 *
 * This is mostly intended as a smoke test.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        // Preconnect causes issues with the single-threaded Java test server.
        "--disable-features=NetworkPrediction"})
public class AddToHomescreenDialogTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static class MockAddToHomescreenManager extends AddToHomescreenManager {
        public MockAddToHomescreenManager(Activity activity, Tab tab) {
            super(activity, tab);
        }

        @Override
        public void addToHomescreen(String userRequestedTitle) {}
    }

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @SmallTest
    @Feature("{Webapp}")
    @RetryOnFailure
    public void testSmoke() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                AddToHomescreenDialog dialog =
                        new AddToHomescreenDialog(mActivityTestRule.getActivity(),
                                new MockAddToHomescreenManager(mActivityTestRule.getActivity(),
                                        mActivityTestRule.getActivity().getActivityTab()));
                dialog.show();

                AlertDialog alertDialog = dialog.getAlertDialogForTesting();
                Assert.assertNotNull(alertDialog);

                Assert.assertTrue(alertDialog.isShowing());

                Assert.assertNotNull(alertDialog.findViewById(R.id.spinny));
                Assert.assertNotNull(alertDialog.findViewById(R.id.icon));
                Assert.assertNotNull(alertDialog.findViewById(R.id.text));
                Assert.assertNotNull(alertDialog.getButton(DialogInterface.BUTTON_POSITIVE));
                Assert.assertNotNull(alertDialog.getButton(DialogInterface.BUTTON_NEGATIVE));
            }
        });
    }
}
