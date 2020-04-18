// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.graphics.drawable.Drawable;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.widget.prefeditor.EditableOption;
import org.chromium.payments.mojom.PaymentDetailsModifier;
import org.chromium.payments.mojom.PaymentItem;
import org.chromium.payments.mojom.PaymentMethodData;

import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * The base class for a single payment instrument, e.g., a credit card.
 */
public abstract class PaymentInstrument extends EditableOption {
    /**
     * The interface for the requester of instrument details.
     */
    public interface InstrumentDetailsCallback {
        /**
         * Called by the payment instrument to let Chrome know that the payment app's UI is
         * now hidden, but the payment instrument has not been returned yet. This is a good
         * time to show a "loading" progress indicator UI.
         */
        void onInstrumentDetailsLoadingWithoutUI();

        /**
         * Called after retrieving instrument details.
         *
         * @param methodName         Method name. For example, "visa".
         * @param stringifiedDetails JSON-serialized object. For example, {"card": "123"}.
         */
        void onInstrumentDetailsReady(String methodName, String stringifiedDetails);

        /**
         * Called if unable to retrieve instrument details.
         */
        void onInstrumentDetailsError();
    }

    /** The interface for the requester to abort payment. */
    public interface AbortCallback {
        /**
         * Called after aborting payment is finished.
         *
         * @param abortSucceeded Indicates whether abort is succeed.
         */
        void onInstrumentAbortResult(boolean abortSucceeded);
    }

    protected PaymentInstrument(String id, String label, String sublabel, Drawable icon) {
        super(id, label, sublabel, icon);
    }

    protected PaymentInstrument(
            String id, String label, String sublabel, String tertiarylabel, Drawable icon) {
        super(id, label, sublabel, tertiarylabel, icon);
    }

    /**
     * Sets the modified total for this payment instrument.
     *
     * @param modifiedTotal The new modified total to use.
     */
    public void setModifiedTotal(@Nullable String modifiedTotal) {
        updatePromoMessage(modifiedTotal);
    }

    /**
     * Returns a set of payment method names for this instrument, e.g., "visa" or
     * "mastercard" in basic card payments:
     * https://w3c.github.io/webpayments-methods-card/#method-id
     *
     * @return The method names for this instrument.
     */
    public abstract Set<String> getInstrumentMethodNames();

    /**
     * @return Whether this is an autofill instrument. All autofill instruments are sorted below all
     *         non-autofill instruments.
     */
    public boolean isAutofillInstrument() {
        return false;
    }

    /** @return Whether this is a server autofill instrument. */
    public boolean isServerAutofillInstrument() {
        return false;
    }

    /**
     * @return Whether this is a replacement for all server autofill instruments. If at least one of
     *         the displayed instruments returns true here, then all instruments that return true
     *         in isServerAutofillInstrument() should be hidden.
     */
    public boolean isServerAutofillInstrumentReplacement() {
        return false;
    }

    /**
     * @return Whether the instrument is exactly matching all filters provided by the merchant. For
     *         example, this can return false for unknown card types, if the merchant requested only
     *         debit cards.
     */
    public boolean isExactlyMatchingMerchantRequest() {
        return true;
    }

    /**
     * @return Whether the instrument supports the payment method with the method data. For example,
     *         supported card types and networks in the data should be verified for 'basic-card'
     *         payment method.
     */
    public boolean isValidForPaymentMethodData(String method, PaymentMethodData data) {
        return getInstrumentMethodNames().contains(method);
    }

    /** @return The country code (or null if none) associated with this payment instrument. */
    @Nullable
    public String getCountryCode() {
        return null;
    }

    /**
     * @return Whether presence of this payment instrument should cause the
     *         PaymentRequest.canMakePayment() to return true.
     */
    public boolean canMakePayment() {
        return true;
    }

    /** @return Whether this payment instrument can be pre-selected for immediate payment. */
    public boolean canPreselect() {
        return true;
    }

    /** @return Whether skip-UI flow with this instrument requires a user gesture. */
    public boolean isUserGestureRequiredToSkipUi() {
        return true;
    }

    /**
     * Invoke the payment app to retrieve the instrument details.
     *
     * The callback will be invoked with the resulting payment details or error.
     *
     * @param id               The unique identifier of the PaymentRequest.
     * @param merchantName     The name of the merchant.
     * @param origin           The origin of this merchant.
     * @param iframeOrigin     The origin of the iframe that invoked PaymentRequest.
     * @param certificateChain The site certificate chain of the merchant. Can be null for localhost
     *                         or local file, which are secure contexts without SSL.
     * @param methodDataMap    The payment-method specific data for all applicable payment methods,
     *                         e.g., whether the app should be invoked in test or production, a
     *                         merchant identifier, or a public key.
     * @param total            The total amount.
     * @param displayItems     The shopping cart items.
     * @param modifiers        The relevant payment details modifiers.
     * @param callback         The object that will receive the instrument details.
     */
    public abstract void invokePaymentApp(String id, String merchantName, String origin,
            String iframeOrigin, @Nullable byte[][] certificateChain,
            Map<String, PaymentMethodData> methodDataMap, PaymentItem total,
            List<PaymentItem> displayItems, Map<String, PaymentDetailsModifier> modifiers,
            InstrumentDetailsCallback callback);

    /**
     * Abort invocation of the payment app.
     *
     * @param callback The callback to return abort result.
     */
    public void abortPaymentApp(AbortCallback callback) {
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                callback.onInstrumentAbortResult(false);
            }
        });
    }

    /**
     * Cleans up any resources held by the payment instrument. For example, closes server
     * connections.
     */
    public abstract void dismissInstrument();
}
