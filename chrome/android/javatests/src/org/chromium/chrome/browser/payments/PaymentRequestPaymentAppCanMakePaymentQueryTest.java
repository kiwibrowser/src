// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import static org.chromium.chrome.browser.payments.PaymentRequestTestRule.DELAYED_RESPONSE;
import static org.chromium.chrome.browser.payments.PaymentRequestTestRule.HAVE_INSTRUMENTS;
import static org.chromium.chrome.browser.payments.PaymentRequestTestRule.IMMEDIATE_RESPONSE;
import static org.chromium.chrome.browser.payments.PaymentRequestTestRule.NO_INSTRUMENTS;

import android.support.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.payments.PaymentRequestTestRule.MainActivityStartCallback;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * A payment integration test for checking whether user can make a payment using a payment app.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PaymentRequestPaymentAppCanMakePaymentQueryTest implements MainActivityStartCallback {
    @Rule
    public PaymentRequestTestRule mPaymentRequestTestRule = new PaymentRequestTestRule(
            "payment_request_can_make_payment_query_bobpay_test.html", this);

    @Override
    public void onMainActivityStarted() throws InterruptedException, ExecutionException,
            TimeoutException {}

    @Before
    public void setUp() {
        PaymentRequestImpl.setIsLocalCanMakePaymentQueryQuotaEnforcedForTest();
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testNoBobPayInstalled()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, false"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "otherBuy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, NotAllowedError"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testBobPayInstalledLater()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, false"});

        mPaymentRequestTestRule.installPaymentApp(HAVE_INSTRUMENTS, IMMEDIATE_RESPONSE);

        Thread.sleep(10000);
        mPaymentRequestTestRule.clickNodeAndWait(
                "otherBuy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        Thread.sleep(10000);
        mPaymentRequestTestRule.expectResultContains(new String[] {"true, NotAllowedError"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testNoInstrumentsInFastBobPay()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.installPaymentApp(NO_INSTRUMENTS, IMMEDIATE_RESPONSE);
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, false"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "otherBuy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, NotAllowedError"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testNoInstrumentsInSlowBobPay()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.installPaymentApp(NO_INSTRUMENTS, DELAYED_RESPONSE);
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, false"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "otherBuy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false, NotAllowedError"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testPayViaFastBobPay()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.installPaymentApp(HAVE_INSTRUMENTS, IMMEDIATE_RESPONSE);
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true, true"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "otherBuy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true, NotAllowedError"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testPayViaSlowBobPay()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.installPaymentApp(HAVE_INSTRUMENTS, DELAYED_RESPONSE);
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true, true"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "otherBuy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true, NotAllowedError"});
    }
}
