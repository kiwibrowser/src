// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;

import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Helper class for testing AutofillProfiles.
 */
public class AutofillTestHelper {

    private final CallbackHelper mOnPersonalDataChangedHelper = new CallbackHelper();

    public AutofillTestHelper() {
        registerDataObserver();
        setRequestTimeoutForTesting();
        setSyncServiceForTesting();
    }

    void setRequestTimeoutForTesting() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().setRequestTimeoutForTesting(0));
    }

    void setSyncServiceForTesting() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().setSyncServiceForTesting());
    }

    AutofillProfile getProfile(final String guid) throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getProfile(guid));
    }

    List<AutofillProfile> getProfilesToSuggest(final boolean includeNameInLabel) throws
            ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getProfilesToSuggest(includeNameInLabel));
    }

    List<AutofillProfile> getProfilesForSettings() throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getProfilesForSettings());
    }

    int getNumberOfProfilesToSuggest() throws ExecutionException {
        return getProfilesToSuggest(false).size();
    }

    int getNumberOfProfilesForSettings() throws ExecutionException {
        return getProfilesForSettings().size();
    }

    public String setProfile(final AutofillProfile profile) throws InterruptedException,
            ExecutionException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        String guid = ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().setProfile(profile));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
        return guid;
    }

    public void deleteProfile(final String guid) throws InterruptedException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().deleteProfile(guid));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    public CreditCard getCreditCard(final String guid) throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getCreditCard(guid));
    }

    List<CreditCard> getCreditCardsToSuggest() throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getCreditCardsToSuggest());
    }

    List<CreditCard> getCreditCardsForSettings() throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getCreditCardsForSettings());
    }

    int getNumberOfCreditCardsToSuggest() throws ExecutionException {
        return getCreditCardsToSuggest().size();
    }

    int getNumberOfCreditCardsForSettings() throws ExecutionException {
        return getCreditCardsForSettings().size();
    }

    public String setCreditCard(final CreditCard card) throws InterruptedException,
            ExecutionException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        String guid = ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().setCreditCard(card));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
        return guid;
    }

    public void addServerCreditCard(final CreditCard card)
            throws InterruptedException, ExecutionException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().addServerCreditCardForTest(card));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    void deleteCreditCard(final String guid) throws InterruptedException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().deleteCreditCard(guid));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    /**
     * Records the use of the profile associated with the specified {@code guid}.. Effectively
     * increments the use count of the profile and set its use date to the current time. Also logs
     * usage metrics.
     *
     * @param guid The GUID of the profile.
     */
    void recordAndLogProfileUse(final String guid) throws InterruptedException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().recordAndLogProfileUse(guid));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    /**
     * Sets the use {@code count} and use {@code date} of the test profile associated with the
     * {@code guid}. This update is not saved to disk.
     *
     * @param guid The GUID of the profile to modify.
     * @param count The use count to assign to the profile. It should be non-negative.
     * @param date The use date to assign to the profile. It represents an absolute point in
     *             coordinated universal time (UTC) represented as microseconds since the Windows
     *             epoch. For more details see the comment header in time.h. It should always be a
     *             positive number.
     */
    public void setProfileUseStatsForTesting(final String guid, final int count, final long date)
            throws InterruptedException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().setProfileUseStatsForTesting(guid, count,
                        date));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    /**
     * Get the use count of the test profile associated with the {@code guid}.
     *
     * @param guid The GUID of the profile to query.
     * @return The non-negative use count of the profile.
     */
    public int getProfileUseCountForTesting(final String guid) throws InterruptedException,
            ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getProfileUseCountForTesting(guid));
    }

    /**
     * Get the use date of the test profile associated with the {@code guid}.
     *
     * @param guid The GUID of the profile to query.
     * @return A non-negative long representing the last use date of the profile. It represents an
     *         absolute point in coordinated universal time (UTC) represented as microseconds since
     *         the Windows epoch. For more details see the comment header in time.h.
     */
    public long getProfileUseDateForTesting(final String guid) throws InterruptedException,
            ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getProfileUseDateForTesting(guid));
    }

    /**
     * Records the use of the credit card associated with the specified {@code guid}. Effectively
     * increments the use count of the credit card and sets its use date to the current time. Also
     * logs usage metrics.
     *
     * @param guid The GUID of the credit card.
     */
    public void recordAndLogCreditCardUse(final String guid) throws InterruptedException,
            TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().recordAndLogCreditCardUse(guid));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    /**
     * Sets the use {@code count} and use {@code date} of the test credit card associated with the
     * {@code guid}. This update is not saved to disk.
     *
     * @param guid The GUID of the credit card to modify.
     * @param count The use count to assign to the credit card. It should be non-negative.
     * @param date The use date to assign to the credit card. It represents an absolute point in
     *             coordinated universal time (UTC) represented as microseconds since the Windows
     *             epoch. For more details see the comment header in time.h. It should always be a
     *             positive number.
     */
    public void setCreditCardUseStatsForTesting(final String guid, final int count, final long date)
            throws InterruptedException, TimeoutException {
        int callCount = mOnPersonalDataChangedHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().setCreditCardUseStatsForTesting(
                        guid, count, date));
        mOnPersonalDataChangedHelper.waitForCallback(callCount);
    }

    /**
     * Get the use count of the test credit card associated with the {@code guid}.
     *
     * @param guid The GUID of the credit card to query.
     * @return The non-negative use count of the credit card.
     */
    public int getCreditCardUseCountForTesting(final String guid) throws InterruptedException,
            ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getCreditCardUseCountForTesting(guid));
    }

    /**
     * Get the use date of the test credit card associated with the {@code guid}.
     *
     * @param guid The GUID of the credit card to query.
     * @return A non-negative long representing the last use date of the credit card. It represents
     *         an absolute point in coordinated universal time (UTC) represented as microseconds
     *         since the Windows epoch. For more details see the comment header in time.h.
     */
    public long getCreditCardUseDateForTesting(final String guid) throws InterruptedException,
            ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getCreditCardUseDateForTesting(guid));
    }

    /**
     * Get the current use date to be used in test to compare with credit card or profile use dates.
     *
     * @return A non-negative long representing the current date. It represents an absolute point in
     *         coordinated universal time (UTC) represented as microseconds since the Windows epoch.
     *         For more details see the comment header in time.h.
     */
    public long getCurrentDateForTesting() throws InterruptedException, ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> PersonalDataManager.getInstance().getCurrentDateForTesting());
    }

    private void registerDataObserver() {
        try {
            int callCount = mOnPersonalDataChangedHelper.getCallCount();
            boolean isDataLoaded = ThreadUtils.runOnUiThreadBlocking(
                    () -> PersonalDataManager.getInstance().registerDataObserver(
                            () -> mOnPersonalDataChangedHelper.notifyCalled()));
            if (isDataLoaded) return;
            mOnPersonalDataChangedHelper.waitForCallback(callCount);
        } catch (TimeoutException | InterruptedException | ExecutionException e) {
            throw new AssertionError(e);
        }
    }
}
