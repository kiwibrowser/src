// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import java.util.concurrent.Callable;

/**
 * Extension of the Criteria that handles object equality while providing a standard error message.
 *
 * @param <T> The type of value whose equality will be tested.
 */
class EqualityCriteria<T> extends Criteria {

    private final T mExpectedValue;
    private final Callable<T> mActualValueCallable;

    /**
     * Construct the EqualityCriteria with the given expected value.
     * @param expectedValue The value that is expected to determine the success of the criteria.
     */
    public EqualityCriteria(T expectedValue, Callable<T> actualValueCallable) {
        mExpectedValue = expectedValue;
        mActualValueCallable = actualValueCallable;
    }

    @Override
    public final boolean isSatisfied() {
        T actualValue = null;
        try {
            actualValue = mActualValueCallable.call();
        } catch (Exception ex) {
            updateFailureReason("Exception occurred: " + ex.getMessage());
            ex.printStackTrace();
            return false;
        }

        updateFailureReason(
                "Values did not match. Expected: " + mExpectedValue + ", actual: " + actualValue);
        if (mExpectedValue == null) {
            return actualValue == null;
        }
        return mExpectedValue.equals(actualValue);
    }
}
