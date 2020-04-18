// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import java.util.concurrent.Callable;

/**
 * Provides a means for validating whether some condition/criteria has been met.
 * <p>
 * See {@link CriteriaHelper} for usage guidelines.
 */
public abstract class Criteria {

    private String mFailureReason;

    /**
     * Constructs a Criteria with a default failure message.
     */
    public Criteria() {
        this("Criteria not met in allotted time.");
    }

    /**
     * Constructs a Criteria with an explicit message to be shown on failure.
     * @param failureReason The failure reason to be shown.
     */
    public Criteria(String failureReason) {
        if (failureReason != null) mFailureReason = failureReason;
    }

    /**
     * @return Whether the criteria this is testing has been satisfied.
     */
    public abstract boolean isSatisfied();

    /**
     * @return The failure message that will be logged if the criteria is not satisfied within
     *         the specified time range.
     */
    public String getFailureReason() {
        return mFailureReason;
    }

    /**
     * Updates the message to displayed if this criteria does not succeed in the allotted time.  For
     * correctness, you should be updating this in {@link #isSatisfied()} to ensure the error state
     * is the same that you last checked.
     *
     * @param reason The failure reason to be shown.
     */
    public void updateFailureReason(String reason) {
        mFailureReason = reason;
    }

    /**
     * Constructs a Criteria that will determine the equality of two items.
     *
     * <p>
     * <pre>
     * Sample Usage:
     * <code>
     * public void waitForTabTitle(final Tab tab, String title) {
     *     CriteriaHelper.pollUiThread(Criteria.equals(title, new Callable<String>() {
     *         {@literal @}Override
     *         public String call() {
     *             return tab.getTitle();
     *         }
     *     }));
     * }
     * </code>
     * </pre>
     *
     * @param <T> The type of value whose equality will be tested.
     * @param expectedValue The value that is expected to determine the success of the criteria.
     * @param actualValueCallable A {@link Callable} that provides a way of getting the current
     *                            actual value.
     * @return A Criteria that will check the equality of the passed in data.
     */
    public static <T> Criteria equals(T expectedValue, Callable<T> actualValueCallable) {
        return new EqualityCriteria<T>(expectedValue, actualValueCallable);
    }

}
