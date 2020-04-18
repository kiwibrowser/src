// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.support.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.CardType;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;
import org.chromium.chrome.browser.payments.PaymentRequestTestRule.MainActivityStartCallback;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * A payment integration test for checking whether user can make a payment via a credit card. This
 * user has a valid  credit card without a billing address on file.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PaymentRequestCcCanMakePaymentQueryTest implements MainActivityStartCallback {
    @Rule
    public PaymentRequestTestRule mPaymentRequestTestRule =
            new PaymentRequestTestRule("payment_request_can_make_payment_query_cc_test.html", this);

    @Before
    public void setUp() {
        PaymentRequestImpl.setIsLocalCanMakePaymentQueryQuotaEnforcedForTest();
    }

    @Override
    public void onMainActivityStarted() throws InterruptedException, ExecutionException,
            TimeoutException {
        // The user has a valid credit card without a billing address on file. This is sufficient
        // for canMakePayment() to return true.
        new AutofillTestHelper().setCreditCard(new CreditCard("", "https://example.com", true, true,
                "Jon Doe", "4111111111111111", "1111", "12", "2050", "visa", R.drawable.visa_card,
                CardType.UNKNOWN, "" /* billingAddressId */, "" /* serverId */));
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testCanMakePayment()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true"});

        // Repeating a query does not count against the quota.
        mPaymentRequestTestRule.clickNodeAndWait(
                "buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true"});

        // Different queries are throttled for a period of time.
        mPaymentRequestTestRule.clickNodeAndWait(
                "other-buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(
                new String[] {"Not allowed to check whether can make payment"});

        // Repeating the same query again does not count against the quota.
        mPaymentRequestTestRule.clickNodeAndWait(
                "buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"true"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testCanMakePaymentDisabled()
            throws InterruptedException, ExecutionException, TimeoutException {
        ThreadUtils.runOnUiThreadBlocking((Runnable) () -> {
            PrefServiceBridge.getInstance().setBoolean(Pref.CAN_MAKE_PAYMENT_ENABLED, false);
        });

        mPaymentRequestTestRule.openPageAndClickBuyAndWait(
                mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false"});

        // Repeating a query does not count against the quota.
        mPaymentRequestTestRule.clickNodeAndWait(
                "buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false"});

        mPaymentRequestTestRule.clickNodeAndWait(
                "buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false"});

        // Different queries are throttled for a period of time.
        mPaymentRequestTestRule.clickNodeAndWait(
                "other-buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(
                new String[] {"Not allowed to check whether can make payment"});

        // Repeating the same query again does not count against the quota.
        mPaymentRequestTestRule.clickNodeAndWait(
                "buy", mPaymentRequestTestRule.getCanMakePaymentQueryResponded());
        mPaymentRequestTestRule.expectResultContains(new String[] {"false"});
    }
}
