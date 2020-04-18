// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.mojo.system.MojoException;
import org.chromium.payments.mojom.CanMakePaymentQueryResult;
import org.chromium.payments.mojom.PaymentDetails;
import org.chromium.payments.mojom.PaymentErrorReason;
import org.chromium.payments.mojom.PaymentMethodData;
import org.chromium.payments.mojom.PaymentOptions;
import org.chromium.payments.mojom.PaymentRequest;
import org.chromium.payments.mojom.PaymentRequestClient;
import org.chromium.services.service_manager.InterfaceFactory;

/**
 * Creates instances of PaymentRequest.
 */
public class PaymentRequestFactory implements InterfaceFactory<PaymentRequest> {
    private final RenderFrameHost mRenderFrameHost;

    /**
     * An implementation of PaymentRequest that immediately rejects all connections.
     * Necessary because Mojo does not handle null returned from createImpl().
     */
    private static final class InvalidPaymentRequest implements PaymentRequest {
        private PaymentRequestClient mClient;

        @Override
        public void init(PaymentRequestClient client, PaymentMethodData[] methodData,
                PaymentDetails details, PaymentOptions options) {
            mClient = client;
        }

        @Override
        public void show(boolean isUserGesture) {
            if (mClient != null) {
                mClient.onError(PaymentErrorReason.USER_CANCEL);
                mClient.close();
            }
        }

        @Override
        public void updateWith(PaymentDetails details) {}

        @Override
        public void noUpdatedPaymentDetails() {}

        @Override
        public void abort() {}

        @Override
        public void complete(int result) {}

        @Override
        public void canMakePayment() {
            if (mClient != null) {
                mClient.onCanMakePayment(CanMakePaymentQueryResult.CANNOT_MAKE_PAYMENT);
            }
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}
    }

    /**
     * Builds a factory for PaymentRequest.
     *
     * @param webContents The web contents that may invoke the PaymentRequest API.
     */
    public PaymentRequestFactory(RenderFrameHost renderFrameHost) {
        mRenderFrameHost = renderFrameHost;
    }

    @Override
    public PaymentRequest createImpl() {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.WEB_PAYMENTS)) {
            return new InvalidPaymentRequest();
        }

        if (mRenderFrameHost == null) return new InvalidPaymentRequest();

        return new PaymentRequestImpl(mRenderFrameHost);
    }
}
