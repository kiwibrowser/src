// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.DialogInterface;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.FlakyTest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.CardType;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;
import org.chromium.chrome.browser.payments.PaymentRequestTestRule.MainActivityStartCallback;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.RenderTestRule;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * A payment integration test for a merchant that provides free shipping regardless of address.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PaymentRequestFreeShippingTest implements MainActivityStartCallback {
    @Rule
    public PaymentRequestTestRule mPaymentRequestTestRule =
            new PaymentRequestTestRule("payment_request_free_shipping_test.html", this);

    @Rule
    public RenderTestRule mRenderTestRule =
            new RenderTestRule("components/test/data/payments/render_tests");

    @Override
    public void onMainActivityStarted()
            throws InterruptedException, ExecutionException, TimeoutException {
        AutofillTestHelper helper = new AutofillTestHelper();
        // The user has a shipping address on disk.
        String billingAddressId = helper.setProfile(new AutofillProfile("", "https://example.com",
                true, "Jon Doe", "Google", "340 Main St", "CA", "Los Angeles", "", "90291", "",
                "US", "650-253-0000", "", "en-US"));
        helper.setCreditCard(new CreditCard("", "https://example.com", true, true, "Jon Doe",
                "4111111111111111", "1111", "12", "2050", "visa", R.drawable.visa_card,
                CardType.UNKNOWN, billingAddressId, "" /* serverId */));
    }

    /** Submit the shipping address to the merchant when the user clicks "Pay." */
    @Test
    @MediumTest
    @Feature({"Payments", "RenderTest"})
    public void testPayWithRender() throws Throwable {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mRenderTestRule.render(mPaymentRequestTestRule.getPaymentRequestView(), "free_shipping");
        mPaymentRequestTestRule.clickAndWait(
                R.id.button_primary, mPaymentRequestTestRule.getReadyForUnmaskInput());
        mRenderTestRule.render(mPaymentRequestTestRule.getCardUnmaskView(), "unmask");
        mPaymentRequestTestRule.setTextInCardUnmaskDialogAndWait(
                R.id.card_unmask_input, "123", mPaymentRequestTestRule.getReadyToUnmask());
        mPaymentRequestTestRule.clickCardUnmaskButtonAndWait(
                DialogInterface.BUTTON_POSITIVE, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {"Jon Doe", "4111111111111111",
                "12", "2050", "basic-card", "123", "Google", "340 Main St", "CA", "Los Angeles",
                "90291", "US", "en", "freeShippingOption"});
    }

    /** Attempt to add an invalid address and cancel the transaction. */
    @Test
    @MediumTest
    @FlakyTest(message = "crbug.com/673371")
    @Feature({"Payments"})
    public void testAddInvalidAddressAndCancel()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_add_option_button, mPaymentRequestTestRule.getReadyToEdit());
        mPaymentRequestTestRule.clickInEditorAndWait(
                R.id.editor_dialog_done_button, mPaymentRequestTestRule.getEditorValidationError());
        mPaymentRequestTestRule.clickInEditorAndWait(
                R.id.payments_edit_cancel_button, mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickAndWait(
                R.id.close_button, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {"Request cancelled"});
    }

    /** Add a valid address and complete the transaction. */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testAddAddressAndPay()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_add_option_button, mPaymentRequestTestRule.getReadyToEdit());
        mPaymentRequestTestRule.setTextInEditorAndWait(
                new String[] {"Bob", "Google", "1600 Amphitheatre Pkwy", "Mountain View", "CA",
                        "94043", "650-253-0000"},
                mPaymentRequestTestRule.getEditorTextUpdate());
        mPaymentRequestTestRule.clickInEditorAndWait(
                R.id.editor_dialog_done_button, mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickAndWait(
                R.id.button_primary, mPaymentRequestTestRule.getReadyForUnmaskInput());
        mPaymentRequestTestRule.setTextInCardUnmaskDialogAndWait(
                R.id.card_unmask_input, "123", mPaymentRequestTestRule.getReadyToUnmask());
        mPaymentRequestTestRule.clickCardUnmaskButtonAndWait(
                DialogInterface.BUTTON_POSITIVE, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {"Bob", "Google",
                "1600 Amphitheatre Pkwy", "Mountain View", "CA", "94043", "+16502530000"});
    }

    /** Change the country in the spinner, add a valid address, and complete the transaction. */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testChangeCountryAddAddressAndPay()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_add_option_button, mPaymentRequestTestRule.getReadyToEdit());
        mPaymentRequestTestRule.setSpinnerSelectionInEditorAndWait(
                0 /* Afghanistan */, mPaymentRequestTestRule.getReadyToEdit());
        mPaymentRequestTestRule.setTextInEditorAndWait(
                new String[] {
                        "Alice", "Supreme Court", "Airport Road", "Kabul", "1043", "020-253-0000"},
                mPaymentRequestTestRule.getEditorTextUpdate());
        mPaymentRequestTestRule.clickInEditorAndWait(
                R.id.editor_dialog_done_button, mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickAndWait(
                R.id.button_primary, mPaymentRequestTestRule.getReadyForUnmaskInput());
        mPaymentRequestTestRule.setTextInCardUnmaskDialogAndWait(
                R.id.card_unmask_input, "123", mPaymentRequestTestRule.getReadyToUnmask());
        mPaymentRequestTestRule.clickCardUnmaskButtonAndWait(
                DialogInterface.BUTTON_POSITIVE, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {
                "Alice", "Supreme Court", "Airport Road", "Kabul", "1043", "+93202530000"});
    }

    /** Quickly pressing on "add address" and then [X] should not crash. */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testQuickAddAddressAndCloseShouldNotCrash()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());

        // Quickly press on "add address" and then [X].
        int callCount = mPaymentRequestTestRule.getReadyToEdit().getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getShippingAddressSectionForTest()
                    .findViewById(R.id.payments_add_option_button)
                    .performClick();
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getDialogForTest()
                    .findViewById(R.id.close_button)
                    .performClick();
        });
        mPaymentRequestTestRule.getReadyToEdit().waitForCallback(callCount);

        mPaymentRequestTestRule.clickInEditorAndWait(
                R.id.payments_edit_cancel_button, mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickAndWait(
                R.id.close_button, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {"Request cancelled"});
    }

    /** Quickly pressing on [X] and then "add address" should not crash. */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testQuickCloseAndAddAddressShouldNotCrash()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());

        // Quickly press on [X] and then "add address."
        int callCount = mPaymentRequestTestRule.getDismissed().getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getDialogForTest()
                    .findViewById(R.id.close_button)
                    .performClick();
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getShippingAddressSectionForTest()
                    .findViewById(R.id.payments_add_option_button)
                    .performClick();
        });
        mPaymentRequestTestRule.getDismissed().waitForCallback(callCount);

        mPaymentRequestTestRule.expectResultContains(new String[] {"Request cancelled"});
    }

    /** Quickly pressing on "add address" and then "cancel" should not crash. */
    @Test
    @MediumTest
    @FlakyTest(message = "crbug.com/673371")
    @Feature({"Payments"})
    public void testQuickAddAddressAndCancelShouldNotCrash()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());

        // Quickly press on "add address" and then "cancel."
        int callCount = mPaymentRequestTestRule.getReadyToEdit().getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getShippingAddressSectionForTest()
                    .findViewById(R.id.payments_add_option_button)
                    .performClick();
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getDialogForTest()
                    .findViewById(R.id.button_secondary)
                    .performClick();
        });
        mPaymentRequestTestRule.getReadyToEdit().waitForCallback(callCount);

        mPaymentRequestTestRule.clickInEditorAndWait(
                R.id.payments_edit_cancel_button, mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickAndWait(
                R.id.close_button, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {"Request cancelled"});
    }

    /** Quickly pressing on "cancel" and then "add address" should not crash. */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testQuickCancelAndAddAddressShouldNotCrash()
            throws InterruptedException, ExecutionException, TimeoutException {
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());

        // Quickly press on "cancel" and then "add address."
        int callCount = mPaymentRequestTestRule.getDismissed().getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getDialogForTest()
                    .findViewById(R.id.button_secondary)
                    .performClick();
            mPaymentRequestTestRule.getPaymentRequestUI()
                    .getShippingAddressSectionForTest()
                    .findViewById(R.id.payments_add_option_button)
                    .performClick();
        });
        mPaymentRequestTestRule.getDismissed().waitForCallback(callCount);

        mPaymentRequestTestRule.expectResultContains(new String[] {"Request cancelled"});
    }

    /**
     * Test that ending a payment request that requires only the shipping address results in the
     * appropriate metric being logged in PaymentRequest.Events.
     */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testPaymentRequestEventsMetric()
            throws InterruptedException, ExecutionException, TimeoutException {
        // Start and abort the Payment Request.
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyToPay());
        mPaymentRequestTestRule.clickAndWait(
                R.id.close_button, mPaymentRequestTestRule.getDismissed());
        mPaymentRequestTestRule.expectResultContains(new String[] {"Request cancelled"});

        int expectedSample = Event.SHOWN | Event.USER_ABORTED | Event.HAD_INITIAL_FORM_OF_PAYMENT
                | Event.HAD_NECESSARY_COMPLETE_SUGGESTIONS | Event.REQUEST_SHIPPING
                | Event.REQUEST_METHOD_BASIC_CARD;
        Assert.assertEquals(1,
                RecordHistogram.getHistogramValueCountForTesting(
                        "PaymentRequest.Events", expectedSample));
    }
}
