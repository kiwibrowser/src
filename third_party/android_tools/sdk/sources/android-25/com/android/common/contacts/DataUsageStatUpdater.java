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

package com.android.common.contacts;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.Data;
import android.text.TextUtils;
import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;
import android.util.Log;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Convenient class for updating usage statistics in ContactsProvider.
 *
 * Applications like Email, Sms, etc. can promote recipients for better sorting with this class
 *
 * @see ContactsContract.Contacts
 */
public class DataUsageStatUpdater {
    private static final String TAG = DataUsageStatUpdater.class.getSimpleName();

    /**
     * Copied from API in ICS (not available before it). You can use values here if you are sure
     * it is supported by the device.
     */
    public static final class DataUsageFeedback {
        static final Uri FEEDBACK_URI =
            Uri.withAppendedPath(Data.CONTENT_URI, "usagefeedback");

        static final String USAGE_TYPE = "type";
        public static final String USAGE_TYPE_CALL = "call";
        public static final String USAGE_TYPE_LONG_TEXT = "long_text";
        public static final String USAGE_TYPE_SHORT_TEXT = "short_text";
    }

    private final ContentResolver mResolver;

    public DataUsageStatUpdater(Context context) {
        mResolver = context.getContentResolver();
    }

    /**
     * Updates usage statistics using comma-separated RFC822 address like
     * "Joe <joe@example.com>, Due <due@example.com>".
     *
     * This will cause Disk access so should be called in a background thread.
     *
     * @return true when update request is correctly sent. False when the request fails,
     * input has no valid entities.
     */
    public boolean updateWithRfc822Address(Collection<CharSequence> texts){
        if (texts == null) {
            return false;
        } else {
            final Set<String> addresses = new HashSet<String>();
            for (CharSequence text : texts) {
                Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(text.toString().trim());
                for (Rfc822Token token : tokens) {
                    addresses.add(token.getAddress());
                }
            }
            return updateWithAddress(addresses);
        }
    }

    /**
     * Update usage statistics information using a list of email addresses.
     *
     * This will cause Disk access so should be called in a background thread.
     *
     * @see #update(Collection, Collection, String)
     *
     * @return true when update request is correctly sent. False when the request fails,
     * input has no valid entities.
     */
    public boolean updateWithAddress(Collection<String> addresses) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "updateWithAddress: " + Arrays.toString(addresses.toArray()));
        }
        if (addresses != null && !addresses.isEmpty()) {
            final ArrayList<String> whereArgs = new ArrayList<String>();
            final StringBuilder whereBuilder = new StringBuilder();
            final String[] questionMarks = new String[addresses.size()];

            whereArgs.addAll(addresses);
            Arrays.fill(questionMarks, "?");
            // Email.ADDRESS == Email.DATA1. Email.ADDRESS can be available from API Level 11.
            whereBuilder.append(Email.DATA1 + " IN (")
                    .append(TextUtils.join(",", questionMarks))
                    .append(")");
            final Cursor cursor = mResolver.query(Email.CONTENT_URI,
                    new String[] {Email.CONTACT_ID, Email._ID}, whereBuilder.toString(),
                    whereArgs.toArray(new String[0]), null);

            if (cursor == null) {
                Log.w(TAG, "Cursor for Email.CONTENT_URI became null.");
            } else {
                final Set<Long> contactIds = new HashSet<Long>(cursor.getCount());
                final Set<Long> dataIds = new HashSet<Long>(cursor.getCount());
                try {
                    cursor.move(-1);
                    while(cursor.moveToNext()) {
                        contactIds.add(cursor.getLong(0));
                        dataIds.add(cursor.getLong(1));
                    }
                } finally {
                    cursor.close();
                }
                return update(contactIds, dataIds, DataUsageFeedback.USAGE_TYPE_LONG_TEXT);
            }
        }

        return false;
    }

    /**
     * Update usage statistics information using a list of phone numbers.
     *
     * This will cause Disk access so should be called in a background thread.
     *
     * @see #update(Collection, Collection, String)
     *
     * @return true when update request is correctly sent. False when the request fails,
     * input has no valid entities.
     */
    public boolean updateWithPhoneNumber(Collection<String> numbers) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "updateWithPhoneNumber: " + Arrays.toString(numbers.toArray()));
        }
        if (numbers != null && !numbers.isEmpty()) {
            final ArrayList<String> whereArgs = new ArrayList<String>();
            final StringBuilder whereBuilder = new StringBuilder();
            final String[] questionMarks = new String[numbers.size()];

            whereArgs.addAll(numbers);
            Arrays.fill(questionMarks, "?");
            // Phone.NUMBER == Phone.DATA1. NUMBER can be available from API Level 11.
            whereBuilder.append(Phone.DATA1 + " IN (")
                    .append(TextUtils.join(",", questionMarks))
                    .append(")");
            final Cursor cursor = mResolver.query(Phone.CONTENT_URI,
                    new String[] {Phone.CONTACT_ID, Phone._ID}, whereBuilder.toString(),
                    whereArgs.toArray(new String[0]), null);

            if (cursor == null) {
                Log.w(TAG, "Cursor for Phone.CONTENT_URI became null.");
            } else {
                final Set<Long> contactIds = new HashSet<Long>(cursor.getCount());
                final Set<Long> dataIds = new HashSet<Long>(cursor.getCount());
                try {
                    cursor.move(-1);
                    while(cursor.moveToNext()) {
                        contactIds.add(cursor.getLong(0));
                        dataIds.add(cursor.getLong(1));
                    }
                } finally {
                    cursor.close();
                }
                return update(contactIds, dataIds, DataUsageFeedback.USAGE_TYPE_SHORT_TEXT);
            }
        }
        return false;
    }

    /**
     * @return true when one or more of update requests are correctly sent.
     * False when all the requests fail.
     */
    private boolean update(Collection<Long> contactIds, Collection<Long> dataIds, String type) {
        final long currentTimeMillis = System.currentTimeMillis();

        boolean successful = false;

        // From ICS (SDK_INT 14) we can use per-contact-method structure. We'll check if the device
        // supports it and call the API.
        if (Build.VERSION.SDK_INT >= 14) {
            if (dataIds.isEmpty()) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "Given list for data IDs is null. Ignoring.");
                }
            } else {
                final Uri uri = DataUsageFeedback.FEEDBACK_URI.buildUpon()
                        .appendPath(TextUtils.join(",", dataIds))
                        .appendQueryParameter(DataUsageFeedback.USAGE_TYPE, type)
                        .build();
                if (mResolver.update(uri, new ContentValues(), null, null) > 0) {
                    successful = true;
                } else {
                    if (Log.isLoggable(TAG, Log.DEBUG)) {
                        Log.d(TAG, "update toward data rows " + dataIds + " failed");
                    }
                }
            }
        } else {
            // Use older API.
            if (contactIds.isEmpty()) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "Given list for contact IDs is null. Ignoring.");
                }
            } else {
                final StringBuilder whereBuilder = new StringBuilder();
                final ArrayList<String> whereArgs = new ArrayList<String>();
                final String[] questionMarks = new String[contactIds.size()];
                for (long contactId : contactIds) {
                    whereArgs.add(String.valueOf(contactId));
                }
                Arrays.fill(questionMarks, "?");
                whereBuilder.append(ContactsContract.Contacts._ID + " IN (").
                        append(TextUtils.join(",", questionMarks)).
                        append(")");

                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "contactId where: " + whereBuilder.toString());
                    Log.d(TAG, "contactId selection: " + whereArgs);
                }

                final ContentValues values = new ContentValues();
                values.put(ContactsContract.Contacts.LAST_TIME_CONTACTED, currentTimeMillis);
                if (mResolver.update(ContactsContract.Contacts.CONTENT_URI, values,
                        whereBuilder.toString(), whereArgs.toArray(new String[0])) > 0) {
                    successful = true;
                } else {
                    if (Log.isLoggable(TAG, Log.DEBUG)) {
                        Log.d(TAG, "update toward raw contacts " + contactIds + " failed");
                    }
                }
            }
        }

        return successful;
    }
}
