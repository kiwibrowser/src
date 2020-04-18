// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import org.chromium.base.annotations.CalledByNative;

/**
 * Class used to propagate the final status of CCT offline pages requests.
 */
public class CCTRequestStatus {
    private CCTRequestStatus(int savePageResult, String savedPageId) {
        mSavePageResult = savePageResult;
        mSavedPageId = savedPageId;
    }

    private int mSavePageResult;
    private String mSavedPageId;

    /**
     * @return An org.chromium.components.offlinepages.BackgroundSavePageResult for this request.
     */
    public int getSavePageResult() {
        return mSavePageResult;
    }

    /**
     * @return The ID string extracted from the ClientId of the page.
     */
    public String getSavedPageId() {
        return mSavedPageId;
    }

    /**
     * Creates a request status object.
     * @param result an offlinepages.BackgroundSavePageResult
     * @param savedPageId the ID of the page that was completed.
     */
    @CalledByNative
    public static CCTRequestStatus create(int result, String savedPageId) {
        return new CCTRequestStatus(result, savedPageId);
    }
}
