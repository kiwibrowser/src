// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.os.Bundle;

/**
 * Defines the host interface for First Run Experience pages.
 */
public interface FirstRunPageDelegate {
    /**
     * Returns FRE properties bundle.
     */
    Bundle getProperties();

    /**
     * Advances the First Run Experience to the next page.
     * Successfully finishes FRE if the current page is the last page.
     */
    void advanceToNextPage();

    /**
     * Unsuccessfully aborts the First Run Experience.
     * This usually means that the application will be closed.
     */
    void abortFirstRunExperience();

    /**
     * Successfully completes the First Run Experience.
     * All results will be packaged and sent over to the main activity.
     */
    void completeFirstRunExperience();

    /**
     * Notifies that the user refused to sign in (e.g. "NO, THANKS").
     */
    void refuseSignIn();

    /**
     * Notifies that the user accepted to be signed in.
     * @param accountName An account to be signed in to.
     * @param isDefaultAccount Whether this account is the default choice for the user.
     */
    void acceptSignIn(String accountName, boolean isDefaultAccount);

    /**
     * Notifies that the user asked to show sign in Settings once the sign in
     * process is complete.
     */
    void askToOpenSignInSettings();

    /**
     * @return Whether the user has accepted Chrome Terms of Service.
     */
    boolean didAcceptTermsOfService();

    /**
     * Notifies all interested parties that the user has accepted Chrome Terms of Service.
     * Must be called only after native has been initialized.
     * @param allowCrashUpload True if the user allows to upload crash dumps and collect stats.
     */
    void acceptTermsOfService(boolean allowCrashUpload);

    /**
     * Show an informational web page. The page doesn't show navigation control.
     * @param url Resource id for the URL of the web page.
     */
    void showInfoPage(int url);
}
