// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.Context;
import android.support.annotation.IntDef;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Pair;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.preferences.autofill.AutofillProfileBridge;
import org.chromium.chrome.browser.preferences.autofill.AutofillProfileBridge.AddressField;
import org.chromium.chrome.browser.widget.prefeditor.EditableOption;
import org.chromium.payments.mojom.PaymentAddress;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.annotation.Nullable;

/**
 * The locally stored autofill address.
 */
public class AutofillAddress extends EditableOption {
    /** The pattern for a valid region code. */
    private static final String REGION_CODE_PATTERN = "^[A-Z]{2}$";

    // Language/script code pattern and capture group numbers.
    private static final String LANGUAGE_SCRIPT_CODE_PATTERN =
            "^([a-z]{2})(-([A-Z][a-z]{3}))?(-[A-Za-z]+)*$";
    private static final int LANGUAGE_CODE_GROUP = 1;
    private static final int SCRIPT_CODE_GROUP = 3;

    @IntDef({COMPLETE, INVALID_ADDRESS, INVALID_PHONE_NUMBER, INVALID_RECIPIENT,
            INVALID_MULTIPLE_FIELDS})
    @Retention(RetentionPolicy.SOURCE)
    public @interface CompletionStatus {}
    /** Can be sent to the merchant as-is without editing first. */
    public static final int COMPLETE = 0;
    /** The address is invalid. For example, missing state or city name. */
    public static final int INVALID_ADDRESS = 1;
    /** The phone number is invalid or missing. */
    public static final int INVALID_PHONE_NUMBER = 2;
    /** The recipient is missing. */
    public static final int INVALID_RECIPIENT = 3;
    /** Multiple fields are invalid or missing. */
    public static final int INVALID_MULTIPLE_FIELDS = 4;

    @IntDef({NORMAL_COMPLETENESS_CHECK, IGNORE_PHONE_COMPLETENESS_CHECK})
    @Retention(RetentionPolicy.SOURCE)
    public @interface CompletenessCheckType {}
    /** A normal completeness check. */
    public static final int NORMAL_COMPLETENESS_CHECK = 0;
    /** A completeness check that ignores phone numbers. */
    public static final int IGNORE_PHONE_COMPLETENESS_CHECK = 1;

    @Nullable private static Pattern sRegionCodePattern;

    private Context mContext;
    private AutofillProfile mProfile;
    @Nullable private Pattern mLanguageScriptCodePattern;
    @Nullable private String mShippingLabelWithCountry;
    @Nullable private String mShippingLabelWithoutCountry;
    @Nullable private String mBillingLabel;

    /**
     * Builds the autofill address.
     *
     * @param profile The autofill profile containing the address information.
     */
    public AutofillAddress(Context context, AutofillProfile profile) {
        super(profile.getGUID(), profile.getFullName(), profile.getLabel(),
                profile.getPhoneNumber(), null);
        mContext = context;
        mProfile = profile;
        mIsEditable = true;
        checkAndUpdateAddressCompleteness();
    }

    /** @return The autofill profile where this address data lives. */
    public AutofillProfile getProfile() {
        return mProfile;
    }

    /**
     * Updates the address and marks it "complete." Called after the user has edited this address.
     * Updates the identifier and labels.
     *
     * @param profile The new profile to use.
     */
    public void completeAddress(AutofillProfile profile) {
        // Since the profile changed, our cached labels are now out of date. Set them to null so the
        // labels are recomputed next time they are needed.
        mShippingLabelWithCountry = null;
        mShippingLabelWithoutCountry = null;
        mBillingLabel = null;

        mProfile = profile;
        updateIdentifierAndLabels(mProfile.getGUID(), mProfile.getFullName(), mProfile.getLabel(),
                mProfile.getPhoneNumber());
        checkAndUpdateAddressCompleteness();
        assert mIsComplete;
    }

    /**
     * Gets the shipping address label which includes the country for the profile associated with
     * this address and sets it as sublabel for this EditableOption.
     */
    public void setShippingAddressLabelWithCountry() {
        assert mProfile != null;

        if (mShippingLabelWithCountry == null) {
            mShippingLabelWithCountry =
                    PersonalDataManager.getInstance()
                            .getShippingAddressLabelWithCountryForPaymentRequest(mProfile);
        }

        mProfile.setLabel(mShippingLabelWithCountry);
        updateSublabel(mProfile.getLabel());
    }

    /**
     * Gets the shipping address label which does not include the country for the profile associated
     * with this address and sets it as sublabel for this EditableOption.
     */
    public void setShippingAddressLabelWithoutCountry() {
        assert mProfile != null;

        if (mShippingLabelWithoutCountry == null) {
            mShippingLabelWithoutCountry =
                    PersonalDataManager.getInstance()
                            .getShippingAddressLabelWithoutCountryForPaymentRequest(mProfile);
        }

        mProfile.setLabel(mShippingLabelWithoutCountry);
        updateSublabel(mProfile.getLabel());
    }

    /*
     * Gets the billing address label for the profile associated with this address and sets it as
     * sublabel for this EditableOption.
     */
    public void setBillingAddressLabel() {
        assert mProfile != null;

        if (mBillingLabel == null) {
            mBillingLabel =
                    PersonalDataManager.getInstance().getBillingAddressLabelForPaymentRequest(
                            mProfile);
        }

        mProfile.setLabel(mBillingLabel);
        updateSublabel(mProfile.getLabel());
    }

    /**
     * Checks whether this address is complete and updates edit message, edit title and complete
     * status.
     */
    private void checkAndUpdateAddressCompleteness() {
        Pair<Integer, Integer> messageResIds = getEditMessageAndTitleResIds(
                checkAddressCompletionStatus(mProfile, NORMAL_COMPLETENESS_CHECK));

        mEditMessage = messageResIds.first.intValue() == 0
                ? null
                : mContext.getString(messageResIds.first);
        mEditTitle = messageResIds.second.intValue() == 0
                ? null
                : mContext.getString(messageResIds.second);
        mIsComplete = mEditMessage == null;
    }

    /**
     * Gets the edit message and title resource Ids for the completion status.
     *
     * @param  completionStatus The completion status.
     * @return The resource Ids. The first is the edit message resource Id. The second is the
     *         correspond editor title resource Id.
     */
    public static Pair<Integer, Integer> getEditMessageAndTitleResIds(
            @CompletionStatus int completionStatus) {
        int editMessageResId = 0;
        int editTitleResId = 0;

        switch (completionStatus) {
            case COMPLETE:
                editTitleResId = R.string.payments_edit_address;
                break;
            case INVALID_ADDRESS:
                editMessageResId = R.string.payments_invalid_address;
                editTitleResId = R.string.payments_add_valid_address;
                break;
            case INVALID_PHONE_NUMBER:
                editMessageResId = R.string.payments_phone_number_required;
                editTitleResId = R.string.payments_add_phone_number;
                break;
            case INVALID_RECIPIENT:
                editMessageResId = R.string.payments_recipient_required;
                editTitleResId = R.string.payments_add_recipient;
                break;
            case INVALID_MULTIPLE_FIELDS:
                editMessageResId = R.string.payments_more_information_required;
                editTitleResId = R.string.payments_add_more_information;
                break;
            default:
                assert false : "Invalid completion status";
        }

        return new Pair<Integer, Integer>(editMessageResId, editTitleResId);
    }

    /**
     * Checks address completion status in the given profile.
     *
     * If the country code is not set or invalid, but all fields for the default locale's country
     * code are present, then the profile is deemed "complete." AutoflllAddress.toPaymentAddress()
     * will use the default locale to fill in a blank country code before sending the address to the
     * renderer.
     *
     * @param  profile The autofill profile containing the address information.
     * @return int     The completion status.
     */
    @CompletionStatus
    public static int checkAddressCompletionStatus(
            AutofillProfile profile, @CompletenessCheckType int checkType) {
        int invalidFieldsCount = 0;
        int completionStatus = COMPLETE;

        if (checkType != IGNORE_PHONE_COMPLETENESS_CHECK
                && !PhoneNumberUtils.isGlobalPhoneNumber(
                           PhoneNumberUtils.stripSeparators(profile.getPhoneNumber().toString()))) {
            completionStatus = INVALID_PHONE_NUMBER;
            invalidFieldsCount++;
        }

        List<Integer> requiredFields = AutofillProfileBridge.getRequiredAddressFields(
                AutofillAddress.getCountryCode(profile));
        for (int fieldId : requiredFields) {
            if (fieldId == AddressField.RECIPIENT || fieldId == AddressField.COUNTRY) continue;
            if (!TextUtils.isEmpty(getProfileField(profile, fieldId))) continue;
            completionStatus = INVALID_ADDRESS;
            invalidFieldsCount++;
            break;
        }

        if (TextUtils.isEmpty(profile.getFullName())) {
            completionStatus = INVALID_RECIPIENT;
            invalidFieldsCount++;
        }

        if (invalidFieldsCount > 1) {
            completionStatus = INVALID_MULTIPLE_FIELDS;
        }

        return completionStatus;
    }

    /** @return The given autofill profile field. */
    public static String getProfileField(AutofillProfile profile, int field) {
        assert profile != null;
        switch (field) {
            case AddressField.COUNTRY:
                return profile.getCountryCode();
            case AddressField.ADMIN_AREA:
                return profile.getRegion();
            case AddressField.LOCALITY:
                return profile.getLocality();
            case AddressField.DEPENDENT_LOCALITY:
                return profile.getDependentLocality();
            case AddressField.SORTING_CODE:
                return profile.getSortingCode();
            case AddressField.POSTAL_CODE:
                return profile.getPostalCode();
            case AddressField.STREET_ADDRESS:
                return profile.getStreetAddress();
            case AddressField.ORGANIZATION:
                return profile.getCompanyName();
            case AddressField.RECIPIENT:
                return profile.getFullName();
        }

        assert false;
        return null;
    }

    /** @return The country code to use, e.g., when constructing an editor for this address. */
    public static String getCountryCode(@Nullable AutofillProfile profile) {
        if (sRegionCodePattern == null) sRegionCodePattern = Pattern.compile(REGION_CODE_PATTERN);

        return profile == null || TextUtils.isEmpty(profile.getCountryCode())
                        || !sRegionCodePattern.matcher(profile.getCountryCode()).matches()
                ? Locale.getDefault().getCountry() : profile.getCountryCode();
    }

    /** @return The address for the merchant. */
    public PaymentAddress toPaymentAddress() {
        assert mIsComplete;
        PaymentAddress result = new PaymentAddress();

        result.country = getCountryCode(mProfile);
        result.addressLine = mProfile.getStreetAddress().split("\n");
        result.region = mProfile.getRegion();
        result.city = mProfile.getLocality();
        result.dependentLocality = mProfile.getDependentLocality();
        result.postalCode = mProfile.getPostalCode();
        result.sortingCode = mProfile.getSortingCode();
        result.organization = mProfile.getCompanyName();
        result.recipient = mProfile.getFullName();
        result.languageCode = "";
        result.scriptCode = "";
        result.phone = mProfile.getPhoneNumber();

        if (mProfile.getLanguageCode() == null) return result;

        if (mLanguageScriptCodePattern == null) {
            mLanguageScriptCodePattern = Pattern.compile(LANGUAGE_SCRIPT_CODE_PATTERN);
        }

        Matcher matcher = mLanguageScriptCodePattern.matcher(mProfile.getLanguageCode());
        if (matcher.matches()) {
            result.languageCode = ensureNotNull(matcher.group(LANGUAGE_CODE_GROUP));
            result.scriptCode = ensureNotNull(matcher.group(SCRIPT_CODE_GROUP));
        }

        return result;
    }

    private static String ensureNotNull(@Nullable String value) {
        return value == null ? "" : value;
    }
}
