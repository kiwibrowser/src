/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.vcard;

import com.android.vcard.VCardUtils.PhoneNumberUtilsPort;

import android.accounts.Account;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Event;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Im;
import android.provider.ContactsContract.CommonDataKinds.Nickname;
import android.provider.ContactsContract.CommonDataKinds.Note;
import android.provider.ContactsContract.CommonDataKinds.Organization;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.provider.ContactsContract.CommonDataKinds.SipAddress;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.CommonDataKinds.StructuredPostal;
import android.provider.ContactsContract.CommonDataKinds.Website;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Represents one vCard entry, which should start with "BEGIN:VCARD" and end
 * with "END:VCARD". This class is for bridging between real vCard data and
 * Android's {@link ContactsContract}, which means some aspects of vCard are
 * dropped before this object being constructed. Raw vCard data should be first
 * supplied with {@link #addProperty(VCardProperty)}. After supplying all data,
 * user should call {@link #consolidateFields()} to prepare some additional
 * information which is constructable from supplied raw data. TODO: preserve raw
 * data using {@link VCardProperty}. If it may just waste memory, this at least
 * should contain them when it cannot convert vCard as a string to Android's
 * Contacts representation. Those raw properties should _not_ be used for
 * {@link #isIgnorable()}.
 */
public class VCardEntry {
    private static final String LOG_TAG = VCardConstants.LOG_TAG;

    private static final int DEFAULT_ORGANIZATION_TYPE = Organization.TYPE_WORK;

    private static final Map<String, Integer> sImMap = new HashMap<String, Integer>();

    static {
        sImMap.put(VCardConstants.PROPERTY_X_AIM, Im.PROTOCOL_AIM);
        sImMap.put(VCardConstants.PROPERTY_X_MSN, Im.PROTOCOL_MSN);
        sImMap.put(VCardConstants.PROPERTY_X_YAHOO, Im.PROTOCOL_YAHOO);
        sImMap.put(VCardConstants.PROPERTY_X_ICQ, Im.PROTOCOL_ICQ);
        sImMap.put(VCardConstants.PROPERTY_X_JABBER, Im.PROTOCOL_JABBER);
        sImMap.put(VCardConstants.PROPERTY_X_SKYPE_USERNAME, Im.PROTOCOL_SKYPE);
        sImMap.put(VCardConstants.PROPERTY_X_GOOGLE_TALK, Im.PROTOCOL_GOOGLE_TALK);
        sImMap.put(VCardConstants.ImportOnly.PROPERTY_X_GOOGLE_TALK_WITH_SPACE,
                Im.PROTOCOL_GOOGLE_TALK);
    }

    public enum EntryLabel {
        NAME,
        PHONE,
        EMAIL,
        POSTAL_ADDRESS,
        ORGANIZATION,
        IM,
        PHOTO,
        WEBSITE,
        SIP,
        NICKNAME,
        NOTE,
        BIRTHDAY,
        ANNIVERSARY,
        ANDROID_CUSTOM
    }

    public static interface EntryElement {
        // Also need to inherit toString(), equals().
        public EntryLabel getEntryLabel();

        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex);

        public boolean isEmpty();
    }

    // TODO: vCard 4.0 logically has multiple formatted names and we need to
    // select the most preferable one using PREF parameter.
    //
    // e.g. (based on rev.13)
    // FN;PREF=1:John M. Doe
    // FN;PREF=2:John Doe
    // FN;PREF=3;John
    public static class NameData implements EntryElement {
        private String mFamily;
        private String mGiven;
        private String mMiddle;
        private String mPrefix;
        private String mSuffix;

        // Used only when no family nor given name is found.
        private String mFormatted;

        private String mPhoneticFamily;
        private String mPhoneticGiven;
        private String mPhoneticMiddle;

        // For "SORT-STRING" in vCard 3.0.
        private String mSortString;

        /**
         * Not in vCard but for {@link StructuredName#DISPLAY_NAME}. This field
         * is constructed by VCardEntry on demand. Consider using
         * {@link VCardEntry#getDisplayName()}.
         */
        // This field should reflect the other Elem fields like Email,
        // PostalAddress, etc., while
        // This is static class which cannot see other data. Thus we ask
        // VCardEntry to populate it.
        public String displayName;

        public boolean emptyStructuredName() {
            return TextUtils.isEmpty(mFamily) && TextUtils.isEmpty(mGiven)
                    && TextUtils.isEmpty(mMiddle) && TextUtils.isEmpty(mPrefix)
                    && TextUtils.isEmpty(mSuffix);
        }

        public boolean emptyPhoneticStructuredName() {
            return TextUtils.isEmpty(mPhoneticFamily) && TextUtils.isEmpty(mPhoneticGiven)
                    && TextUtils.isEmpty(mPhoneticMiddle);
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(StructuredName.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);

            if (!TextUtils.isEmpty(mGiven)) {
                builder.withValue(StructuredName.GIVEN_NAME, mGiven);
            }
            if (!TextUtils.isEmpty(mFamily)) {
                builder.withValue(StructuredName.FAMILY_NAME, mFamily);
            }
            if (!TextUtils.isEmpty(mMiddle)) {
                builder.withValue(StructuredName.MIDDLE_NAME, mMiddle);
            }
            if (!TextUtils.isEmpty(mPrefix)) {
                builder.withValue(StructuredName.PREFIX, mPrefix);
            }
            if (!TextUtils.isEmpty(mSuffix)) {
                builder.withValue(StructuredName.SUFFIX, mSuffix);
            }

            boolean phoneticNameSpecified = false;

            if (!TextUtils.isEmpty(mPhoneticGiven)) {
                builder.withValue(StructuredName.PHONETIC_GIVEN_NAME, mPhoneticGiven);
                phoneticNameSpecified = true;
            }
            if (!TextUtils.isEmpty(mPhoneticFamily)) {
                builder.withValue(StructuredName.PHONETIC_FAMILY_NAME, mPhoneticFamily);
                phoneticNameSpecified = true;
            }
            if (!TextUtils.isEmpty(mPhoneticMiddle)) {
                builder.withValue(StructuredName.PHONETIC_MIDDLE_NAME, mPhoneticMiddle);
                phoneticNameSpecified = true;
            }

            // SORT-STRING is used only when phonetic names aren't specified in
            // the original vCard.
            if (!phoneticNameSpecified) {
                builder.withValue(StructuredName.PHONETIC_GIVEN_NAME, mSortString);
            }

            builder.withValue(StructuredName.DISPLAY_NAME, displayName);
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return (TextUtils.isEmpty(mFamily) && TextUtils.isEmpty(mMiddle)
                    && TextUtils.isEmpty(mGiven) && TextUtils.isEmpty(mPrefix)
                    && TextUtils.isEmpty(mSuffix) && TextUtils.isEmpty(mFormatted)
                    && TextUtils.isEmpty(mPhoneticFamily) && TextUtils.isEmpty(mPhoneticMiddle)
                    && TextUtils.isEmpty(mPhoneticGiven) && TextUtils.isEmpty(mSortString));
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof NameData)) {
                return false;
            }
            NameData nameData = (NameData) obj;

            return (TextUtils.equals(mFamily, nameData.mFamily)
                    && TextUtils.equals(mMiddle, nameData.mMiddle)
                    && TextUtils.equals(mGiven, nameData.mGiven)
                    && TextUtils.equals(mPrefix, nameData.mPrefix)
                    && TextUtils.equals(mSuffix, nameData.mSuffix)
                    && TextUtils.equals(mFormatted, nameData.mFormatted)
                    && TextUtils.equals(mPhoneticFamily, nameData.mPhoneticFamily)
                    && TextUtils.equals(mPhoneticMiddle, nameData.mPhoneticMiddle)
                    && TextUtils.equals(mPhoneticGiven, nameData.mPhoneticGiven)
                    && TextUtils.equals(mSortString, nameData.mSortString));
        }

        @Override
        public int hashCode() {
            final String[] hashTargets = new String[] {mFamily, mMiddle, mGiven, mPrefix, mSuffix,
                    mFormatted, mPhoneticFamily, mPhoneticMiddle,
                    mPhoneticGiven, mSortString};
            int hash = 0;
            for (String hashTarget : hashTargets) {
                hash = hash * 31 + (hashTarget != null ? hashTarget.hashCode() : 0);
            }
            return hash;
        }

        @Override
        public String toString() {
            return String.format("family: %s, given: %s, middle: %s, prefix: %s, suffix: %s",
                    mFamily, mGiven, mMiddle, mPrefix, mSuffix);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.NAME;
        }

        public String getFamily() {
            return mFamily;
        }

        public String getMiddle() {
            return mMiddle;
        }

        public String getGiven() {
            return mGiven;
        }

        public String getPrefix() {
            return mPrefix;
        }

        public String getSuffix() {
            return mSuffix;
        }

        public String getFormatted() {
            return mFormatted;
        }

        public String getSortString() {
            return mSortString;
        }

        /** @hide Just for testing. */
        public void setFamily(String family) { mFamily = family; }
        /** @hide Just for testing. */
        public void setMiddle(String middle) { mMiddle = middle; }
        /** @hide Just for testing. */
        public void setGiven(String given) { mGiven = given; }
        /** @hide Just for testing. */
        public void setPrefix(String prefix) { mPrefix = prefix; }
        /** @hide Just for testing. */
        public void setSuffix(String suffix) { mSuffix = suffix; }
    }

    public static class PhoneData implements EntryElement {
        private final String mNumber;
        private final int mType;
        private final String mLabel;

        // isPrimary is (not final but) changable, only when there's no
        // appropriate one existing
        // in the original VCard.
        private boolean mIsPrimary;

        public PhoneData(String data, int type, String label, boolean isPrimary) {
            mNumber = data;
            mType = type;
            mLabel = label;
            mIsPrimary = isPrimary;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Phone.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);

            builder.withValue(Phone.TYPE, mType);
            if (mType == Phone.TYPE_CUSTOM) {
                builder.withValue(Phone.LABEL, mLabel);
            }
            builder.withValue(Phone.NUMBER, mNumber);
            if (mIsPrimary) {
                builder.withValue(Phone.IS_PRIMARY, 1);
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mNumber);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof PhoneData)) {
                return false;
            }
            PhoneData phoneData = (PhoneData) obj;
            return (mType == phoneData.mType
                    && TextUtils.equals(mNumber, phoneData.mNumber)
                    && TextUtils.equals(mLabel, phoneData.mLabel)
                    && (mIsPrimary == phoneData.mIsPrimary));
        }

        @Override
        public int hashCode() {
            int hash = mType;
            hash = hash * 31 + (mNumber != null ? mNumber.hashCode() : 0);
            hash = hash * 31 + (mLabel != null ? mLabel.hashCode() : 0);
            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);
            return hash;
        }

        @Override
        public String toString() {
            return String.format("type: %d, data: %s, label: %s, isPrimary: %s", mType, mNumber,
                    mLabel, mIsPrimary);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.PHONE;
        }

        public String getNumber() {
            return mNumber;
        }

        public int getType() {
            return mType;
        }

        public String getLabel() {
            return mLabel;
        }

        public boolean isPrimary() {
            return mIsPrimary;
        }
    }

    public static class EmailData implements EntryElement {
        private final String mAddress;
        private final int mType;
        // Used only when TYPE is TYPE_CUSTOM.
        private final String mLabel;
        private final boolean mIsPrimary;

        public EmailData(String data, int type, String label, boolean isPrimary) {
            mType = type;
            mAddress = data;
            mLabel = label;
            mIsPrimary = isPrimary;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Email.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Email.CONTENT_ITEM_TYPE);

            builder.withValue(Email.TYPE, mType);
            if (mType == Email.TYPE_CUSTOM) {
                builder.withValue(Email.LABEL, mLabel);
            }
            builder.withValue(Email.DATA, mAddress);
            if (mIsPrimary) {
                builder.withValue(Data.IS_PRIMARY, 1);
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mAddress);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof EmailData)) {
                return false;
            }
            EmailData emailData = (EmailData) obj;
            return (mType == emailData.mType
                    && TextUtils.equals(mAddress, emailData.mAddress)
                    && TextUtils.equals(mLabel, emailData.mLabel)
                    && (mIsPrimary == emailData.mIsPrimary));
        }

        @Override
        public int hashCode() {
            int hash = mType;
            hash = hash * 31 + (mAddress != null ? mAddress.hashCode() : 0);
            hash = hash * 31 + (mLabel != null ? mLabel.hashCode() : 0);
            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);
            return hash;
        }

        @Override
        public String toString() {
            return String.format("type: %d, data: %s, label: %s, isPrimary: %s", mType, mAddress,
                    mLabel, mIsPrimary);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.EMAIL;
        }

        public String getAddress() {
            return mAddress;
        }

        public int getType() {
            return mType;
        }

        public String getLabel() {
            return mLabel;
        }

        public boolean isPrimary() {
            return mIsPrimary;
        }
    }

    public static class PostalData implements EntryElement {
        // Determined by vCard specification.
        // - PO Box, Extended Addr, Street, Locality, Region, Postal Code, Country Name
        private static final int ADDR_MAX_DATA_SIZE = 7;
        private final String mPobox;
        private final String mExtendedAddress;
        private final String mStreet;
        private final String mLocalty;
        private final String mRegion;
        private final String mPostalCode;
        private final String mCountry;
        private final int mType;
        private final String mLabel;
        private boolean mIsPrimary;

        /** We keep this for {@link StructuredPostal#FORMATTED_ADDRESS} */
        // TODO: need better way to construct formatted address.
        private int mVCardType;

        public PostalData(String pobox, String extendedAddress, String street, String localty,
                String region, String postalCode, String country, int type, String label,
                boolean isPrimary, int vcardType) {
            mType = type;
            mPobox = pobox;
            mExtendedAddress = extendedAddress;
            mStreet = street;
            mLocalty = localty;
            mRegion = region;
            mPostalCode = postalCode;
            mCountry = country;
            mLabel = label;
            mIsPrimary = isPrimary;
            mVCardType = vcardType;
        }

        /**
         * Accepts raw propertyValueList in vCard and constructs PostalData.
         */
        public static PostalData constructPostalData(final List<String> propValueList,
                final int type, final String label, boolean isPrimary, int vcardType) {
            final String[] dataArray = new String[ADDR_MAX_DATA_SIZE];

            int size = propValueList.size();
            if (size > ADDR_MAX_DATA_SIZE) {
                size = ADDR_MAX_DATA_SIZE;
            }

            // adr-value = 0*6(text-value ";") text-value
            // ; PO Box, Extended Address, Street, Locality, Region, Postal Code, Country Name
            //
            // Use Iterator assuming List may be LinkedList, though actually it is
            // always ArrayList in the current implementation.
            int i = 0;
            for (String addressElement : propValueList) {
                dataArray[i] = addressElement;
                if (++i >= size) {
                    break;
                }
            }
            while (i < ADDR_MAX_DATA_SIZE) {
                dataArray[i++] = null;
            }

            return new PostalData(dataArray[0], dataArray[1], dataArray[2], dataArray[3],
                    dataArray[4], dataArray[5], dataArray[6], type, label, isPrimary, vcardType);
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(StructuredPostal.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, StructuredPostal.CONTENT_ITEM_TYPE);

            builder.withValue(StructuredPostal.TYPE, mType);
            if (mType == StructuredPostal.TYPE_CUSTOM) {
                builder.withValue(StructuredPostal.LABEL, mLabel);
            }

            final String streetString;
            if (TextUtils.isEmpty(mStreet)) {
                if (TextUtils.isEmpty(mExtendedAddress)) {
                    streetString = null;
                } else {
                    streetString = mExtendedAddress;
                }
            } else {
                if (TextUtils.isEmpty(mExtendedAddress)) {
                    streetString = mStreet;
                } else {
                    streetString = mStreet + " " + mExtendedAddress;
                }
            }
            builder.withValue(StructuredPostal.POBOX, mPobox);
            builder.withValue(StructuredPostal.STREET, streetString);
            builder.withValue(StructuredPostal.CITY, mLocalty);
            builder.withValue(StructuredPostal.REGION, mRegion);
            builder.withValue(StructuredPostal.POSTCODE, mPostalCode);
            builder.withValue(StructuredPostal.COUNTRY, mCountry);

            builder.withValue(StructuredPostal.FORMATTED_ADDRESS, getFormattedAddress(mVCardType));
            if (mIsPrimary) {
                builder.withValue(Data.IS_PRIMARY, 1);
            }
            operationList.add(builder.build());
        }

        public String getFormattedAddress(final int vcardType) {
            StringBuilder builder = new StringBuilder();
            boolean empty = true;
            final String[] dataArray = new String[] {
                    mPobox, mExtendedAddress, mStreet, mLocalty, mRegion, mPostalCode, mCountry
            };
            if (VCardConfig.isJapaneseDevice(vcardType)) {
                // In Japan, the order is reversed.
                for (int i = ADDR_MAX_DATA_SIZE - 1; i >= 0; i--) {
                    String addressPart = dataArray[i];
                    if (!TextUtils.isEmpty(addressPart)) {
                        if (!empty) {
                            builder.append(' ');
                        } else {
                            empty = false;
                        }
                        builder.append(addressPart);
                    }
                }
            } else {
                for (int i = 0; i < ADDR_MAX_DATA_SIZE; i++) {
                    String addressPart = dataArray[i];
                    if (!TextUtils.isEmpty(addressPart)) {
                        if (!empty) {
                            builder.append(' ');
                        } else {
                            empty = false;
                        }
                        builder.append(addressPart);
                    }
                }
            }

            return builder.toString().trim();
        }

        @Override
        public boolean isEmpty() {
            return (TextUtils.isEmpty(mPobox)
                    && TextUtils.isEmpty(mExtendedAddress)
                    && TextUtils.isEmpty(mStreet)
                    && TextUtils.isEmpty(mLocalty)
                    && TextUtils.isEmpty(mRegion)
                    && TextUtils.isEmpty(mPostalCode)
                    && TextUtils.isEmpty(mCountry));
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof PostalData)) {
                return false;
            }
            final PostalData postalData = (PostalData) obj;
            return (mType == postalData.mType)
                    && (mType == StructuredPostal.TYPE_CUSTOM ? TextUtils.equals(mLabel,
                            postalData.mLabel) : true)
                    && (mIsPrimary == postalData.mIsPrimary)
                    && TextUtils.equals(mPobox, postalData.mPobox)
                    && TextUtils.equals(mExtendedAddress, postalData.mExtendedAddress)
                    && TextUtils.equals(mStreet, postalData.mStreet)
                    && TextUtils.equals(mLocalty, postalData.mLocalty)
                    && TextUtils.equals(mRegion, postalData.mRegion)
                    && TextUtils.equals(mPostalCode, postalData.mPostalCode)
                    && TextUtils.equals(mCountry, postalData.mCountry);
        }

        @Override
        public int hashCode() {
            int hash = mType;
            hash = hash * 31 + (mLabel != null ? mLabel.hashCode() : 0);
            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);

            final String[] hashTargets = new String[] {mPobox, mExtendedAddress, mStreet,
                    mLocalty, mRegion, mPostalCode, mCountry};
            for (String hashTarget : hashTargets) {
                hash = hash * 31 + (hashTarget != null ? hashTarget.hashCode() : 0);
            }
            return hash;
        }

        @Override
        public String toString() {
            return String.format("type: %d, label: %s, isPrimary: %s, pobox: %s, "
                    + "extendedAddress: %s, street: %s, localty: %s, region: %s, postalCode %s, "
                    + "country: %s", mType, mLabel, mIsPrimary, mPobox, mExtendedAddress, mStreet,
                    mLocalty, mRegion, mPostalCode, mCountry);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.POSTAL_ADDRESS;
        }

        public String getPobox() {
            return mPobox;
        }

        public String getExtendedAddress() {
            return mExtendedAddress;
        }

        public String getStreet() {
            return mStreet;
        }

        public String getLocalty() {
            return mLocalty;
        }

        public String getRegion() {
            return mRegion;
        }

        public String getPostalCode() {
            return mPostalCode;
        }

        public String getCountry() {
            return mCountry;
        }

        public int getType() {
            return mType;
        }

        public String getLabel() {
            return mLabel;
        }

        public boolean isPrimary() {
            return mIsPrimary;
        }
    }

    public static class OrganizationData implements EntryElement {
        // non-final is Intentional: we may change the values since this info is separated into
        // two parts in vCard: "ORG" + "TITLE", and we have to cope with each field in different
        // timing.
        private String mOrganizationName;
        private String mDepartmentName;
        private String mTitle;
        private final String mPhoneticName; // We won't have this in "TITLE" property.
        private final int mType;
        private boolean mIsPrimary;

        public OrganizationData(final String organizationName, final String departmentName,
                final String titleName, final String phoneticName, int type,
                final boolean isPrimary) {
            mType = type;
            mOrganizationName = organizationName;
            mDepartmentName = departmentName;
            mTitle = titleName;
            mPhoneticName = phoneticName;
            mIsPrimary = isPrimary;
        }

        public String getFormattedString() {
            final StringBuilder builder = new StringBuilder();
            if (!TextUtils.isEmpty(mOrganizationName)) {
                builder.append(mOrganizationName);
            }

            if (!TextUtils.isEmpty(mDepartmentName)) {
                if (builder.length() > 0) {
                    builder.append(", ");
                }
                builder.append(mDepartmentName);
            }

            if (!TextUtils.isEmpty(mTitle)) {
                if (builder.length() > 0) {
                    builder.append(", ");
                }
                builder.append(mTitle);
            }

            return builder.toString();
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Organization.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Organization.CONTENT_ITEM_TYPE);
            builder.withValue(Organization.TYPE, mType);
            if (mOrganizationName != null) {
                builder.withValue(Organization.COMPANY, mOrganizationName);
            }
            if (mDepartmentName != null) {
                builder.withValue(Organization.DEPARTMENT, mDepartmentName);
            }
            if (mTitle != null) {
                builder.withValue(Organization.TITLE, mTitle);
            }
            if (mPhoneticName != null) {
                builder.withValue(Organization.PHONETIC_NAME, mPhoneticName);
            }
            if (mIsPrimary) {
                builder.withValue(Organization.IS_PRIMARY, 1);
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mOrganizationName) && TextUtils.isEmpty(mDepartmentName)
                    && TextUtils.isEmpty(mTitle) && TextUtils.isEmpty(mPhoneticName);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof OrganizationData)) {
                return false;
            }
            OrganizationData organization = (OrganizationData) obj;
            return (mType == organization.mType
                    && TextUtils.equals(mOrganizationName, organization.mOrganizationName)
                    && TextUtils.equals(mDepartmentName, organization.mDepartmentName)
                    && TextUtils.equals(mTitle, organization.mTitle)
                    && (mIsPrimary == organization.mIsPrimary));
        }

        @Override
        public int hashCode() {
            int hash = mType;
            hash = hash * 31 + (mOrganizationName != null ? mOrganizationName.hashCode() : 0);
            hash = hash * 31 + (mDepartmentName != null ? mDepartmentName.hashCode() : 0);
            hash = hash * 31 + (mTitle != null ? mTitle.hashCode() : 0);
            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);
            return hash;
        }

        @Override
        public String toString() {
            return String.format(
                    "type: %d, organization: %s, department: %s, title: %s, isPrimary: %s", mType,
                    mOrganizationName, mDepartmentName, mTitle, mIsPrimary);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.ORGANIZATION;
        }

        public String getOrganizationName() {
            return mOrganizationName;
        }

        public String getDepartmentName() {
            return mDepartmentName;
        }

        public String getTitle() {
            return mTitle;
        }

        public String getPhoneticName() {
            return mPhoneticName;
        }

        public int getType() {
            return mType;
        }

        public boolean isPrimary() {
            return mIsPrimary;
        }
    }

    public static class ImData implements EntryElement {
        private final String mAddress;
        private final int mProtocol;
        private final String mCustomProtocol;
        private final int mType;
        private final boolean mIsPrimary;

        public ImData(final int protocol, final String customProtocol, final String address,
                final int type, final boolean isPrimary) {
            mProtocol = protocol;
            mCustomProtocol = customProtocol;
            mType = type;
            mAddress = address;
            mIsPrimary = isPrimary;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Im.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Im.CONTENT_ITEM_TYPE);
            builder.withValue(Im.TYPE, mType);
            builder.withValue(Im.PROTOCOL, mProtocol);
            builder.withValue(Im.DATA, mAddress);
            if (mProtocol == Im.PROTOCOL_CUSTOM) {
                builder.withValue(Im.CUSTOM_PROTOCOL, mCustomProtocol);
            }
            if (mIsPrimary) {
                builder.withValue(Data.IS_PRIMARY, 1);
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mAddress);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof ImData)) {
                return false;
            }
            ImData imData = (ImData) obj;
            return (mType == imData.mType
                    && mProtocol == imData.mProtocol
                    && TextUtils.equals(mCustomProtocol, imData.mCustomProtocol)
                    && TextUtils.equals(mAddress, imData.mAddress)
                    && (mIsPrimary == imData.mIsPrimary));
        }

        @Override
        public int hashCode() {
            int hash = mType;
            hash = hash * 31 + mProtocol;
            hash = hash * 31 + (mCustomProtocol != null ? mCustomProtocol.hashCode() : 0);
            hash = hash * 31 + (mAddress != null ? mAddress.hashCode() : 0);
            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);
            return hash;
        }

        @Override
        public String toString() {
            return String.format(
                    "type: %d, protocol: %d, custom_protcol: %s, data: %s, isPrimary: %s", mType,
                    mProtocol, mCustomProtocol, mAddress, mIsPrimary);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.IM;
        }

        public String getAddress() {
            return mAddress;
        }

        /**
         * One of the value available for {@link Im#PROTOCOL}. e.g.
         * {@link Im#PROTOCOL_GOOGLE_TALK}
         */
        public int getProtocol() {
            return mProtocol;
        }

        public String getCustomProtocol() {
            return mCustomProtocol;
        }

        public int getType() {
            return mType;
        }

        public boolean isPrimary() {
            return mIsPrimary;
        }
    }

    public static class PhotoData implements EntryElement {
        // private static final String FORMAT_FLASH = "SWF";

        // used when type is not defined in ContactsContract.
        private final String mFormat;
        private final boolean mIsPrimary;

        private final byte[] mBytes;

        private Integer mHashCode = null;

        public PhotoData(String format, byte[] photoBytes, boolean isPrimary) {
            mFormat = format;
            mBytes = photoBytes;
            mIsPrimary = isPrimary;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Photo.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Photo.CONTENT_ITEM_TYPE);
            builder.withValue(Photo.PHOTO, mBytes);
            if (mIsPrimary) {
                builder.withValue(Photo.IS_PRIMARY, 1);
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return mBytes == null || mBytes.length == 0;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof PhotoData)) {
                return false;
            }
            PhotoData photoData = (PhotoData) obj;
            return (TextUtils.equals(mFormat, photoData.mFormat)
                    && Arrays.equals(mBytes, photoData.mBytes)
                    && (mIsPrimary == photoData.mIsPrimary));
        }

        @Override
        public int hashCode() {
            if (mHashCode != null) {
                return mHashCode;
            }

            int hash = mFormat != null ? mFormat.hashCode() : 0;
            hash = hash * 31;
            if (mBytes != null) {
                for (byte b : mBytes) {
                    hash += b;
                }
            }

            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);
            mHashCode = hash;
            return hash;
        }

        @Override
        public String toString() {
            return String.format("format: %s: size: %d, isPrimary: %s", mFormat, mBytes.length,
                    mIsPrimary);
        }

        @Override
        public final EntryLabel getEntryLabel() {
            return EntryLabel.PHOTO;
        }

        public String getFormat() {
            return mFormat;
        }

        public byte[] getBytes() {
            return mBytes;
        }

        public boolean isPrimary() {
            return mIsPrimary;
        }
    }

    public static class NicknameData implements EntryElement {
        private final String mNickname;

        public NicknameData(String nickname) {
            mNickname = nickname;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Nickname.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Nickname.CONTENT_ITEM_TYPE);
            builder.withValue(Nickname.TYPE, Nickname.TYPE_DEFAULT);
            builder.withValue(Nickname.NAME, mNickname);
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mNickname);
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof NicknameData)) {
                return false;
            }
            NicknameData nicknameData = (NicknameData) obj;
            return TextUtils.equals(mNickname, nicknameData.mNickname);
        }

        @Override
        public int hashCode() {
            return mNickname != null ? mNickname.hashCode() : 0;
        }

        @Override
        public String toString() {
            return "nickname: " + mNickname;
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.NICKNAME;
        }

        public String getNickname() {
            return mNickname;
        }
    }

    public static class NoteData implements EntryElement {
        public final String mNote;

        public NoteData(String note) {
            mNote = note;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Note.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Note.CONTENT_ITEM_TYPE);
            builder.withValue(Note.NOTE, mNote);
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mNote);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof NoteData)) {
                return false;
            }
            NoteData noteData = (NoteData) obj;
            return TextUtils.equals(mNote, noteData.mNote);
        }

        @Override
        public int hashCode() {
            return mNote != null ? mNote.hashCode() : 0;
        }

        @Override
        public String toString() {
            return "note: " + mNote;
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.NOTE;
        }

        public String getNote() {
            return mNote;
        }
    }

    public static class WebsiteData implements EntryElement {
        private final String mWebsite;

        public WebsiteData(String website) {
            mWebsite = website;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Website.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Website.CONTENT_ITEM_TYPE);
            builder.withValue(Website.URL, mWebsite);
            // There's no information about the type of URL in vCard.
            // We use TYPE_HOMEPAGE for safety.
            builder.withValue(Website.TYPE, Website.TYPE_HOMEPAGE);
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mWebsite);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof WebsiteData)) {
                return false;
            }
            WebsiteData websiteData = (WebsiteData) obj;
            return TextUtils.equals(mWebsite, websiteData.mWebsite);
        }

        @Override
        public int hashCode() {
            return mWebsite != null ? mWebsite.hashCode() : 0;
        }

        @Override
        public String toString() {
            return "website: " + mWebsite;
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.WEBSITE;
        }

        public String getWebsite() {
            return mWebsite;
        }
    }

    public static class BirthdayData implements EntryElement {
        private final String mBirthday;

        public BirthdayData(String birthday) {
            mBirthday = birthday;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Event.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Event.CONTENT_ITEM_TYPE);
            builder.withValue(Event.START_DATE, mBirthday);
            builder.withValue(Event.TYPE, Event.TYPE_BIRTHDAY);
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mBirthday);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof BirthdayData)) {
                return false;
            }
            BirthdayData birthdayData = (BirthdayData) obj;
            return TextUtils.equals(mBirthday, birthdayData.mBirthday);
        }

        @Override
        public int hashCode() {
            return mBirthday != null ? mBirthday.hashCode() : 0;
        }

        @Override
        public String toString() {
            return "birthday: " + mBirthday;
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.BIRTHDAY;
        }

        public String getBirthday() {
            return mBirthday;
        }
    }

    public static class AnniversaryData implements EntryElement {
        private final String mAnniversary;

        public AnniversaryData(String anniversary) {
            mAnniversary = anniversary;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(Event.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, Event.CONTENT_ITEM_TYPE);
            builder.withValue(Event.START_DATE, mAnniversary);
            builder.withValue(Event.TYPE, Event.TYPE_ANNIVERSARY);
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mAnniversary);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof AnniversaryData)) {
                return false;
            }
            AnniversaryData anniversaryData = (AnniversaryData) obj;
            return TextUtils.equals(mAnniversary, anniversaryData.mAnniversary);
        }

        @Override
        public int hashCode() {
            return mAnniversary != null ? mAnniversary.hashCode() : 0;
        }

        @Override
        public String toString() {
            return "anniversary: " + mAnniversary;
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.ANNIVERSARY;
        }

        public String getAnniversary() { return mAnniversary; }
    }

    public static class SipData implements EntryElement {
        /**
         * Note that schema part ("sip:") is automatically removed. e.g.
         * "sip:username:password@host:port" becomes
         * "username:password@host:port"
         */
        private final String mAddress;
        private final int mType;
        private final String mLabel;
        private final boolean mIsPrimary;

        public SipData(String rawSip, int type, String label, boolean isPrimary) {
            if (rawSip.startsWith("sip:")) {
                mAddress = rawSip.substring(4);
            } else {
                mAddress = rawSip;
            }
            mType = type;
            mLabel = label;
            mIsPrimary = isPrimary;
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(SipAddress.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, SipAddress.CONTENT_ITEM_TYPE);
            builder.withValue(SipAddress.SIP_ADDRESS, mAddress);
            builder.withValue(SipAddress.TYPE, mType);
            if (mType == SipAddress.TYPE_CUSTOM) {
                builder.withValue(SipAddress.LABEL, mLabel);
            }
            if (mIsPrimary) {
                builder.withValue(SipAddress.IS_PRIMARY, mIsPrimary);
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mAddress);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof SipData)) {
                return false;
            }
            SipData sipData = (SipData) obj;
            return (mType == sipData.mType
                    && TextUtils.equals(mLabel, sipData.mLabel)
                    && TextUtils.equals(mAddress, sipData.mAddress)
                    && (mIsPrimary == sipData.mIsPrimary));
        }

        @Override
        public int hashCode() {
            int hash = mType;
            hash = hash * 31 + (mLabel != null ? mLabel.hashCode() : 0);
            hash = hash * 31 + (mAddress != null ? mAddress.hashCode() : 0);
            hash = hash * 31 + (mIsPrimary ? 1231 : 1237);
            return hash;
        }

        @Override
        public String toString() {
            return "sip: " + mAddress;
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.SIP;
        }

        /**
         * @return Address part of the sip data. The schema ("sip:") isn't contained here.
         */
        public String getAddress() { return mAddress; }
        public int getType() { return mType; }
        public String getLabel() { return mLabel; }
    }

    /**
     * Some Contacts data in Android cannot be converted to vCard
     * representation. VCardEntry preserves those data using this class.
     */
    public static class AndroidCustomData implements EntryElement {
        private final String mMimeType;

        private final List<String> mDataList; // 1 .. VCardConstants.MAX_DATA_COLUMN

        public AndroidCustomData(String mimeType, List<String> dataList) {
            mMimeType = mimeType;
            mDataList = dataList;
        }

        public static AndroidCustomData constructAndroidCustomData(List<String> list) {
            String mimeType;
            List<String> dataList;

            if (list == null) {
                mimeType = null;
                dataList = null;
            } else if (list.size() < 2) {
                mimeType = list.get(0);
                dataList = null;
            } else {
                final int max = (list.size() < VCardConstants.MAX_DATA_COLUMN + 1) ? list.size()
                        : VCardConstants.MAX_DATA_COLUMN + 1;
                mimeType = list.get(0);
                dataList = list.subList(1, max);
            }

            return new AndroidCustomData(mimeType, dataList);
        }

        @Override
        public void constructInsertOperation(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            final ContentProviderOperation.Builder builder = ContentProviderOperation
                    .newInsert(Data.CONTENT_URI);
            builder.withValueBackReference(GroupMembership.RAW_CONTACT_ID, backReferenceIndex);
            builder.withValue(Data.MIMETYPE, mMimeType);
            for (int i = 0; i < mDataList.size(); i++) {
                String value = mDataList.get(i);
                if (!TextUtils.isEmpty(value)) {
                    // 1-origin
                    builder.withValue("data" + (i + 1), value);
                }
            }
            operationList.add(builder.build());
        }

        @Override
        public boolean isEmpty() {
            return TextUtils.isEmpty(mMimeType) || mDataList == null || mDataList.size() == 0;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof AndroidCustomData)) {
                return false;
            }
            AndroidCustomData data = (AndroidCustomData) obj;
            if (!TextUtils.equals(mMimeType, data.mMimeType)) {
                return false;
            }
            if (mDataList == null) {
                return data.mDataList == null;
            } else {
                final int size = mDataList.size();
                if (size != data.mDataList.size()) {
                    return false;
                }
                for (int i = 0; i < size; i++) {
                    if (!TextUtils.equals(mDataList.get(i), data.mDataList.get(i))) {
                        return false;
                    }
                }
                return true;
            }
        }

        @Override
        public int hashCode() {
            int hash = mMimeType != null ? mMimeType.hashCode() : 0;
            if (mDataList != null) {
                for (String data : mDataList) {
                    hash = hash * 31 + (data != null ? data.hashCode() : 0);
                }
            }
            return hash;
        }

        @Override
        public String toString() {
            final StringBuilder builder = new StringBuilder();
            builder.append("android-custom: " + mMimeType + ", data: ");
            builder.append(mDataList == null ? "null" : Arrays.toString(mDataList.toArray()));
            return builder.toString();
        }

        @Override
        public EntryLabel getEntryLabel() {
            return EntryLabel.ANDROID_CUSTOM;
        }

        public String getMimeType() { return mMimeType; }
        public List<String> getDataList() { return mDataList; }
    }

    private final NameData mNameData = new NameData();
    private List<PhoneData> mPhoneList;
    private List<EmailData> mEmailList;
    private List<PostalData> mPostalList;
    private List<OrganizationData> mOrganizationList;
    private List<ImData> mImList;
    private List<PhotoData> mPhotoList;
    private List<WebsiteData> mWebsiteList;
    private List<SipData> mSipList;
    private List<NicknameData> mNicknameList;
    private List<NoteData> mNoteList;
    private List<AndroidCustomData> mAndroidCustomDataList;
    private BirthdayData mBirthday;
    private AnniversaryData mAnniversary;
    private List<Pair<String, String>> mUnknownXData;

    /**
     * Inner iterator interface.
     */
    public interface EntryElementIterator {
        public void onIterationStarted();

        public void onIterationEnded();

        /**
         * Called when there are one or more {@link EntryElement} instances
         * associated with {@link EntryLabel}.
         */
        public void onElementGroupStarted(EntryLabel label);

        /**
         * Called after all {@link EntryElement} instances for
         * {@link EntryLabel} provided on {@link #onElementGroupStarted(EntryLabel)}
         * being processed by {@link #onElement(EntryElement)}
         */
        public void onElementGroupEnded();

        /**
         * @return should be true when child wants to continue the operation.
         *         False otherwise.
         */
        public boolean onElement(EntryElement elem);
    }

    public final void iterateAllData(EntryElementIterator iterator) {
        iterator.onIterationStarted();
        iterator.onElementGroupStarted(mNameData.getEntryLabel());
        iterator.onElement(mNameData);
        iterator.onElementGroupEnded();

        iterateOneList(mPhoneList, iterator);
        iterateOneList(mEmailList, iterator);
        iterateOneList(mPostalList, iterator);
        iterateOneList(mOrganizationList, iterator);
        iterateOneList(mImList, iterator);
        iterateOneList(mPhotoList, iterator);
        iterateOneList(mWebsiteList, iterator);
        iterateOneList(mSipList, iterator);
        iterateOneList(mNicknameList, iterator);
        iterateOneList(mNoteList, iterator);
        iterateOneList(mAndroidCustomDataList, iterator);

        if (mBirthday != null) {
            iterator.onElementGroupStarted(mBirthday.getEntryLabel());
            iterator.onElement(mBirthday);
            iterator.onElementGroupEnded();
        }
        if (mAnniversary != null) {
            iterator.onElementGroupStarted(mAnniversary.getEntryLabel());
            iterator.onElement(mAnniversary);
            iterator.onElementGroupEnded();
        }
        iterator.onIterationEnded();
    }

    private void iterateOneList(List<? extends EntryElement> elemList,
            EntryElementIterator iterator) {
        if (elemList != null && elemList.size() > 0) {
            iterator.onElementGroupStarted(elemList.get(0).getEntryLabel());
            for (EntryElement elem : elemList) {
                iterator.onElement(elem);
            }
            iterator.onElementGroupEnded();
        }
    }

    private class IsIgnorableIterator implements EntryElementIterator {
        private boolean mEmpty = true;

        @Override
        public void onIterationStarted() {
        }

        @Override
        public void onIterationEnded() {
        }

        @Override
        public void onElementGroupStarted(EntryLabel label) {
        }

        @Override
        public void onElementGroupEnded() {
        }

        @Override
        public boolean onElement(EntryElement elem) {
            if (!elem.isEmpty()) {
                mEmpty = false;
                // exit now
                return false;
            } else {
                return true;
            }
        }

        public boolean getResult() {
            return mEmpty;
        }
    }

    private class ToStringIterator implements EntryElementIterator {
        private StringBuilder mBuilder;

        private boolean mFirstElement;

        @Override
        public void onIterationStarted() {
            mBuilder = new StringBuilder();
            mBuilder.append("[[hash: " + VCardEntry.this.hashCode() + "\n");
        }

        @Override
        public void onElementGroupStarted(EntryLabel label) {
            mBuilder.append(label.toString() + ": ");
            mFirstElement = true;
        }

        @Override
        public boolean onElement(EntryElement elem) {
            if (!mFirstElement) {
                mBuilder.append(", ");
                mFirstElement = false;
            }
            mBuilder.append("[").append(elem.toString()).append("]");
            return true;
        }

        @Override
        public void onElementGroupEnded() {
            mBuilder.append("\n");
        }

        @Override
        public void onIterationEnded() {
            mBuilder.append("]]\n");
        }

        @Override
        public String toString() {
            return mBuilder.toString();
        }
    }

    private class InsertOperationConstrutor implements EntryElementIterator {
        private final List<ContentProviderOperation> mOperationList;

        private final int mBackReferenceIndex;

        public InsertOperationConstrutor(List<ContentProviderOperation> operationList,
                int backReferenceIndex) {
            mOperationList = operationList;
            mBackReferenceIndex = backReferenceIndex;
        }

        @Override
        public void onIterationStarted() {
        }

        @Override
        public void onIterationEnded() {
        }

        @Override
        public void onElementGroupStarted(EntryLabel label) {
        }

        @Override
        public void onElementGroupEnded() {
        }

        @Override
        public boolean onElement(EntryElement elem) {
            if (!elem.isEmpty()) {
                elem.constructInsertOperation(mOperationList, mBackReferenceIndex);
            }
            return true;
        }
    }

    private final int mVCardType;
    private final Account mAccount;

    private List<VCardEntry> mChildren;

    @Override
    public String toString() {
        ToStringIterator iterator = new ToStringIterator();
        iterateAllData(iterator);
        return iterator.toString();
    }

    public VCardEntry() {
        this(VCardConfig.VCARD_TYPE_V21_GENERIC);
    }

    public VCardEntry(int vcardType) {
        this(vcardType, null);
    }

    public VCardEntry(int vcardType, Account account) {
        mVCardType = vcardType;
        mAccount = account;
    }

    private void addPhone(int type, String data, String label, boolean isPrimary) {
        if (mPhoneList == null) {
            mPhoneList = new ArrayList<PhoneData>();
        }
        final StringBuilder builder = new StringBuilder();
        final String trimmed = data.trim();
        final String formattedNumber;
        if (type == Phone.TYPE_PAGER || VCardConfig.refrainPhoneNumberFormatting(mVCardType)) {
            formattedNumber = trimmed;
        } else {
            // TODO: from the view of vCard spec these auto conversions should be removed.
            // Note that some other codes (like the phone number formatter) or modules expect this
            // auto conversion (bug 5178723), so just omitting this code won't be preferable enough
            // (bug 4177894)
            boolean hasPauseOrWait = false;
            final int length = trimmed.length();
            for (int i = 0; i < length; i++) {
                char ch = trimmed.charAt(i);
                // See RFC 3601 and docs for PhoneNumberUtils for more info.
                if (ch == 'p' || ch == 'P') {
                    builder.append(PhoneNumberUtils.PAUSE);
                    hasPauseOrWait = true;
                } else if (ch == 'w' || ch == 'W') {
                    builder.append(PhoneNumberUtils.WAIT);
                    hasPauseOrWait = true;
                } else if (('0' <= ch && ch <= '9') || (i == 0 && ch == '+')) {
                    builder.append(ch);
                }
            }
            if (!hasPauseOrWait) {
                final int formattingType = VCardUtils.getPhoneNumberFormat(mVCardType);
                formattedNumber = PhoneNumberUtilsPort.formatNumber(
                        builder.toString(), formattingType);
            } else {
                formattedNumber = builder.toString();
            }
        }
        PhoneData phoneData = new PhoneData(formattedNumber, type, label, isPrimary);
        mPhoneList.add(phoneData);
    }

    private void addSip(String sipData, int type, String label, boolean isPrimary) {
        if (mSipList == null) {
            mSipList = new ArrayList<SipData>();
        }
        mSipList.add(new SipData(sipData, type, label, isPrimary));
    }

    private void addNickName(final String nickName) {
        if (mNicknameList == null) {
            mNicknameList = new ArrayList<NicknameData>();
        }
        mNicknameList.add(new NicknameData(nickName));
    }

    private void addEmail(int type, String data, String label, boolean isPrimary) {
        if (mEmailList == null) {
            mEmailList = new ArrayList<EmailData>();
        }
        mEmailList.add(new EmailData(data, type, label, isPrimary));
    }

    private void addPostal(int type, List<String> propValueList, String label, boolean isPrimary) {
        if (mPostalList == null) {
            mPostalList = new ArrayList<PostalData>(0);
        }
        mPostalList.add(PostalData.constructPostalData(propValueList, type, label, isPrimary,
                mVCardType));
    }

    /**
     * Should be called via {@link #handleOrgValue(int, List, Map, boolean)} or
     * {@link #handleTitleValue(String)}.
     */
    private void addNewOrganization(final String organizationName, final String departmentName,
            final String titleName, final String phoneticName, int type, final boolean isPrimary) {
        if (mOrganizationList == null) {
            mOrganizationList = new ArrayList<OrganizationData>();
        }
        mOrganizationList.add(new OrganizationData(organizationName, departmentName, titleName,
                phoneticName, type, isPrimary));
    }

    private static final List<String> sEmptyList = Collections
            .unmodifiableList(new ArrayList<String>(0));

    private String buildSinglePhoneticNameFromSortAsParam(Map<String, Collection<String>> paramMap) {
        final Collection<String> sortAsCollection = paramMap.get(VCardConstants.PARAM_SORT_AS);
        if (sortAsCollection != null && sortAsCollection.size() != 0) {
            if (sortAsCollection.size() > 1) {
                Log.w(LOG_TAG,
                        "Incorrect multiple SORT_AS parameters detected: "
                                + Arrays.toString(sortAsCollection.toArray()));
            }
            final List<String> sortNames = VCardUtils.constructListFromValue(sortAsCollection
                    .iterator().next(), mVCardType);
            final StringBuilder builder = new StringBuilder();
            for (final String elem : sortNames) {
                builder.append(elem);
            }
            return builder.toString();
        } else {
            return null;
        }
    }

    /**
     * Set "ORG" related values to the appropriate data. If there's more than
     * one {@link OrganizationData} objects, this input data are attached to the
     * last one which does not have valid values (not including empty but only
     * null). If there's no {@link OrganizationData} object, a new
     * {@link OrganizationData} is created, whose title is set to null.
     */
    private void handleOrgValue(final int type, List<String> orgList,
            Map<String, Collection<String>> paramMap, boolean isPrimary) {
        final String phoneticName = buildSinglePhoneticNameFromSortAsParam(paramMap);
        if (orgList == null) {
            orgList = sEmptyList;
        }
        final String organizationName;
        final String departmentName;
        final int size = orgList.size();
        switch (size) {
        case 0: {
            organizationName = "";
            departmentName = null;
            break;
        }
        case 1: {
            organizationName = orgList.get(0);
            departmentName = null;
            break;
        }
        default: { // More than 1.
            organizationName = orgList.get(0);
            // We're not sure which is the correct string for department.
            // In order to keep all the data, concatinate the rest of elements.
            StringBuilder builder = new StringBuilder();
            for (int i = 1; i < size; i++) {
                if (i > 1) {
                    builder.append(' ');
                }
                builder.append(orgList.get(i));
            }
            departmentName = builder.toString();
        }
        }
        if (mOrganizationList == null) {
            // Create new first organization entry, with "null" title which may be
            // added via handleTitleValue().
            addNewOrganization(organizationName, departmentName, null, phoneticName, type,
                    isPrimary);
            return;
        }
        for (OrganizationData organizationData : mOrganizationList) {
            // Not use TextUtils.isEmpty() since ORG was set but the elements might be empty.
            // e.g. "ORG;PREF:;" -> Both companyName and departmentName become empty but not null.
            if (organizationData.mOrganizationName == null
                    && organizationData.mDepartmentName == null) {
                // Probably the "TITLE" property comes before the "ORG" property via
                // handleTitleLine().
                organizationData.mOrganizationName = organizationName;
                organizationData.mDepartmentName = departmentName;
                organizationData.mIsPrimary = isPrimary;
                return;
            }
        }
        // No OrganizatioData is available. Create another one, with "null" title, which may be
        // added via handleTitleValue().
        addNewOrganization(organizationName, departmentName, null, phoneticName, type, isPrimary);
    }

    /**
     * Set "title" value to the appropriate data. If there's more than one
     * OrganizationData objects, this input is attached to the last one which
     * does not have valid title value (not including empty but only null). If
     * there's no OrganizationData object, a new OrganizationData is created,
     * whose company name is set to null.
     */
    private void handleTitleValue(final String title) {
        if (mOrganizationList == null) {
            // Create new first organization entry, with "null" other info, which may be
            // added via handleOrgValue().
            addNewOrganization(null, null, title, null, DEFAULT_ORGANIZATION_TYPE, false);
            return;
        }
        for (OrganizationData organizationData : mOrganizationList) {
            if (organizationData.mTitle == null) {
                organizationData.mTitle = title;
                return;
            }
        }
        // No Organization is available. Create another one, with "null" other info, which may be
        // added via handleOrgValue().
        addNewOrganization(null, null, title, null, DEFAULT_ORGANIZATION_TYPE, false);
    }

    private void addIm(int protocol, String customProtocol, String propValue, int type,
            boolean isPrimary) {
        if (mImList == null) {
            mImList = new ArrayList<ImData>();
        }
        mImList.add(new ImData(protocol, customProtocol, propValue, type, isPrimary));
    }

    private void addNote(final String note) {
        if (mNoteList == null) {
            mNoteList = new ArrayList<NoteData>(1);
        }
        mNoteList.add(new NoteData(note));
    }

    private void addPhotoBytes(String formatName, byte[] photoBytes, boolean isPrimary) {
        if (mPhotoList == null) {
            mPhotoList = new ArrayList<PhotoData>(1);
        }
        final PhotoData photoData = new PhotoData(formatName, photoBytes, isPrimary);
        mPhotoList.add(photoData);
    }

    /**
     * Tries to extract paramMap, constructs SORT-AS parameter values, and store
     * them in appropriate phonetic name variables. This method does not care
     * the vCard version. Even when we have SORT-AS parameters in invalid
     * versions (i.e. 2.1 and 3.0), we scilently accept them so that we won't
     * drop meaningful information. If we had this parameter in the N field of
     * vCard 3.0, and the contact data also have SORT-STRING, we will prefer
     * SORT-STRING, since it is regitimate property to be understood.
     */
    private void tryHandleSortAsName(final Map<String, Collection<String>> paramMap) {
        if (VCardConfig.isVersion30(mVCardType)
                && !(TextUtils.isEmpty(mNameData.mPhoneticFamily)
                        && TextUtils.isEmpty(mNameData.mPhoneticMiddle) && TextUtils
                        .isEmpty(mNameData.mPhoneticGiven))) {
            return;
        }

        final Collection<String> sortAsCollection = paramMap.get(VCardConstants.PARAM_SORT_AS);
        if (sortAsCollection != null && sortAsCollection.size() != 0) {
            if (sortAsCollection.size() > 1) {
                Log.w(LOG_TAG,
                        "Incorrect multiple SORT_AS parameters detected: "
                                + Arrays.toString(sortAsCollection.toArray()));
            }
            final List<String> sortNames = VCardUtils.constructListFromValue(sortAsCollection
                    .iterator().next(), mVCardType);
            int size = sortNames.size();
            if (size > 3) {
                size = 3;
            }
            switch (size) {
            case 3:
                mNameData.mPhoneticMiddle = sortNames.get(2); //$FALL-THROUGH$
            case 2:
                mNameData.mPhoneticGiven = sortNames.get(1); //$FALL-THROUGH$
            default:
                mNameData.mPhoneticFamily = sortNames.get(0);
                break;
            }
        }
    }

    @SuppressWarnings("fallthrough")
    private void handleNProperty(final List<String> paramValues,
            Map<String, Collection<String>> paramMap) {
        // in vCard 4.0, SORT-AS parameter is available.
        tryHandleSortAsName(paramMap);

        // Family, Given, Middle, Prefix, Suffix. (1 - 5)
        int size;
        if (paramValues == null || (size = paramValues.size()) < 1) {
            return;
        }
        if (size > 5) {
            size = 5;
        }

        switch (size) {
        // Fall-through.
        case 5:
            mNameData.mSuffix = paramValues.get(4);
        case 4:
            mNameData.mPrefix = paramValues.get(3);
        case 3:
            mNameData.mMiddle = paramValues.get(2);
        case 2:
            mNameData.mGiven = paramValues.get(1);
        default:
            mNameData.mFamily = paramValues.get(0);
        }
    }

    /**
     * Note: Some Japanese mobile phones use this field for phonetic name, since
     * vCard 2.1 does not have "SORT-STRING" type. Also, in some cases, the
     * field has some ';'s in it. Assume the ';' means the same meaning in N
     * property
     */
    @SuppressWarnings("fallthrough")
    private void handlePhoneticNameFromSound(List<String> elems) {
        if (!(TextUtils.isEmpty(mNameData.mPhoneticFamily)
                && TextUtils.isEmpty(mNameData.mPhoneticMiddle) && TextUtils
                .isEmpty(mNameData.mPhoneticGiven))) {
            // This means the other properties like "X-PHONETIC-FIRST-NAME" was already found.
            // Ignore "SOUND;X-IRMC-N".
            return;
        }

        int size;
        if (elems == null || (size = elems.size()) < 1) {
            return;
        }

        // Assume that the order is "Family, Given, Middle".
        // This is not from specification but mere assumption. Some Japanese
        // phones use this order.
        if (size > 3) {
            size = 3;
        }

        if (elems.get(0).length() > 0) {
            boolean onlyFirstElemIsNonEmpty = true;
            for (int i = 1; i < size; i++) {
                if (elems.get(i).length() > 0) {
                    onlyFirstElemIsNonEmpty = false;
                    break;
                }
            }
            if (onlyFirstElemIsNonEmpty) {
                final String[] namesArray = elems.get(0).split(" ");
                final int nameArrayLength = namesArray.length;
                if (nameArrayLength == 3) {
                    // Assume the string is "Family Middle Given".
                    mNameData.mPhoneticFamily = namesArray[0];
                    mNameData.mPhoneticMiddle = namesArray[1];
                    mNameData.mPhoneticGiven = namesArray[2];
                } else if (nameArrayLength == 2) {
                    // Assume the string is "Family Given" based on the Japanese mobile
                    // phones' preference.
                    mNameData.mPhoneticFamily = namesArray[0];
                    mNameData.mPhoneticGiven = namesArray[1];
                } else {
                    mNameData.mPhoneticGiven = elems.get(0);
                }
                return;
            }
        }

        switch (size) {
        // fallthrough
        case 3:
            mNameData.mPhoneticMiddle = elems.get(2);
        case 2:
            mNameData.mPhoneticGiven = elems.get(1);
        default:
            mNameData.mPhoneticFamily = elems.get(0);
        }
    }

    public void addProperty(final VCardProperty property) {
        final String propertyName = property.getName();
        final Map<String, Collection<String>> paramMap = property.getParameterMap();
        final List<String> propertyValueList = property.getValueList();
        byte[] propertyBytes = property.getByteValue();

        if ((propertyValueList == null || propertyValueList.size() == 0)
                && propertyBytes == null) {
            return;
        }
        final String propValue = (propertyValueList != null
                ? listToString(propertyValueList).trim()
                : null);

        if (propertyName.equals(VCardConstants.PROPERTY_VERSION)) {
            // vCard version. Ignore this.
        } else if (propertyName.equals(VCardConstants.PROPERTY_FN)) {
            mNameData.mFormatted = propValue;
        } else if (propertyName.equals(VCardConstants.PROPERTY_NAME)) {
            // Only in vCard 3.0. Use this if FN doesn't exist though it is
            // required in vCard 3.0.
            if (TextUtils.isEmpty(mNameData.mFormatted)) {
                mNameData.mFormatted = propValue;
            }
        } else if (propertyName.equals(VCardConstants.PROPERTY_N)) {
            handleNProperty(propertyValueList, paramMap);
        } else if (propertyName.equals(VCardConstants.PROPERTY_SORT_STRING)) {
            mNameData.mSortString = propValue;
        } else if (propertyName.equals(VCardConstants.PROPERTY_NICKNAME)
                || propertyName.equals(VCardConstants.ImportOnly.PROPERTY_X_NICKNAME)) {
            addNickName(propValue);
        } else if (propertyName.equals(VCardConstants.PROPERTY_SOUND)) {
            Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
            if (typeCollection != null
                    && typeCollection.contains(VCardConstants.PARAM_TYPE_X_IRMC_N)) {
                // As of 2009-10-08, Parser side does not split a property value into separated
                // values using ';' (in other words, propValueList.size() == 1),
                // which is correct behavior from the view of vCard 2.1.
                // But we want it to be separated, so do the separation here.
                final List<String> phoneticNameList = VCardUtils.constructListFromValue(propValue,
                        mVCardType);
                handlePhoneticNameFromSound(phoneticNameList);
            } else {
                // Ignore this field since Android cannot understand what it is.
            }
        } else if (propertyName.equals(VCardConstants.PROPERTY_ADR)) {
            boolean valuesAreAllEmpty = true;
            for (String value : propertyValueList) {
                if (!TextUtils.isEmpty(value)) {
                    valuesAreAllEmpty = false;
                    break;
                }
            }
            if (valuesAreAllEmpty) {
                return;
            }

            int type = -1;
            String label = null;
            boolean isPrimary = false;
            final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
            if (typeCollection != null) {
                for (final String typeStringOrg : typeCollection) {
                    final String typeStringUpperCase = typeStringOrg.toUpperCase();
                    if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_PREF)) {
                        isPrimary = true;
                    } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_HOME)) {
                        type = StructuredPostal.TYPE_HOME;
                        label = null;
                    } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_WORK)
                            || typeStringUpperCase
                                    .equalsIgnoreCase(VCardConstants.PARAM_EXTRA_TYPE_COMPANY)) {
                        // "COMPANY" seems emitted by Windows Mobile, which is not
                        // specifically supported by vCard 2.1. We assume this is same
                        // as "WORK".
                        type = StructuredPostal.TYPE_WORK;
                        label = null;
                    } else if (typeStringUpperCase.equals(VCardConstants.PARAM_ADR_TYPE_PARCEL)
                            || typeStringUpperCase.equals(VCardConstants.PARAM_ADR_TYPE_DOM)
                            || typeStringUpperCase.equals(VCardConstants.PARAM_ADR_TYPE_INTL)) {
                        // We do not have any appropriate way to store this information.
                    } else if (type < 0) { // If no other type is specified before.
                        type = StructuredPostal.TYPE_CUSTOM;
                        if (typeStringUpperCase.startsWith("X-")) { // If X- or x-
                            label = typeStringOrg.substring(2);
                        } else {
                            label = typeStringOrg;
                        }
                    }
                }
            }
            // We use "HOME" as default
            if (type < 0) {
                type = StructuredPostal.TYPE_HOME;
            }

            addPostal(type, propertyValueList, label, isPrimary);
        } else if (propertyName.equals(VCardConstants.PROPERTY_EMAIL)) {
            int type = -1;
            String label = null;
            boolean isPrimary = false;
            final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
            if (typeCollection != null) {
                for (final String typeStringOrg : typeCollection) {
                    final String typeStringUpperCase = typeStringOrg.toUpperCase();
                    if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_PREF)) {
                        isPrimary = true;
                    } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_HOME)) {
                        type = Email.TYPE_HOME;
                    } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_WORK)) {
                        type = Email.TYPE_WORK;
                    } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_CELL)) {
                        type = Email.TYPE_MOBILE;
                    } else if (type < 0) { // If no other type is specified before
                        if (typeStringUpperCase.startsWith("X-")) { // If X- or x-
                            label = typeStringOrg.substring(2);
                        } else {
                            label = typeStringOrg;
                        }
                        type = Email.TYPE_CUSTOM;
                    }
                }
            }
            if (type < 0) {
                type = Email.TYPE_OTHER;
            }
            addEmail(type, propValue, label, isPrimary);
        } else if (propertyName.equals(VCardConstants.PROPERTY_ORG)) {
            // vCard specification does not specify other types.
            final int type = Organization.TYPE_WORK;
            boolean isPrimary = false;
            Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
            if (typeCollection != null) {
                for (String typeString : typeCollection) {
                    if (typeString.equals(VCardConstants.PARAM_TYPE_PREF)) {
                        isPrimary = true;
                    }
                }
            }
            handleOrgValue(type, propertyValueList, paramMap, isPrimary);
        } else if (propertyName.equals(VCardConstants.PROPERTY_TITLE)) {
            handleTitleValue(propValue);
        } else if (propertyName.equals(VCardConstants.PROPERTY_ROLE)) {
            // This conflicts with TITLE. Ignore for now...
            // handleTitleValue(propValue);
        } else if (propertyName.equals(VCardConstants.PROPERTY_PHOTO)
                || propertyName.equals(VCardConstants.PROPERTY_LOGO)) {
            Collection<String> paramMapValue = paramMap.get("VALUE");
            if (paramMapValue != null && paramMapValue.contains("URL")) {
                // Currently we do not have appropriate example for testing this case.
            } else {
                final Collection<String> typeCollection = paramMap.get("TYPE");
                String formatName = null;
                boolean isPrimary = false;
                if (typeCollection != null) {
                    for (String typeValue : typeCollection) {
                        if (VCardConstants.PARAM_TYPE_PREF.equals(typeValue)) {
                            isPrimary = true;
                        } else if (formatName == null) {
                            formatName = typeValue;
                        }
                    }
                }
                addPhotoBytes(formatName, propertyBytes, isPrimary);
            }
        } else if (propertyName.equals(VCardConstants.PROPERTY_TEL)) {
            String phoneNumber = null;
            boolean isSip = false;
            if (VCardConfig.isVersion40(mVCardType)) {
                // Given propValue is in URI format, not in phone number format used until
                // vCard 3.0.
                if (propValue.startsWith("sip:")) {
                    isSip = true;
                } else if (propValue.startsWith("tel:")) {
                    phoneNumber = propValue.substring(4);
                } else {
                    // We don't know appropriate way to handle the other schemas. Also,
                    // we may still have non-URI phone number. To keep given data as much as
                    // we can, just save original value here.
                    phoneNumber = propValue;
                }
            } else {
                phoneNumber = propValue;
            }

            if (isSip) {
                final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
                handleSipCase(propValue, typeCollection);
            } else {
                if (propValue.length() == 0) {
                    return;
                }

                final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
                final Object typeObject = VCardUtils.getPhoneTypeFromStrings(typeCollection,
                        phoneNumber);
                final int type;
                final String label;
                if (typeObject instanceof Integer) {
                    type = (Integer) typeObject;
                    label = null;
                } else {
                    type = Phone.TYPE_CUSTOM;
                    label = typeObject.toString();
                }

                final boolean isPrimary;
                if (typeCollection != null &&
                        typeCollection.contains(VCardConstants.PARAM_TYPE_PREF)) {
                    isPrimary = true;
                } else {
                    isPrimary = false;
                }

                addPhone(type, phoneNumber, label, isPrimary);
            }
        } else if (propertyName.equals(VCardConstants.PROPERTY_X_SKYPE_PSTNNUMBER)) {
            // The phone number available via Skype.
            Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
            final int type = Phone.TYPE_OTHER;
            final boolean isPrimary;
            if (typeCollection != null
                    && typeCollection.contains(VCardConstants.PARAM_TYPE_PREF)) {
                isPrimary = true;
            } else {
                isPrimary = false;
            }
            addPhone(type, propValue, null, isPrimary);
        } else if (sImMap.containsKey(propertyName)) {
            final int protocol = sImMap.get(propertyName);
            boolean isPrimary = false;
            int type = -1;
            final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
            if (typeCollection != null) {
                for (String typeString : typeCollection) {
                    if (typeString.equals(VCardConstants.PARAM_TYPE_PREF)) {
                        isPrimary = true;
                    } else if (type < 0) {
                        if (typeString.equalsIgnoreCase(VCardConstants.PARAM_TYPE_HOME)) {
                            type = Im.TYPE_HOME;
                        } else if (typeString.equalsIgnoreCase(VCardConstants.PARAM_TYPE_WORK)) {
                            type = Im.TYPE_WORK;
                        }
                    }
                }
            }
            if (type < 0) {
                type = Im.TYPE_HOME;
            }
            addIm(protocol, null, propValue, type, isPrimary);
        } else if (propertyName.equals(VCardConstants.PROPERTY_NOTE)) {
            addNote(propValue);
        } else if (propertyName.equals(VCardConstants.PROPERTY_URL)) {
            if (mWebsiteList == null) {
                mWebsiteList = new ArrayList<WebsiteData>(1);
            }
            mWebsiteList.add(new WebsiteData(propValue));
        } else if (propertyName.equals(VCardConstants.PROPERTY_BDAY)) {
            mBirthday = new BirthdayData(propValue);
        } else if (propertyName.equals(VCardConstants.PROPERTY_ANNIVERSARY)) {
            mAnniversary = new AnniversaryData(propValue);
        } else if (propertyName.equals(VCardConstants.PROPERTY_X_PHONETIC_FIRST_NAME)) {
            mNameData.mPhoneticGiven = propValue;
        } else if (propertyName.equals(VCardConstants.PROPERTY_X_PHONETIC_MIDDLE_NAME)) {
            mNameData.mPhoneticMiddle = propValue;
        } else if (propertyName.equals(VCardConstants.PROPERTY_X_PHONETIC_LAST_NAME)) {
            mNameData.mPhoneticFamily = propValue;
        } else if (propertyName.equals(VCardConstants.PROPERTY_IMPP)) {
            // See also RFC 4770 (for vCard 3.0)
            if (propValue.startsWith("sip:")) {
                final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
                handleSipCase(propValue, typeCollection);
            }
        } else if (propertyName.equals(VCardConstants.PROPERTY_X_SIP)) {
            if (!TextUtils.isEmpty(propValue)) {
                final Collection<String> typeCollection = paramMap.get(VCardConstants.PARAM_TYPE);
                handleSipCase(propValue, typeCollection);
            }
        } else if (propertyName.equals(VCardConstants.PROPERTY_X_ANDROID_CUSTOM)) {
            final List<String> customPropertyList = VCardUtils.constructListFromValue(propValue,
                    mVCardType);
            handleAndroidCustomProperty(customPropertyList);
        } else if (propertyName.toUpperCase().startsWith("X-")) {
            // Catch all for X- properties. The caller can decide what to do with these.
            if (mUnknownXData == null) {
                mUnknownXData = new ArrayList<Pair<String, String>>();
            }
            mUnknownXData.add(new Pair<String, String>(propertyName, propValue));
        } else {
        }
        // Be careful when adding some logic here, as some blocks above may use "return".
    }

    /**
     * @param propValue may contain "sip:" at the beginning.
     * @param typeCollection
     */
    private void handleSipCase(String propValue, Collection<String> typeCollection) {
        if (TextUtils.isEmpty(propValue)) {
            return;
        }
        if (propValue.startsWith("sip:")) {
            propValue = propValue.substring(4);
            if (propValue.length() == 0) {
                return;
            }
        }

        int type = -1;
        String label = null;
        boolean isPrimary = false;
        if (typeCollection != null) {
            for (final String typeStringOrg : typeCollection) {
                final String typeStringUpperCase = typeStringOrg.toUpperCase();
                if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_PREF)) {
                    isPrimary = true;
                } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_HOME)) {
                    type = SipAddress.TYPE_HOME;
                } else if (typeStringUpperCase.equals(VCardConstants.PARAM_TYPE_WORK)) {
                    type = SipAddress.TYPE_WORK;
                } else if (type < 0) { // If no other type is specified before
                    if (typeStringUpperCase.startsWith("X-")) { // If X- or x-
                        label = typeStringOrg.substring(2);
                    } else {
                        label = typeStringOrg;
                    }
                    type = SipAddress.TYPE_CUSTOM;
                }
            }
        }
        if (type < 0) {
            type = SipAddress.TYPE_OTHER;
        }
        addSip(propValue, type, label, isPrimary);
    }

    public void addChild(VCardEntry child) {
        if (mChildren == null) {
            mChildren = new ArrayList<VCardEntry>();
        }
        mChildren.add(child);
    }

    private void handleAndroidCustomProperty(final List<String> customPropertyList) {
        if (mAndroidCustomDataList == null) {
            mAndroidCustomDataList = new ArrayList<AndroidCustomData>();
        }
        mAndroidCustomDataList
                .add(AndroidCustomData.constructAndroidCustomData(customPropertyList));
    }

    /**
     * Construct the display name. The constructed data must not be null.
     */
    private String constructDisplayName() {
        String displayName = null;
        // FullName (created via "FN" or "NAME" field) is prefered.
        if (!TextUtils.isEmpty(mNameData.mFormatted)) {
            displayName = mNameData.mFormatted;
        } else if (!mNameData.emptyStructuredName()) {
            displayName = VCardUtils.constructNameFromElements(mVCardType, mNameData.mFamily,
                    mNameData.mMiddle, mNameData.mGiven, mNameData.mPrefix, mNameData.mSuffix);
        } else if (!mNameData.emptyPhoneticStructuredName()) {
            displayName = VCardUtils.constructNameFromElements(mVCardType,
                    mNameData.mPhoneticFamily, mNameData.mPhoneticMiddle, mNameData.mPhoneticGiven);
        } else if (mEmailList != null && mEmailList.size() > 0) {
            displayName = mEmailList.get(0).mAddress;
        } else if (mPhoneList != null && mPhoneList.size() > 0) {
            displayName = mPhoneList.get(0).mNumber;
        } else if (mPostalList != null && mPostalList.size() > 0) {
            displayName = mPostalList.get(0).getFormattedAddress(mVCardType);
        } else if (mOrganizationList != null && mOrganizationList.size() > 0) {
            displayName = mOrganizationList.get(0).getFormattedString();
        }
        if (displayName == null) {
            displayName = "";
        }
        return displayName;
    }

    /**
     * Consolidate several fielsds (like mName) using name candidates,
     */
    public void consolidateFields() {
        mNameData.displayName = constructDisplayName();
    }

    /**
     * @return true when this object has nothing meaningful for Android's
     *         Contacts, and thus is "ignorable" for Android's Contacts. This
     *         does not mean an original vCard is really empty. Even when the
     *         original vCard has some fields, this may ignore it if those
     *         fields cannot be transcoded into Android's Contacts
     *         representation.
     */
    public boolean isIgnorable() {
        IsIgnorableIterator iterator = new IsIgnorableIterator();
        iterateAllData(iterator);
        return iterator.getResult();
    }

    /**
     * Constructs the list of insert operation for this object. When the
     * operationList argument is null, this method creates a new ArrayList and
     * return it. The returned object is filled with new insert operations for
     * this object. When operationList argument is not null, this method appends
     * those new operations into the object instead of creating a new ArrayList.
     *
     * @param resolver {@link ContentResolver} object to be used in this method.
     * @param operationList object to be filled. You can use this argument to
     *            concatinate operation lists. If null, this method creates a
     *            new array object.
     * @return If operationList argument is null, new object with new insert
     *         operations. If it is not null, the operationList object with
     *         operations inserted by this method.
     */
    public ArrayList<ContentProviderOperation> constructInsertOperations(ContentResolver resolver,
            ArrayList<ContentProviderOperation> operationList) {
        if (operationList == null) {
            operationList = new ArrayList<ContentProviderOperation>();
        }

        if (isIgnorable()) {
            return operationList;
        }

        final int backReferenceIndex = operationList.size();

        // After applying the batch the first result's Uri is returned so it is important that
        // the RawContact is the first operation that gets inserted into the list.
        ContentProviderOperation.Builder builder = ContentProviderOperation
                .newInsert(RawContacts.CONTENT_URI);
        if (mAccount != null) {
            builder.withValue(RawContacts.ACCOUNT_NAME, mAccount.name);
            builder.withValue(RawContacts.ACCOUNT_TYPE, mAccount.type);
        } else {
            builder.withValue(RawContacts.ACCOUNT_NAME, null);
            builder.withValue(RawContacts.ACCOUNT_TYPE, null);
        }
        operationList.add(builder.build());

        int start = operationList.size();
        iterateAllData(new InsertOperationConstrutor(operationList, backReferenceIndex));
        int end = operationList.size();

        return operationList;
    }

    public static VCardEntry buildFromResolver(ContentResolver resolver) {
        return buildFromResolver(resolver, Contacts.CONTENT_URI);
    }

    public static VCardEntry buildFromResolver(ContentResolver resolver, Uri uri) {
        return null;
    }

    private String listToString(List<String> list) {
        final int size = list.size();
        if (size > 1) {
            StringBuilder builder = new StringBuilder();
            int i = 0;
            for (String type : list) {
                builder.append(type);
                if (i < size - 1) {
                    builder.append(";");
                }
            }
            return builder.toString();
        } else if (size == 1) {
            return list.get(0);
        } else {
            return "";
        }
    }

    public final NameData getNameData() {
        return mNameData;
    }

    public final List<NicknameData> getNickNameList() {
        return mNicknameList;
    }

    public final String getBirthday() {
        return mBirthday != null ? mBirthday.mBirthday : null;
    }

    public final List<NoteData> getNotes() {
        return mNoteList;
    }

    public final List<PhoneData> getPhoneList() {
        return mPhoneList;
    }

    public final List<EmailData> getEmailList() {
        return mEmailList;
    }

    public final List<PostalData> getPostalList() {
        return mPostalList;
    }

    public final List<OrganizationData> getOrganizationList() {
        return mOrganizationList;
    }

    public final List<ImData> getImList() {
        return mImList;
    }

    public final List<PhotoData> getPhotoList() {
        return mPhotoList;
    }

    public final List<WebsiteData> getWebsiteList() {
        return mWebsiteList;
    }

    /**
     * @hide this interface may be changed for better support of vCard 4.0 (UID)
     */
    public final List<VCardEntry> getChildlen() {
        return mChildren;
    }

    public String getDisplayName() {
        if (mNameData.displayName == null) {
            mNameData.displayName = constructDisplayName();
        }
        return mNameData.displayName;
    }

    public List<Pair<String, String>> getUnknownXData() {
        return mUnknownXData;
    }
}
