/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.ex.chips;

import android.net.Uri;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.DisplayNameSources;
import android.support.annotation.DrawableRes;
import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;

/**
 * Represents one entry inside recipient auto-complete list.
 */
public class RecipientEntry {
    /* package */ static final int INVALID_CONTACT = -1;
    /**
     * A GENERATED_CONTACT is one that was created based entirely on
     * information passed in to the RecipientEntry from an external source
     * that is not a real contact.
     */
    /* package */ static final int GENERATED_CONTACT = -2;

    /** Used when {@link #mDestinationType} is invalid and thus shouldn't be used for display. */
    public static final int INVALID_DESTINATION_TYPE = -1;

    public static final int ENTRY_TYPE_PERSON = 0;

    /**
     * Entry of this type represents the item in auto-complete that asks user to grant permissions
     * to the app. This permission model is introduced in M platform.
     *
     * <p>Entries of this type should have {@link #mPermissions} set as well.
     */
    public static final int ENTRY_TYPE_PERMISSION_REQUEST = 1;

    public static final int ENTRY_TYPE_SIZE = 2;

    private final int mEntryType;

    /**
     * True when this entry is the first entry in a group, which should have a photo and display
     * name, while the second or later entries won't.
     */
    private boolean mIsFirstLevel;
    private final String mDisplayName;

    /** Destination for this contact entry. Would be an email address or a phone number. */
    private final String mDestination;
    /** Type of the destination like {@link Email#TYPE_HOME} */
    private final int mDestinationType;
    /**
     * Label of the destination which will be used when type was {@link Email#TYPE_CUSTOM}.
     * Can be null when {@link #mDestinationType} is {@link #INVALID_DESTINATION_TYPE}.
     */
    private final String mDestinationLabel;
    /** ID for the person */
    private final long mContactId;
    /** ID for the directory this contact came from, or <code>null</code> */
    private final Long mDirectoryId;
    /** ID for the destination */
    private final long mDataId;

    private final Uri mPhotoThumbnailUri;
    /** Configures showing the icon in the chip */
    private final boolean mShouldDisplayIcon;

    private boolean mIsValid;
    /**
     * This can be updated after this object being constructed, when the photo is fetched
     * from remote directories.
     */
    private byte[] mPhotoBytes;

    @DrawableRes private int mIndicatorIconId;
    private String mIndicatorText;

    /** See {@link android.provider.ContactsContract.ContactsColumns#LOOKUP_KEY} */
    private final String mLookupKey;

    /** Should be used when type is {@link #ENTRY_TYPE_PERMISSION_REQUEST}. */
    private final String[] mPermissions;

    protected RecipientEntry(int entryType, String displayName, String destination,
        int destinationType, String destinationLabel, long contactId, Long directoryId,
        long dataId, Uri photoThumbnailUri, boolean isFirstLevel, boolean isValid,
        String lookupKey, String[] permissions) {
        this(entryType, displayName, destination, destinationType,
            destinationLabel, contactId, directoryId, dataId, photoThumbnailUri,
            true /* shouldDisplayIcon */, isFirstLevel, isValid, lookupKey, permissions);
    }

    protected RecipientEntry(int entryType, String displayName, String destination,
            int destinationType, String destinationLabel, long contactId, Long directoryId,
            long dataId, Uri photoThumbnailUri, boolean shouldDisplayIcon,
            boolean isFirstLevel, boolean isValid, String lookupKey, String[] permissions) {
        mEntryType = entryType;
        mIsFirstLevel = isFirstLevel;
        mDisplayName = displayName;
        mDestination = destination;
        mDestinationType = destinationType;
        mDestinationLabel = destinationLabel;
        mContactId = contactId;
        mDirectoryId = directoryId;
        mDataId = dataId;
        mPhotoThumbnailUri = photoThumbnailUri;
        mShouldDisplayIcon = shouldDisplayIcon;
        mPhotoBytes = null;
        mIsValid = isValid;
        mLookupKey = lookupKey;
        mIndicatorIconId = 0;
        mIndicatorText = null;
        mPermissions = permissions;
    }

    protected RecipientEntry(int entryType, String displayName, String destination,
            int destinationType, String destinationLabel, long contactId, Long directoryId,
            long dataId, Uri photoThumbnailUri, boolean isFirstLevel, boolean isValid,
            String lookupKey) {
        this(entryType, displayName, destination, destinationType, destinationLabel,
                contactId, directoryId, dataId, photoThumbnailUri, isFirstLevel, isValid,
                lookupKey, null);
    }

    public boolean isValid() {
        return mIsValid;
    }

    /**
     * Determine if this was a RecipientEntry created from recipient info or
     * an entry from contacts.
     */
    public static boolean isCreatedRecipient(long id) {
        return id == RecipientEntry.INVALID_CONTACT || id == RecipientEntry.GENERATED_CONTACT;
    }

    /**
     * Construct a RecipientEntry from just an address that has been entered.
     * This address has not been resolved to a contact and therefore does not
     * have a contact id or photo.
     */
    public static RecipientEntry constructFakeEntry(final String address, final boolean isValid) {
        final Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(address);
        final String tokenizedAddress = tokens.length > 0 ? tokens[0].getAddress() : address;

        return new RecipientEntry(ENTRY_TYPE_PERSON, tokenizedAddress, tokenizedAddress,
                INVALID_DESTINATION_TYPE, null, INVALID_CONTACT, null /* directoryId */,
                INVALID_CONTACT, null, true, isValid, null /* lookupKey */, null /* permissions */);
    }

    /**
     * Construct a RecipientEntry from just a phone number.
     */
    public static RecipientEntry constructFakePhoneEntry(final String phoneNumber,
            final boolean isValid) {
        return new RecipientEntry(ENTRY_TYPE_PERSON, phoneNumber, phoneNumber,
                INVALID_DESTINATION_TYPE, null, INVALID_CONTACT, null /* directoryId */,
                INVALID_CONTACT, null, true, isValid, null /* lookupKey */, null /* permissions */);
    }

    /**
     * Construct a RecipientEntry from just an address that has been entered
     * with both an associated display name. This address has not been resolved
     * to a contact and therefore does not have a contact id or photo.
     */
    public static RecipientEntry constructGeneratedEntry(String display, String address,
            boolean isValid) {
        return new RecipientEntry(ENTRY_TYPE_PERSON, display, address, INVALID_DESTINATION_TYPE,
                null, GENERATED_CONTACT, null /* directoryId */, GENERATED_CONTACT, null, true,
                isValid, null /* lookupKey */, null /* permissions */);
    }

    public static RecipientEntry constructTopLevelEntry(String displayName, int displayNameSource,
            String destination, int destinationType, String destinationLabel, long contactId,
            Long directoryId, long dataId, Uri photoThumbnailUri, boolean isValid,
            String lookupKey) {
        return new RecipientEntry(ENTRY_TYPE_PERSON, pickDisplayName(displayNameSource,
                displayName, destination), destination, destinationType, destinationLabel,
                contactId, directoryId, dataId, photoThumbnailUri, true, isValid, lookupKey,
                null /* permissions */);
    }

    public static RecipientEntry constructTopLevelEntry(String displayName, int displayNameSource,
            String destination, int destinationType, String destinationLabel, long contactId,
            Long directoryId, long dataId, String thumbnailUriAsString, boolean isValid,
            String lookupKey) {
        return new RecipientEntry(ENTRY_TYPE_PERSON, pickDisplayName(displayNameSource,
                displayName, destination), destination, destinationType, destinationLabel,
                contactId, directoryId, dataId, (thumbnailUriAsString != null
                ? Uri.parse(thumbnailUriAsString) : null), true, isValid, lookupKey,
                null /* permissions */);
    }

    public static RecipientEntry constructSecondLevelEntry(String displayName,
            int displayNameSource, String destination, int destinationType,
            String destinationLabel, long contactId, Long directoryId, long dataId,
            String thumbnailUriAsString, boolean isValid, String lookupKey) {
        return new RecipientEntry(ENTRY_TYPE_PERSON, pickDisplayName(displayNameSource,
                displayName, destination), destination, destinationType, destinationLabel,
                contactId, directoryId, dataId, (thumbnailUriAsString != null
                ? Uri.parse(thumbnailUriAsString) : null), false, isValid, lookupKey,
                null /* permissions */);
    }

    public static RecipientEntry constructPermissionEntry(String[] permissions) {
        return new RecipientEntry(
                ENTRY_TYPE_PERMISSION_REQUEST,
                "" /* displayName */,
                "" /* destination */,
                Email.TYPE_CUSTOM,
                "" /* destinationLabel */,
                INVALID_CONTACT,
                null /* directoryId */,
                INVALID_CONTACT,
                null /* photoThumbnailUri */,
                true /* isFirstLevel*/,
                false /* isValid */,
                null /* lookupKey */,
                permissions);
    }

    /**
     * @return the display name for the entry.  If the display name source is larger than
     * {@link DisplayNameSources#PHONE} we use the contact's display name, but if not,
     * i.e. the display name came from an email address or a phone number, we don't use it
     * to avoid confusion and just use the destination instead.
     */
    private static String pickDisplayName(int displayNameSource, String displayName,
            String destination) {
        return (displayNameSource > DisplayNameSources.PHONE) ? displayName : destination;
    }

    public int getEntryType() {
        return mEntryType;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public String getDestination() {
        return mDestination;
    }

    public int getDestinationType() {
        return mDestinationType;
    }

    public String getDestinationLabel() {
        return mDestinationLabel;
    }

    public long getContactId() {
        return mContactId;
    }

    public Long getDirectoryId() {
        return mDirectoryId;
    }

    public long getDataId() {
        return mDataId;
    }

    public boolean isFirstLevel() {
        return mIsFirstLevel;
    }

    public Uri getPhotoThumbnailUri() {
        return mPhotoThumbnailUri;
    }

    /** Indicates whether the icon in the chip is displayed or not. */
    public boolean shouldDisplayIcon() {
        return mShouldDisplayIcon;
    }

    /** This can be called outside main Looper thread. */
    public synchronized void setPhotoBytes(byte[] photoBytes) {
        mPhotoBytes = photoBytes;
    }

    /** This can be called outside main Looper thread. */
    public synchronized byte[] getPhotoBytes() {
        return mPhotoBytes;
    }

    /**
     * Used together with {@link #ENTRY_TYPE_PERMISSION_REQUEST} and indicates what permissions we
     * need to ask user to grant.
     */
    public String[] getPermissions() {
        return mPermissions;
    }

    public String getLookupKey() {
        return mLookupKey;
    }

    public boolean isSelectable() {
        return mEntryType == ENTRY_TYPE_PERSON || mEntryType == ENTRY_TYPE_PERMISSION_REQUEST;
    }

    @Override
    public String toString() {
        return mDisplayName + " <" + mDestination + ">, isValid=" + mIsValid;
    }

    /**
     * Returns if entry represents the same person as this instance. The default implementation
     * checks whether the contact ids are the same, and subclasses may opt to override this.
     */
    public boolean isSamePerson(final RecipientEntry entry) {
        return entry != null && mContactId == entry.mContactId;
    }

    /**
     * Returns the resource ID for the indicator icon, or 0 if no icon should be displayed.
     */
    @DrawableRes
    public int getIndicatorIconId() {
        return mIndicatorIconId;
    }

    /**
     * Sets the indicator icon to the given resource ID.  Set to 0 to display no icon.
     */
    public void setIndicatorIconId(@DrawableRes int indicatorIconId) {
        mIndicatorIconId = indicatorIconId;
    }

    /**
     * Get the indicator text, or null if no text should be displayed.
     */
    public String getIndicatorText() {
        return mIndicatorText;
    }

    /**
     * Set the indicator text.  Set to null for no text to be displayed.
     */
    public void setIndicatorText(String indicatorText) {
        mIndicatorText = indicatorText;
    }
}
