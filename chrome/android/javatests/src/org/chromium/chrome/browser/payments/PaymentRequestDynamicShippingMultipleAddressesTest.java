// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.payments.PaymentRequestTestRule.MainActivityStartCallback;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.ArrayList;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * A payment integration test for a merchant that requires shipping address to calculate shipping
 * and user that has 5 addresses stored in autofill settings.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PaymentRequestDynamicShippingMultipleAddressesTest
        implements MainActivityStartCallback {
    @Rule
    public PaymentRequestTestRule mPaymentRequestTestRule =
            new PaymentRequestTestRule("payment_request_dynamic_shipping_test.html", this);

    private static final AutofillProfile[] AUTOFILL_PROFILES = {
            // Incomplete profile (missing phone number)
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */,
                    "Bart Simpson", "Acme Inc.", "123 Main", "California", "Los Angeles", "",
                    "90210", "", "US", "", "bart@simpson.com", ""),

            // Incomplete profile (missing street address).
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */,
                    "Homer Simpson", "Acme Inc.", "", "California", "Los Angeles", "", "90210", "",
                    "US", "555 123-4567", "homer@simpson.com", ""),

            // Complete profile.
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */,
                    "Lisa Simpson", "Acme Inc.", "123 Main", "California", "Los Angeles", "",
                    "90210", "", "US", "555 123-4567", "lisa@simpson.com", ""),

            // Complete profile in another country.
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */,
                    "Maggie Simpson", "Acme Inc.", "123 Main", "California", "Los Angeles", "",
                    "90210", "", "Uzbekistan", "555 123-4567", "maggie@simpson.com", ""),

            // Incomplete profile (invalid address).
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */,
                    "Marge Simpson", "Acme Inc.", "123 Main", "California", "", "", "90210", "",
                    "US", "555 123-4567", "marge@simpson.com", ""),

            // Incomplete profile (missing recipient name).
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */, "",
                    "Acme Inc.", "123 Main", "California", "Los Angeles", "", "90210", "", "US",
                    "555 123-4567", "lisa@simpson.com", ""),

            // Incomplete profile (need more information).
            new AutofillProfile("" /* guid */, "https://www.example.com" /* origin */, "",
                    "Acme Inc.", "123 Main", "California", "", "", "90210", "", "US",
                    "555 123-4567", "lisa@simpson.com", ""),
    };

    private AutofillProfile[] mProfilesToAdd;
    private int[] mCountsToSet;
    private int[] mDatesToSet;

    @Override
    public void onMainActivityStarted()
            throws InterruptedException, ExecutionException, TimeoutException {
        AutofillTestHelper helper = new AutofillTestHelper();

        // Add the profiles.
        ArrayList<String> guids = new ArrayList<>();
        for (int i = 0; i < mProfilesToAdd.length; i++) {
            guids.add(helper.setProfile(mProfilesToAdd[i]));
        }

        // Set up the profile use stats.
        for (int i = 0; i < guids.size(); i++) {
            helper.setProfileUseStatsForTesting(guids.get(i), mCountsToSet[i], mDatesToSet[i]);
        }
    }

    /**
     * Make sure the address suggestions are in the correct order and that only the top 4
     * suggestions are shown. They should be ordered by frecency and complete addresses should be
     * suggested first.
     */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testShippingAddressSuggestionOrdering()
            throws InterruptedException, ExecutionException, TimeoutException {
        // Create two complete and two incomplete profiles. Values are set so that the profiles are
        // ordered by frecency.
        mProfilesToAdd = new AutofillProfile[] {
                AUTOFILL_PROFILES[0], AUTOFILL_PROFILES[2], AUTOFILL_PROFILES[3],
                AUTOFILL_PROFILES[4]};
        mCountsToSet = new int[] {20, 15, 10, 5, 1};
        mDatesToSet = new int[] {5000, 5000, 5000, 5000, 1};

        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        Assert.assertEquals(4, mPaymentRequestTestRule.getNumberOfShippingAddressSuggestions());
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(0).contains(
                "Lisa Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(1).contains(
                "Maggie Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(2).contains(
                "Bart Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(3).contains(
                "Marge Simpson"));
    }

    /**
     * Make sure that a maximum of four profiles are shown to the user.
     */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testShippingAddressSuggestionLimit()
            throws InterruptedException, ExecutionException, TimeoutException {
        // Create five profiles that can be suggested to the user.
        mProfilesToAdd = new AutofillProfile[] {
                AUTOFILL_PROFILES[0], AUTOFILL_PROFILES[2], AUTOFILL_PROFILES[3],
                AUTOFILL_PROFILES[4], AUTOFILL_PROFILES[5]};
        mCountsToSet = new int[] {20, 15, 10, 5, 2, 1};
        mDatesToSet = new int[] {5000, 5000, 5000, 5000, 2, 1};

        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        // Only four profiles should be suggested to the user.
        Assert.assertEquals(4, mPaymentRequestTestRule.getNumberOfShippingAddressSuggestions());
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(0).contains(
                "Lisa Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(1).contains(
                "Maggie Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(2).contains(
                "Bart Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(3).contains(
                "Marge Simpson"));
    }

    /**
     * Make sure that only profiles with a street address are suggested to the user.
     */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testShippingAddressSuggestion_OnlyIncludeProfilesWithStreetAddress()
            throws InterruptedException, ExecutionException, TimeoutException {
        // Create two complete profiles and two incomplete profiles, one of which has no street
        // address.
        mProfilesToAdd = new AutofillProfile[] {
                AUTOFILL_PROFILES[0], AUTOFILL_PROFILES[1], AUTOFILL_PROFILES[2],
                AUTOFILL_PROFILES[3]};
        mCountsToSet = new int[] {15, 10, 5, 1};
        mDatesToSet = new int[] {5000, 5000, 5000, 1};

        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        // Only 3 profiles should be suggested, the two complete ones and the incomplete one that
        // has a street address.
        Assert.assertEquals(3, mPaymentRequestTestRule.getNumberOfShippingAddressSuggestions());
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(0).contains(
                "Lisa Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(1).contains(
                "Maggie Simpson"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(2).contains(
                "Bart Simpson"));
    }

    /**
     * Select a shipping address that the website refuses to accept, which should force the dialog
     * to show an error.
     */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testShippingAddresNotAcceptedByMerchant()
            throws InterruptedException, ExecutionException, TimeoutException {
        // Add a profile that is not accepted by the website.
        mProfilesToAdd = new AutofillProfile[] {AUTOFILL_PROFILES[3]};
        mCountsToSet = new int[] {5};
        mDatesToSet = new int[] {5000};

        // Click on the unacceptable shipping address.
        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(0).contains(
                AUTOFILL_PROFILES[3].getFullName()));
        mPaymentRequestTestRule.clickOnShippingAddressSuggestionOptionAndWait(
                0, mPaymentRequestTestRule.getSelectionChecked());

        // The string should reflect the error sent from the merchant.
        CharSequence actualString =
                mPaymentRequestTestRule.getShippingAddressOptionRowAtIndex(0).getLabelText();
        Assert.assertEquals("We do not ship to this address", actualString);
    }

    /**
     * Make sure the information required message has been displayed for incomplete profile
     * correctly.
     */
    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testShippingAddressEditRequiredMessage()
            throws InterruptedException, ExecutionException, TimeoutException {
        // Create four incomplete profiles with different missing information.
        mProfilesToAdd = new AutofillProfile[] {AUTOFILL_PROFILES[0], AUTOFILL_PROFILES[4],
                AUTOFILL_PROFILES[5], AUTOFILL_PROFILES[6]};
        mCountsToSet = new int[] {15, 10, 5, 1};
        mDatesToSet = new int[] {5000, 5000, 5000, 1};

        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        mPaymentRequestTestRule.clickInShippingAddressAndWait(
                R.id.payments_section, mPaymentRequestTestRule.getReadyForInput());

        Assert.assertEquals(4, mPaymentRequestTestRule.getNumberOfShippingAddressSuggestions());
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(0).contains(
                "Phone number required"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(1).contains(
                "Enter a valid address"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(2).contains(
                "Name required"));
        Assert.assertTrue(mPaymentRequestTestRule.getShippingAddressSuggestionLabel(3).contains(
                "More information required"));
    }
}
