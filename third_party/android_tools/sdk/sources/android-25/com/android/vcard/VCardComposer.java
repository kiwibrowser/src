/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.vcard;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Entity;
import android.content.Entity.NamedContentValues;
import android.content.EntityIterator;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Event;
import android.provider.ContactsContract.CommonDataKinds.Im;
import android.provider.ContactsContract.CommonDataKinds.Nickname;
import android.provider.ContactsContract.CommonDataKinds.Note;
import android.provider.ContactsContract.CommonDataKinds.Organization;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.provider.ContactsContract.CommonDataKinds.Relation;
import android.provider.ContactsContract.CommonDataKinds.SipAddress;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.CommonDataKinds.StructuredPostal;
import android.provider.ContactsContract.CommonDataKinds.Website;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.RawContactsEntity;
import android.provider.ContactsContract;
import android.text.TextUtils;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * <p>
 * The class for composing vCard from Contacts information.
 * </p>
 * <p>
 * Usually, this class should be used like this.
 * </p>
 * <pre class="prettyprint">VCardComposer composer = null;
 * try {
 *     composer = new VCardComposer(context);
 *     composer.addHandler(
 *             composer.new HandlerForOutputStream(outputStream));
 *     if (!composer.init()) {
 *         // Do something handling the situation.
 *         return;
 *     }
 *     while (!composer.isAfterLast()) {
 *         if (mCanceled) {
 *             // Assume a user may cancel this operation during the export.
 *             return;
 *         }
 *         if (!composer.createOneEntry()) {
 *             // Do something handling the error situation.
 *             return;
 *         }
 *     }
 * } finally {
 *     if (composer != null) {
 *         composer.terminate();
 *     }
 * }</pre>
 * <p>
 * Users have to manually take care of memory efficiency. Even one vCard may contain
 * image of non-trivial size for mobile devices.
 * </p>
 * <p>
 * {@link VCardBuilder} is used to build each vCard.
 * </p>
 */
public class VCardComposer {
    private static final String LOG_TAG = "VCardComposer";
    private static final boolean DEBUG = false;

    public static final String FAILURE_REASON_FAILED_TO_GET_DATABASE_INFO =
        "Failed to get database information";

    public static final String FAILURE_REASON_NO_ENTRY =
        "There's no exportable in the database";

    public static final String FAILURE_REASON_NOT_INITIALIZED =
        "The vCard composer object is not correctly initialized";

    /** Should be visible only from developers... (no need to translate, hopefully) */
    public static final String FAILURE_REASON_UNSUPPORTED_URI =
        "The Uri vCard composer received is not supported by the composer.";

    public static final String NO_ERROR = "No error";

    // Strictly speaking, "Shift_JIS" is the most appropriate, but we use upper version here,
    // since usual vCard devices for Japanese devices already use it.
    private static final String SHIFT_JIS = "SHIFT_JIS";
    private static final String UTF_8 = "UTF-8";

    private static final Map<Integer, String> sImMap;

    static {
        sImMap = new HashMap<Integer, String>();
        sImMap.put(Im.PROTOCOL_AIM, VCardConstants.PROPERTY_X_AIM);
        sImMap.put(Im.PROTOCOL_MSN, VCardConstants.PROPERTY_X_MSN);
        sImMap.put(Im.PROTOCOL_YAHOO, VCardConstants.PROPERTY_X_YAHOO);
        sImMap.put(Im.PROTOCOL_ICQ, VCardConstants.PROPERTY_X_ICQ);
        sImMap.put(Im.PROTOCOL_JABBER, VCardConstants.PROPERTY_X_JABBER);
        sImMap.put(Im.PROTOCOL_SKYPE, VCardConstants.PROPERTY_X_SKYPE_USERNAME);
        // We don't add Google talk here since it has to be handled separately.
    }

    private final int mVCardType;
    private final ContentResolver mContentResolver;

    private final boolean mIsDoCoMo;
    /**
     * Used only when {@link #mIsDoCoMo} is true. Set to true when the first vCard for DoCoMo
     * vCard is emitted.
     */
    private boolean mFirstVCardEmittedInDoCoMoCase;

    private Cursor mCursor;
    private boolean mCursorSuppliedFromOutside;
    private int mIdColumn;
    private Uri mContentUriForRawContactsEntity;

    private final String mCharset;

    private boolean mInitDone;
    private String mErrorReason = NO_ERROR;

    /**
     * Set to false when one of {@link #init()} variants is called, and set to true when
     * {@link #terminate()} is called. Initially set to true.
     */
    private boolean mTerminateCalled = true;

    private RawContactEntitlesInfoCallback mRawContactEntitlesInfoCallback;

    private static final String[] sContactsProjection = new String[] {
        Contacts._ID,
    };

    public VCardComposer(Context context) {
        this(context, VCardConfig.VCARD_TYPE_DEFAULT, null, true);
    }

    /**
     * The variant which sets charset to null and sets careHandlerErrors to true.
     */
    public VCardComposer(Context context, int vcardType) {
        this(context, vcardType, null, true);
    }

    public VCardComposer(Context context, int vcardType, String charset) {
        this(context, vcardType, charset, true);
    }

    /**
     * The variant which sets charset to null.
     */
    public VCardComposer(final Context context, final int vcardType,
            final boolean careHandlerErrors) {
        this(context, vcardType, null, careHandlerErrors);
    }

    /**
     * Constructs for supporting call log entry vCard composing.
     *
     * @param context Context to be used during the composition.
     * @param vcardType The type of vCard, typically available via {@link VCardConfig}.
     * @param charset The charset to be used. Use null when you don't need the charset.
     * @param careHandlerErrors If true, This object returns false everytime
     */
    public VCardComposer(final Context context, final int vcardType, String charset,
            final boolean careHandlerErrors) {
        this(context, context.getContentResolver(), vcardType, charset, careHandlerErrors);
    }

    /**
     * Just for testing for now.
     * @param resolver {@link ContentResolver} which used by this object.
     * @hide
     */
    public VCardComposer(final Context context, ContentResolver resolver,
            final int vcardType, String charset, final boolean careHandlerErrors) {
        // Not used right now
        // mContext = context;
        mVCardType = vcardType;
        mContentResolver = resolver;

        mIsDoCoMo = VCardConfig.isDoCoMo(vcardType);

        charset = (TextUtils.isEmpty(charset) ? VCardConfig.DEFAULT_EXPORT_CHARSET : charset);
        final boolean shouldAppendCharsetParam = !(
                VCardConfig.isVersion30(vcardType) && UTF_8.equalsIgnoreCase(charset));

        if (mIsDoCoMo || shouldAppendCharsetParam) {
            if (SHIFT_JIS.equalsIgnoreCase(charset)) {
                mCharset = charset;
            } else {
                /* Log.w(LOG_TAG,
                        "The charset \"" + charset + "\" is used while "
                        + SHIFT_JIS + " is needed to be used."); */
                if (TextUtils.isEmpty(charset)) {
                    mCharset = SHIFT_JIS;
                } else {
                    mCharset = charset;
                }
            }
        } else {
            if (TextUtils.isEmpty(charset)) {
                mCharset = UTF_8;
            } else {
                mCharset = charset;
            }
        }

        Log.d(LOG_TAG, "Use the charset \"" + mCharset + "\"");
    }

    /**
     * Initializes this object using default {@link Contacts#CONTENT_URI}.
     *
     * You can call this method or a variant of this method just once. In other words, you cannot
     * reuse this object.
     *
     * @return Returns true when initialization is successful and all the other
     *          methods are available. Returns false otherwise.
     */
    public boolean init() {
        return init(null, null);
    }

    /**
     * Special variant of init(), which accepts a Uri for obtaining {@link RawContactsEntity} from
     * {@link ContentResolver} with {@link Contacts#_ID}.
     * <code>
     * String selection = Data.CONTACT_ID + "=?";
     * String[] selectionArgs = new String[] {contactId};
     * Cursor cursor = mContentResolver.query(
     *         contentUriForRawContactsEntity, null, selection, selectionArgs, null)
     * </code>
     *
     * You can call this method or a variant of this method just once. In other words, you cannot
     * reuse this object.
     *
     * @deprecated Use {@link #init(Uri, String[], String, String[], String, Uri)} if you really
     * need to change the default Uri.
     */
    @Deprecated
    public boolean initWithRawContactsEntityUri(Uri contentUriForRawContactsEntity) {
        return init(Contacts.CONTENT_URI, sContactsProjection, null, null, null,
                contentUriForRawContactsEntity);
    }

    /**
     * Initializes this object using default {@link Contacts#CONTENT_URI} and given selection
     * arguments.
     */
    public boolean init(final String selection, final String[] selectionArgs) {
        return init(Contacts.CONTENT_URI, sContactsProjection, selection, selectionArgs,
                null, null);
    }

    /**
     * Note that this is unstable interface, may be deleted in the future.
     */
    public boolean init(final Uri contentUri, final String selection,
            final String[] selectionArgs, final String sortOrder) {
        return init(contentUri, sContactsProjection, selection, selectionArgs, sortOrder, null);
    }

    /**
     * @param contentUri Uri for obtaining the list of contactId. Used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param selection selection used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param selectionArgs selectionArgs used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param sortOrder sortOrder used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param contentUriForRawContactsEntity Uri for obtaining entries relevant to each
     * contactId.
     * Note that this is an unstable interface, may be deleted in the future.
     */
    public boolean init(final Uri contentUri, final String selection,
            final String[] selectionArgs, final String sortOrder,
            final Uri contentUriForRawContactsEntity) {
        return init(contentUri, sContactsProjection, selection, selectionArgs, sortOrder,
                contentUriForRawContactsEntity);
    }

    /**
     * A variant of init(). Currently just for testing. Use other variants for init().
     *
     * First we'll create {@link Cursor} for the list of contactId.
     *
     * <code>
     * Cursor cursorForId = mContentResolver.query(
     *         contentUri, projection, selection, selectionArgs, sortOrder);
     * </code>
     *
     * After that, we'll obtain data for each contactId in the list.
     *
     * <code>
     * Cursor cursorForContent = mContentResolver.query(
     *         contentUriForRawContactsEntity, null,
     *         Data.CONTACT_ID + "=?", new String[] {contactId}, null)
     * </code>
     *
     * {@link #createOneEntry()} or its variants let the caller obtain each entry from
     * <code>cursorForContent</code> above.
     *
     * @param contentUri Uri for obtaining the list of contactId. Used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param projection projection used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param selection selection used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param selectionArgs selectionArgs used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param sortOrder sortOrder used with
     * {@link ContentResolver#query(Uri, String[], String, String[], String)}
     * @param contentUriForRawContactsEntity Uri for obtaining entries relevant to each
     * contactId.
     * @return true when successful
     *
     * @hide
     */
    public boolean init(final Uri contentUri, final String[] projection,
            final String selection, final String[] selectionArgs,
            final String sortOrder, Uri contentUriForRawContactsEntity) {
        if (!ContactsContract.AUTHORITY.equals(contentUri.getAuthority())) {
            if (DEBUG) Log.d(LOG_TAG, "Unexpected contentUri: " + contentUri);
            mErrorReason = FAILURE_REASON_UNSUPPORTED_URI;
            return false;
        }

        if (!initInterFirstPart(contentUriForRawContactsEntity)) {
            return false;
        }
        if (!initInterCursorCreationPart(contentUri, projection, selection, selectionArgs,
                sortOrder)) {
            return false;
        }
        if (!initInterMainPart()) {
            return false;
        }
        return initInterLastPart();
    }

    /**
     * Just for testing for now. Do not use.
     * @hide
     */
    public boolean init(Cursor cursor) {
        return initWithCallback(cursor, null);
    }

    /**
    * @param cursor Cursor that used to get contact id
    * @param rawContactEntitlesInfoCallback Callback that return RawContactEntitlesInfo
    * Note that this is an unstable interface, may be deleted in the future.
    *
    * @return true when successful
    */
    public boolean initWithCallback(Cursor cursor,
            RawContactEntitlesInfoCallback rawContactEntitlesInfoCallback) {
        if (!initInterFirstPart(null)) {
            return false;
        }
        mCursorSuppliedFromOutside = true;
        mCursor = cursor;
        mRawContactEntitlesInfoCallback = rawContactEntitlesInfoCallback;
        if (!initInterMainPart()) {
            return false;
        }
        return initInterLastPart();
    }

    private boolean initInterFirstPart(Uri contentUriForRawContactsEntity) {
        mContentUriForRawContactsEntity =
                (contentUriForRawContactsEntity != null ? contentUriForRawContactsEntity :
                        RawContactsEntity.CONTENT_URI);
        if (mInitDone) {
            Log.e(LOG_TAG, "init() is already called");
            return false;
        }
        return true;
    }

    private boolean initInterCursorCreationPart(
            final Uri contentUri, final String[] projection,
            final String selection, final String[] selectionArgs, final String sortOrder) {
        mCursorSuppliedFromOutside = false;
        mCursor = mContentResolver.query(
                contentUri, projection, selection, selectionArgs, sortOrder);
        if (mCursor == null) {
            Log.e(LOG_TAG, String.format("Cursor became null unexpectedly"));
            mErrorReason = FAILURE_REASON_FAILED_TO_GET_DATABASE_INFO;
            return false;
        }
        return true;
    }

    private boolean initInterMainPart() {
        if (mCursor.getCount() == 0 || !mCursor.moveToFirst()) {
            if (DEBUG) {
                Log.d(LOG_TAG,
                    String.format("mCursor has an error (getCount: %d): ", mCursor.getCount()));
            }
            closeCursorIfAppropriate();
            return false;
        }
        mIdColumn = mCursor.getColumnIndex(Data.CONTACT_ID);
        if (mIdColumn < 0) {
            mIdColumn = mCursor.getColumnIndex(Contacts._ID);
        }
        return mIdColumn >= 0;
    }

    private boolean initInterLastPart() {
        mInitDone = true;
        mTerminateCalled = false;
        return true;
    }

    /**
     * @return a vCard string.
     */
    public String createOneEntry() {
        return createOneEntry(null);
    }

    /**
     * @hide
     */
    public String createOneEntry(Method getEntityIteratorMethod) {
        if (mIsDoCoMo && !mFirstVCardEmittedInDoCoMoCase) {
            mFirstVCardEmittedInDoCoMoCase = true;
            // Previously we needed to emit empty data for this specific case, but actually
            // this doesn't work now, as resolver doesn't return any data with "-1" contactId.
            // TODO: re-introduce or remove this logic. Needs to modify unit test when we
            // re-introduce the logic.
            // return createOneEntryInternal("-1", getEntityIteratorMethod);
        }

        final String vcard = createOneEntryInternal(mCursor.getLong(mIdColumn),
                getEntityIteratorMethod);
        if (!mCursor.moveToNext()) {
            Log.e(LOG_TAG, "Cursor#moveToNext() returned false");
        }
        return vcard;
    }

    /**
     *  Class that store rawContactEntitlesUri and contactId
     */
    public static class RawContactEntitlesInfo {
        public final Uri rawContactEntitlesUri;
        public final long contactId;
        public RawContactEntitlesInfo(Uri rawContactEntitlesUri, long contactId) {
            this.rawContactEntitlesUri = rawContactEntitlesUri;
            this.contactId = contactId;
        }
    }

    /**
    * Listener for getting raw contact entitles info
    */
    public interface RawContactEntitlesInfoCallback {
        /**
        * Callback to get RawContactEntitlesInfo from contact id
        *
        * @param contactId Contact id that you want to process.
        * @return RawContactEntitlesInfo that ready to process.
        */
        RawContactEntitlesInfo getRawContactEntitlesInfo(long contactId);
    }

    private String createOneEntryInternal(long contactId,
            final Method getEntityIteratorMethod) {
        final Map<String, List<ContentValues>> contentValuesListMap =
                new HashMap<String, List<ContentValues>>();
        // The resolver may return the entity iterator with no data. It is possible.
        // e.g. If all the data in the contact of the given contact id are not exportable ones,
        //      they are hidden from the view of this method, though contact id itself exists.
        EntityIterator entityIterator = null;
        try {
            Uri uri = mContentUriForRawContactsEntity;
            if (mRawContactEntitlesInfoCallback != null) {
                RawContactEntitlesInfo rawContactEntitlesInfo =
                        mRawContactEntitlesInfoCallback.getRawContactEntitlesInfo(contactId);
                uri = rawContactEntitlesInfo.rawContactEntitlesUri;
                contactId = rawContactEntitlesInfo.contactId;
            }
            final String selection = Data.CONTACT_ID + "=?";
            final String[] selectionArgs = new String[] {String.valueOf(contactId)};
            if (getEntityIteratorMethod != null) {
                // Please note that this branch is executed by unit tests only
                try {
                    entityIterator = (EntityIterator)getEntityIteratorMethod.invoke(null,
                            mContentResolver, uri, selection, selectionArgs, null);
                } catch (IllegalArgumentException e) {
                    Log.e(LOG_TAG, "IllegalArgumentException has been thrown: " +
                            e.getMessage());
                } catch (IllegalAccessException e) {
                    Log.e(LOG_TAG, "IllegalAccessException has been thrown: " +
                            e.getMessage());
                } catch (InvocationTargetException e) {
                    Log.e(LOG_TAG, "InvocationTargetException has been thrown: ", e);
                    throw new RuntimeException("InvocationTargetException has been thrown");
                }
            } else {
                entityIterator = RawContacts.newEntityIterator(mContentResolver.query(
                        uri, null, selection, selectionArgs, null));
            }

            if (entityIterator == null) {
                Log.e(LOG_TAG, "EntityIterator is null");
                return "";
            }

            if (!entityIterator.hasNext()) {
                Log.w(LOG_TAG, "Data does not exist. contactId: " + contactId);
                return "";
            }

            while (entityIterator.hasNext()) {
                Entity entity = entityIterator.next();
                for (NamedContentValues namedContentValues : entity.getSubValues()) {
                    ContentValues contentValues = namedContentValues.values;
                    String key = contentValues.getAsString(Data.MIMETYPE);
                    if (key != null) {
                        List<ContentValues> contentValuesList =
                                contentValuesListMap.get(key);
                        if (contentValuesList == null) {
                            contentValuesList = new ArrayList<ContentValues>();
                            contentValuesListMap.put(key, contentValuesList);
                        }
                        contentValuesList.add(contentValues);
                    }
                }
            }
        } finally {
            if (entityIterator != null) {
                entityIterator.close();
            }
        }

        return buildVCard(contentValuesListMap);
    }

    private VCardPhoneNumberTranslationCallback mPhoneTranslationCallback;
    /**
     * <p>
     * Set a callback for phone number formatting. It will be called every time when this object
     * receives a phone number for printing.
     * </p>
     * <p>
     * When this is set {@link VCardConfig#FLAG_REFRAIN_PHONE_NUMBER_FORMATTING} will be ignored
     * and the callback should be responsible for everything about phone number formatting.
     * </p>
     * <p>
     * Caution: This interface will change. Please don't use without any strong reason.
     * </p>
     */
    public void setPhoneNumberTranslationCallback(VCardPhoneNumberTranslationCallback callback) {
        mPhoneTranslationCallback = callback;
    }

    /**
     * Builds and returns vCard using given map, whose key is CONTENT_ITEM_TYPE defined in
     * {ContactsContract}. Developers can override this method to customize the output.
     */
    public String buildVCard(final Map<String, List<ContentValues>> contentValuesListMap) {
        if (contentValuesListMap == null) {
            Log.e(LOG_TAG, "The given map is null. Ignore and return empty String");
            return "";
        } else {
            final VCardBuilder builder = new VCardBuilder(mVCardType, mCharset);
            builder.appendNameProperties(contentValuesListMap.get(StructuredName.CONTENT_ITEM_TYPE))
                    .appendNickNames(contentValuesListMap.get(Nickname.CONTENT_ITEM_TYPE))
                    .appendPhones(contentValuesListMap.get(Phone.CONTENT_ITEM_TYPE),
                            mPhoneTranslationCallback)
                    .appendEmails(contentValuesListMap.get(Email.CONTENT_ITEM_TYPE))
                    .appendPostals(contentValuesListMap.get(StructuredPostal.CONTENT_ITEM_TYPE))
                    .appendOrganizations(contentValuesListMap.get(Organization.CONTENT_ITEM_TYPE))
                    .appendWebsites(contentValuesListMap.get(Website.CONTENT_ITEM_TYPE));
            if ((mVCardType & VCardConfig.FLAG_REFRAIN_IMAGE_EXPORT) == 0) {
                builder.appendPhotos(contentValuesListMap.get(Photo.CONTENT_ITEM_TYPE));
            }
            builder.appendNotes(contentValuesListMap.get(Note.CONTENT_ITEM_TYPE))
                    .appendEvents(contentValuesListMap.get(Event.CONTENT_ITEM_TYPE))
                    .appendIms(contentValuesListMap.get(Im.CONTENT_ITEM_TYPE))
                    .appendSipAddresses(contentValuesListMap.get(SipAddress.CONTENT_ITEM_TYPE))
                    .appendRelation(contentValuesListMap.get(Relation.CONTENT_ITEM_TYPE));
            return builder.toString();
        }
    }

    public void terminate() {
        closeCursorIfAppropriate();
        mTerminateCalled = true;
    }

    private void closeCursorIfAppropriate() {
        if (!mCursorSuppliedFromOutside && mCursor != null) {
            try {
                mCursor.close();
            } catch (SQLiteException e) {
                Log.e(LOG_TAG, "SQLiteException on Cursor#close(): " + e.getMessage());
            }
            mCursor = null;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (!mTerminateCalled) {
                Log.e(LOG_TAG, "finalized() is called before terminate() being called");
            }
        } finally {
            super.finalize();
        }
    }

    /**
     * @return returns the number of available entities. The return value is undefined
     * when this object is not ready yet (typically when {{@link #init()} is not called
     * or when {@link #terminate()} is already called).
     */
    public int getCount() {
        if (mCursor == null) {
            Log.w(LOG_TAG, "This object is not ready yet.");
            return 0;
        }
        return mCursor.getCount();
    }

    /**
     * @return true when there's no entity to be built. The return value is undefined
     * when this object is not ready yet.
     */
    public boolean isAfterLast() {
        if (mCursor == null) {
            Log.w(LOG_TAG, "This object is not ready yet.");
            return false;
        }
        return mCursor.isAfterLast();
    }

    /**
     * @return Returns the error reason.
     */
    public String getErrorReason() {
        return mErrorReason;
    }
}
