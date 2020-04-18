// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import org.chromium.base.annotations.JNINamespace;

/**
 * A class used to record journey metrics for the Payment Request feature.
 */
@JNINamespace("payments")
public class JourneyLogger {
    /**
     * Pointer to the native implementation.
     */
    private long mJourneyLoggerAndroid;

    private boolean mWasPaymentRequestTriggered;
    private boolean mHasRecorded;

    public JourneyLogger(boolean isIncognito, String url) {
        // Note that this pointer could leak the native object. The called must call destroy() to
        // ensure that the native object is destroyed.
        mJourneyLoggerAndroid = nativeInitJourneyLoggerAndroid(isIncognito, url);
    }

    /** Will destroy the native object. This class shouldn't be used afterwards. */
    public void destroy() {
        if (mJourneyLoggerAndroid != 0) {
            nativeDestroy(mJourneyLoggerAndroid);
            mJourneyLoggerAndroid = 0;
        }
    }

    /**
     * Sets the number of suggestions shown for the specified section.
     *
     * @param section               The section for which to log.
     * @param number                The number of suggestions.
     * @param hasCompleteSuggestion Whether the section has at least one
     *                              complete suggestion.
     */
    public void setNumberOfSuggestionsShown(
            int section, int number, boolean hasCompleteSuggestion) {
        assert section < Section.MAX;
        nativeSetNumberOfSuggestionsShown(
                mJourneyLoggerAndroid, section, number, hasCompleteSuggestion);
    }

    /**
     * Increments the number of selection changes for the specified section.
     *
     * @param section The section for which to log.
     */
    public void incrementSelectionChanges(int section) {
        assert section < Section.MAX;
        nativeIncrementSelectionChanges(mJourneyLoggerAndroid, section);
    }

    /**
     * Increments the number of selection edits for the specified section.
     *
     * @param section The section for which to log.
     */
    public void incrementSelectionEdits(int section) {
        assert section < Section.MAX;
        nativeIncrementSelectionEdits(mJourneyLoggerAndroid, section);
    }

    /**
     * Increments the number of selection adds for the specified section.
     *
     * @param section The section for which to log.
     */
    public void incrementSelectionAdds(int section) {
        assert section < Section.MAX;
        nativeIncrementSelectionAdds(mJourneyLoggerAndroid, section);
    }

    /**
     * Records the fact that the merchant called CanMakePayment and records it's return value.
     *
     * @param value The return value of the CanMakePayment call.
     */
    public void setCanMakePaymentValue(boolean value) {
        nativeSetCanMakePaymentValue(mJourneyLoggerAndroid, value);
    }

    /**
     * Records that an event occurred.
     *
     * @param event The event that occurred.
     */
    public void setEventOccurred(int event) {
        assert event >= 0;
        assert event < Event.ENUM_MAX;

        if (event == Event.SHOWN || event == Event.SKIPPED_SHOW) mWasPaymentRequestTriggered = true;

        nativeSetEventOccurred(mJourneyLoggerAndroid, event);
    }

    /*
     * Records what user information were requested by the merchant to complete the Payment Request.
     *
     * @param requestShipping Whether the merchant requested a shipping address.
     * @param requestEmail    Whether the merchant requested an email address.
     * @param requestPhone    Whether the merchant requested a phone number.
     * @param requestName     Whether the merchant requestes a name.
     */
    public void setRequestedInformation(boolean requestShipping, boolean requestEmail,
            boolean requestPhone, boolean requestName) {
        nativeSetRequestedInformation(
                mJourneyLoggerAndroid, requestShipping, requestEmail, requestPhone, requestName);
    }

    /*
     * Records what types of payment methods were requested by the merchant in the Payment Request.
     *
     * @param requestedBasicCard    Whether the merchant requested basic-card.
     * @param requestedMethodGoogle Whether the merchant requested a Google payment method.
     * @param requestedMethodOther  Whether the merchant requested a non basic-card, non-Google
     *                              payment method.
     */
    public void setRequestedPaymentMethodTypes(boolean requestedBasicCard,
            boolean requestedMethodGoogle, boolean requestedMethodOther) {
        nativeSetRequestedPaymentMethodTypes(mJourneyLoggerAndroid, requestedBasicCard,
                requestedMethodGoogle, requestedMethodOther);
    }

    /**
     * Records that the Payment Request was completed sucessfully. Also starts the logging of
     * all the journey logger metrics.
     */
    public void setCompleted() {
        assert !mHasRecorded;
        assert mWasPaymentRequestTriggered;

        if (!mHasRecorded && mWasPaymentRequestTriggered) {
            mHasRecorded = true;
            nativeSetCompleted(mJourneyLoggerAndroid);
        }
    }

    /**
     * Records that the Payment Request was aborted and for what reason. Also starts the logging of
     * all the journey logger metrics.
     *
     * @param reason An int indicating why the payment request was aborted.
     */
    public void setAborted(int reason) {
        assert reason < AbortReason.MAX;

        // The abort reasons on Android cascade into each other, so only the first one should be
        // recorded.
        if (!mHasRecorded && mWasPaymentRequestTriggered) {
            mHasRecorded = true;
            nativeSetAborted(mJourneyLoggerAndroid, reason);
        }
    }

    /**
     * Records that the Payment Request was not shown to the user and for what reason.
     *
     * @param reason An int indicating why the payment request was not shown.
     */
    public void setNotShown(int reason) {
        assert reason < NotShownReason.MAX;
        assert !mWasPaymentRequestTriggered;
        assert !mHasRecorded;

        if (!mHasRecorded) {
            mHasRecorded = true;
            nativeSetNotShown(mJourneyLoggerAndroid, reason);
        }
    }

    private native long nativeInitJourneyLoggerAndroid(boolean isIncognito, String url);
    private native void nativeDestroy(long nativeJourneyLoggerAndroid);
    private native void nativeSetNumberOfSuggestionsShown(long nativeJourneyLoggerAndroid,
            int section, int number, boolean hasCompleteSuggestion);
    private native void nativeIncrementSelectionChanges(
            long nativeJourneyLoggerAndroid, int section);
    private native void nativeIncrementSelectionEdits(long nativeJourneyLoggerAndroid, int section);
    private native void nativeIncrementSelectionAdds(long nativeJourneyLoggerAndroid, int section);
    private native void nativeSetCanMakePaymentValue(
            long nativeJourneyLoggerAndroid, boolean value);
    private native void nativeSetEventOccurred(long nativeJourneyLoggerAndroid, int event);
    private native void nativeSetRequestedInformation(long nativeJourneyLoggerAndroid,
            boolean requestShipping, boolean requestEmail, boolean requestPhone,
            boolean requestName);
    private native void nativeSetRequestedPaymentMethodTypes(long nativeJourneyLoggerAndroid,
            boolean requestedBasicCard, boolean requestedMethodGoogle,
            boolean requestedMethodOther);
    private native void nativeSetCompleted(long nativeJourneyLoggerAndroid);
    private native void nativeSetAborted(long nativeJourneyLoggerAndroid, int reason);
    private native void nativeSetNotShown(long nativeJourneyLoggerAndroid, int reason);
}
