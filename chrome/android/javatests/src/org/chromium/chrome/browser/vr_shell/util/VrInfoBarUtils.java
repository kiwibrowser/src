// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_SHORT_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.infobar.InfoBar;
import org.chromium.chrome.browser.vr_shell.TestFramework;
import org.chromium.chrome.test.util.InfoBarUtil;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.List;
import java.util.concurrent.Callable;

/**
 * Class containing utility functions for interacting with InfoBars at
 * a high level.
 */
public class VrInfoBarUtils {
    public enum Button { PRIMARY, SECONDARY };

    /**
     * Determines whether InfoBars are present in the current activity.
     * @param framework The TestFramework to get the current activity from
     * @return True if there are any InfoBars present, false otherwise
     */
    @SuppressWarnings("unchecked")
    public static boolean isInfoBarPresent(TestFramework framework) {
        List<InfoBar> infoBars = framework.getRule().getInfoBars();
        return infoBars != null && !infoBars.isEmpty();
    }

    /**
     * Clicks on either the primary or secondary button of the first InfoBar
     * in the activity.
     * @param button Which button to click
     * @param framework The TestFramework to get the current activity from
     */
    @SuppressWarnings("unchecked")
    public static void clickInfoBarButton(final Button button, TestFramework framework) {
        if (!isInfoBarPresent(framework)) return;
        final List<InfoBar> infoBars = framework.getRule().getInfoBars();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                switch (button) {
                    case PRIMARY:
                        InfoBarUtil.clickPrimaryButton(infoBars.get(0));
                        break;
                    default:
                        InfoBarUtil.clickSecondaryButton(infoBars.get(0));
                }
            }
        });
        InfoBarUtil.waitUntilNoInfoBarsExist(framework.getRule().getInfoBars());
    }

    /**
     * Clicks on the close button of the first InfoBar in the activity.
     * @param framework The TestFramework to get the current activity from
     */
    @SuppressWarnings("unchecked")
    public static void clickInfobarCloseButton(TestFramework framework) {
        if (!isInfoBarPresent(framework)) return;
        final List<InfoBar> infoBars = framework.getRule().getInfoBars();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                InfoBarUtil.clickCloseButton(infoBars.get(0));
            }
        });
        InfoBarUtil.waitUntilNoInfoBarsExist(framework.getRule().getInfoBars());
    }

    /**
     * Determines is there is any InfoBar present in the given View hierarchy.
     * @param framework The TestFramework that will be used to get the current activity/view from
     * @param present Whether an InfoBar should be present.
     */
    public static void expectInfoBarPresent(final TestFramework framework, boolean present) {
        CriteriaHelper.pollUiThread(Criteria.equals(present, new Callable<Boolean>() {
            @Override
            public Boolean call() {
                return isInfoBarPresent(framework);
            }
        }), POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_SHORT_MS);
    }
}
