// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.snackbar;

import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * Tests for {@link SnackbarManager}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class SnackbarTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private SnackbarManager mManager;
    private SnackbarController mDefaultController = new SnackbarController() {
        @Override
        public void onDismissNoAction(Object actionData) {
        }

        @Override
        public void onAction(Object actionData) {
        }
    };

    private SnackbarController mDismissController = new SnackbarController() {
        @Override
        public void onDismissNoAction(Object actionData) {
            mDismissed = true;
        }

        @Override
        public void onAction(Object actionData) { }
    };

    private boolean mDismissed;

    @Before
    public void setUp() throws InterruptedException {
        SnackbarManager.setDurationForTesting(1000);
        mActivityTestRule.startMainActivityOnBlankPage();
        mManager = mActivityTestRule.getActivity().getSnackbarManager();
    }

    @Test
    @MediumTest
    @RetryOnFailure
    public void testStackQueueOrder() {
        final Snackbar stackbar = Snackbar.make("stack", mDefaultController,
                Snackbar.TYPE_ACTION, Snackbar.UMA_TEST_SNACKBAR);
        final Snackbar queuebar = Snackbar.make("queue", mDefaultController,
                Snackbar.TYPE_NOTIFICATION, Snackbar.UMA_TEST_SNACKBAR);
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.showSnackbar(stackbar);
            }
        });
        CriteriaHelper.pollUiThread(new Criteria("First snackbar not shown") {
            @Override
            public boolean isSatisfied() {
                return mManager.isShowing() && mManager.getCurrentSnackbarForTesting() == stackbar;
            }
        });
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.showSnackbar(queuebar);
                Assert.assertTrue("Snackbar not showing", mManager.isShowing());
                Assert.assertEquals(
                        "Snackbars on stack should not be cancled by snackbars on queue", stackbar,
                        mManager.getCurrentSnackbarForTesting());
            }
        });
        CriteriaHelper.pollUiThread(new Criteria("Snackbar on queue not shown") {
            @Override
            public boolean isSatisfied() {
                return mManager.isShowing() && mManager.getCurrentSnackbarForTesting() == queuebar;
            }
        });
        CriteriaHelper.pollUiThread(new Criteria("Snackbar did not time out") {
            @Override
            public boolean isSatisfied() {
                return !mManager.isShowing();
            }
        });
    }

    @Test
    @SmallTest
    @RetryOnFailure
    public void testQueueStackOrder() {
        final Snackbar stackbar = Snackbar.make("stack", mDefaultController,
                Snackbar.TYPE_ACTION, Snackbar.UMA_TEST_SNACKBAR);
        final Snackbar queuebar = Snackbar.make("queue", mDefaultController,
                Snackbar.TYPE_NOTIFICATION, Snackbar.UMA_TEST_SNACKBAR);
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.showSnackbar(queuebar);
            }
        });
        CriteriaHelper.pollUiThread(new Criteria("First snackbar not shown") {
            @Override
            public boolean isSatisfied() {
                return mManager.isShowing() && mManager.getCurrentSnackbarForTesting() == queuebar;
            }
        });
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.showSnackbar(stackbar);
            }
        });
        CriteriaHelper.pollUiThread(
                new Criteria("Snackbar on queue was not cleared by snackbar stack.") {
                    @Override
                    public boolean isSatisfied() {
                        return mManager.isShowing()
                                && mManager.getCurrentSnackbarForTesting() == stackbar;
                    }
                });
        CriteriaHelper.pollUiThread(new Criteria("Snackbar did not time out") {
            @Override
            public boolean isSatisfied() {
                return !mManager.isShowing();
            }
        });
    }

    @Test
    @SmallTest
    public void testDismissSnackbar() {
        final Snackbar snackbar = Snackbar.make("stack", mDismissController,
                Snackbar.TYPE_ACTION, Snackbar.UMA_TEST_SNACKBAR);
        mDismissed = false;
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.showSnackbar(snackbar);
            }
        });
        CriteriaHelper.pollUiThread(
                new Criteria("Snackbar on queue was not cleared by snackbar stack.") {
                    @Override
                    public boolean isSatisfied() {
                        return mManager.isShowing()
                                && mManager.getCurrentSnackbarForTesting() == snackbar;
                    }
                });
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.dismissSnackbars(mDismissController);
            }
        });
        CriteriaHelper.pollUiThread(new Criteria("Snackbar did not time out") {
            @Override
            public boolean isSatisfied() {
                return !mManager.isShowing() && mDismissed;
            }
        });
    }
}
