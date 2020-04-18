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

import android.accounts.Account;
import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.graphics.drawable.StateListDrawable;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.text.TextUtils;
import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CursorAdapter;

import com.android.ex.chips.BaseRecipientAdapter.DirectoryListQuery;
import com.android.ex.chips.BaseRecipientAdapter.DirectorySearchParams;
import com.android.ex.chips.DropdownChipLayouter.AdapterType;
import com.android.ex.chips.Queries.Query;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * RecipientAlternatesAdapter backs the RecipientEditTextView for managing contacts
 * queried by email or by phone number.
 */
public class RecipientAlternatesAdapter extends CursorAdapter {
    public static final int MAX_LOOKUPS = 50;

    private final long mCurrentId;

    private int mCheckedItemPosition = -1;

    private OnCheckedItemChangedListener mCheckedItemChangedListener;

    private static final String TAG = "RecipAlternates";

    public static final int QUERY_TYPE_EMAIL = 0;
    public static final int QUERY_TYPE_PHONE = 1;
    private final Long mDirectoryId;
    private DropdownChipLayouter mDropdownChipLayouter;
    private final StateListDrawable mDeleteDrawable;

    private static final Map<String, String> sCorrectedPhotoUris = new HashMap<String, String>();

    public interface RecipientMatchCallback {
        public void matchesFound(Map<String, RecipientEntry> results);
        /**
         * Called with all addresses that could not be resolved to valid recipients.
         */
        public void matchesNotFound(Set<String> unfoundAddresses);
    }

    public static void getMatchingRecipients(Context context, BaseRecipientAdapter adapter,
            ArrayList<String> inAddresses, Account account, RecipientMatchCallback callback,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        getMatchingRecipients(context, adapter, inAddresses, QUERY_TYPE_EMAIL, account, callback,
                permissionsCheckListener);
    }

    /**
     * Get a HashMap of address to RecipientEntry that contains all contact
     * information for a contact with the provided address, if one exists. This
     * may block the UI, so run it in an async task.
     *
     * @param context Context.
     * @param inAddresses Array of addresses on which to perform the lookup.
     * @param callback RecipientMatchCallback called when a match or matches are found.
     */
    public static void getMatchingRecipients(Context context, BaseRecipientAdapter adapter,
            ArrayList<String> inAddresses, int addressType, Account account,
            RecipientMatchCallback callback,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        Queries.Query query;
        if (addressType == QUERY_TYPE_EMAIL) {
            query = Queries.EMAIL;
        } else {
            query = Queries.PHONE;
        }
        int addressesSize = Math.min(MAX_LOOKUPS, inAddresses.size());
        HashSet<String> addresses = new HashSet<String>();
        StringBuilder bindString = new StringBuilder();
        // Create the "?" string and set up arguments.
        for (int i = 0; i < addressesSize; i++) {
            Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(inAddresses.get(i).toLowerCase());
            addresses.add(tokens.length > 0 ? tokens[0].getAddress() : inAddresses.get(i));
            bindString.append("?");
            if (i < addressesSize - 1) {
                bindString.append(",");
            }
        }

        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "Doing reverse lookup for " + addresses.toString());
        }

        String[] addressArray = new String[addresses.size()];
        addresses.toArray(addressArray);
        HashMap<String, RecipientEntry> recipientEntries = null;
        Cursor c = null;

        try {
            if (ChipsUtil.hasPermissions(context, permissionsCheckListener)) {
                c = context.getContentResolver().query(
                        query.getContentUri(),
                        query.getProjection(),
                        query.getProjection()[Queries.Query.DESTINATION] + " IN ("
                                + bindString.toString() + ")", addressArray, null);
            }
            recipientEntries = processContactEntries(c, null /* directoryId */);
            callback.matchesFound(recipientEntries);
        } finally {
            if (c != null) {
                c.close();
            }
        }

        final Set<String> matchesNotFound = new HashSet<String>();

        getMatchingRecipientsFromDirectoryQueries(context, recipientEntries,
                addresses, account, matchesNotFound, query, callback, permissionsCheckListener);

        getMatchingRecipientsFromExtensionMatcher(adapter, matchesNotFound, callback);
    }

    public static void getMatchingRecipientsFromDirectoryQueries(Context context,
            Map<String, RecipientEntry> recipientEntries, Set<String> addresses,
            Account account, Set<String> matchesNotFound,
            RecipientMatchCallback callback,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        getMatchingRecipientsFromDirectoryQueries(
                context, recipientEntries, addresses, account,
                matchesNotFound, Queries.EMAIL, callback, permissionsCheckListener);
    }

    private static void getMatchingRecipientsFromDirectoryQueries(Context context,
            Map<String, RecipientEntry> recipientEntries, Set<String> addresses,
            Account account, Set<String> matchesNotFound, Queries.Query query,
            RecipientMatchCallback callback,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        // See if any entries did not resolve; if so, we need to check other
        // directories

        if (recipientEntries.size() < addresses.size()) {
            // Run a directory query for each unmatched recipient.
            HashSet<String> unresolvedAddresses = new HashSet<String>();
            for (String address : addresses) {
                if (!recipientEntries.containsKey(address)) {
                    unresolvedAddresses.add(address);
                }
            }
            matchesNotFound.addAll(unresolvedAddresses);

            final List<DirectorySearchParams> paramsList;
            Cursor directoryCursor = null;
            try {
                if (ChipsUtil.hasPermissions(context, permissionsCheckListener)) {
                    directoryCursor = context.getContentResolver().query(
                            DirectoryListQuery.URI, DirectoryListQuery.PROJECTION,
                            null, null, null);
                }
                if (directoryCursor == null) {
                    return;
                }
                paramsList = BaseRecipientAdapter.setupOtherDirectories(
                        context, directoryCursor, account);
            } finally {
                if (directoryCursor != null) {
                    directoryCursor.close();
                }
            }

            if (paramsList != null) {
                Cursor directoryContactsCursor = null;
                for (String unresolvedAddress : unresolvedAddresses) {
                    for (int i = 0; i < paramsList.size(); i++) {
                        final long directoryId = paramsList.get(i).directoryId;
                        try {
                            directoryContactsCursor = doQuery(unresolvedAddress, 1 /* limit */,
                                    directoryId, account, context, query, permissionsCheckListener);
                            if (directoryContactsCursor != null
                                    && directoryContactsCursor.getCount() != 0) {
                                // We found the directory with at least one contact
                                final Map<String, RecipientEntry> entries =
                                        processContactEntries(directoryContactsCursor, directoryId);

                                for (final String address : entries.keySet()) {
                                    matchesNotFound.remove(address);
                                }

                                callback.matchesFound(entries);
                                break;
                            }
                        } finally {
                            if (directoryContactsCursor != null) {
                                directoryContactsCursor.close();
                                directoryContactsCursor = null;
                            }
                        }
                    }
                }
            }
        }
    }

    public static void getMatchingRecipientsFromExtensionMatcher(BaseRecipientAdapter adapter,
            Set<String> matchesNotFound, RecipientMatchCallback callback) {
        // If no matches found in contact provider or the directories, try the extension
        // matcher.
        // todo (aalbert): This whole method needs to be in the adapter?
        if (adapter != null) {
            final Map<String, RecipientEntry> entries =
                    adapter.getMatchingRecipients(matchesNotFound);
            if (entries != null && entries.size() > 0) {
                callback.matchesFound(entries);
                for (final String address : entries.keySet()) {
                    matchesNotFound.remove(address);
                }
            }
        }
        callback.matchesNotFound(matchesNotFound);
    }

    private static HashMap<String, RecipientEntry> processContactEntries(Cursor c,
            Long directoryId) {
        HashMap<String, RecipientEntry> recipientEntries = new HashMap<String, RecipientEntry>();
        if (c != null && c.moveToFirst()) {
            do {
                String address = c.getString(Queries.Query.DESTINATION);

                final RecipientEntry newRecipientEntry = RecipientEntry.constructTopLevelEntry(
                        c.getString(Queries.Query.NAME),
                        c.getInt(Queries.Query.DISPLAY_NAME_SOURCE),
                        c.getString(Queries.Query.DESTINATION),
                        c.getInt(Queries.Query.DESTINATION_TYPE),
                        c.getString(Queries.Query.DESTINATION_LABEL),
                        c.getLong(Queries.Query.CONTACT_ID),
                        directoryId,
                        c.getLong(Queries.Query.DATA_ID),
                        c.getString(Queries.Query.PHOTO_THUMBNAIL_URI),
                        true,
                        c.getString(Queries.Query.LOOKUP_KEY));

                /*
                 * In certain situations, we may have two results for one address, where one of the
                 * results is just the email address, and the other has a name and photo, so we want
                 * to use the better one.
                 */
                final RecipientEntry recipientEntry =
                        getBetterRecipient(recipientEntries.get(address), newRecipientEntry);

                recipientEntries.put(address, recipientEntry);
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "Received reverse look up information for " + address
                            + " RESULTS: "
                            + " NAME : " + c.getString(Queries.Query.NAME)
                            + " CONTACT ID : " + c.getLong(Queries.Query.CONTACT_ID)
                            + " ADDRESS :" + c.getString(Queries.Query.DESTINATION));
                }
            } while (c.moveToNext());
        }
        return recipientEntries;
    }

    /**
     * Given two {@link RecipientEntry}s for the same email address, this will return the one that
     * contains more complete information for display purposes. Defaults to <code>entry2</code> if
     * no significant differences are found.
     */
    static RecipientEntry getBetterRecipient(final RecipientEntry entry1,
            final RecipientEntry entry2) {
        // If only one has passed in, use it
        if (entry2 == null) {
            return entry1;
        }

        if (entry1 == null) {
            return entry2;
        }

        // If only one has a display name, use it
        if (!TextUtils.isEmpty(entry1.getDisplayName())
                && TextUtils.isEmpty(entry2.getDisplayName())) {
            return entry1;
        }

        if (!TextUtils.isEmpty(entry2.getDisplayName())
                && TextUtils.isEmpty(entry1.getDisplayName())) {
            return entry2;
        }

        // If only one has a display name that is not the same as the destination, use it
        if (!TextUtils.equals(entry1.getDisplayName(), entry1.getDestination())
                && TextUtils.equals(entry2.getDisplayName(), entry2.getDestination())) {
            return entry1;
        }

        if (!TextUtils.equals(entry2.getDisplayName(), entry2.getDestination())
                && TextUtils.equals(entry1.getDisplayName(), entry1.getDestination())) {
            return entry2;
        }

        // If only one has a photo, use it
        if ((entry1.getPhotoThumbnailUri() != null || entry1.getPhotoBytes() != null)
                && (entry2.getPhotoThumbnailUri() == null && entry2.getPhotoBytes() == null)) {
            return entry1;
        }

        if ((entry2.getPhotoThumbnailUri() != null || entry2.getPhotoBytes() != null)
                && (entry1.getPhotoThumbnailUri() == null && entry1.getPhotoBytes() == null)) {
            return entry2;
        }

        // Go with the second option as a default
        return entry2;
    }

    private static Cursor doQuery(CharSequence constraint, int limit, Long directoryId,
            Account account, Context context, Query query,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        if (!ChipsUtil.hasPermissions(context, permissionsCheckListener)) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "Not doing query because we don't have required permissions.");
            }
            return null;
        }
        final Uri.Builder builder = query
                .getContentFilterUri()
                .buildUpon()
                .appendPath(constraint.toString())
                .appendQueryParameter(ContactsContract.LIMIT_PARAM_KEY,
                        String.valueOf(limit + BaseRecipientAdapter.ALLOWANCE_FOR_DUPLICATES));
        if (directoryId != null) {
            builder.appendQueryParameter(ContactsContract.DIRECTORY_PARAM_KEY,
                    String.valueOf(directoryId));
        }
        if (account != null) {
            builder.appendQueryParameter(BaseRecipientAdapter.PRIMARY_ACCOUNT_NAME, account.name);
            builder.appendQueryParameter(BaseRecipientAdapter.PRIMARY_ACCOUNT_TYPE, account.type);
        }
        return context.getContentResolver()
                .query(builder.build(), query.getProjection(), null, null, null);
    }

    public RecipientAlternatesAdapter(Context context, long contactId, Long directoryId,
            String lookupKey, long currentId, int queryMode, OnCheckedItemChangedListener listener,
            DropdownChipLayouter dropdownChipLayouter,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        this(context, contactId, directoryId, lookupKey, currentId, queryMode, listener,
                dropdownChipLayouter, null, permissionsCheckListener);
    }

    public RecipientAlternatesAdapter(Context context, long contactId, Long directoryId,
            String lookupKey, long currentId, int queryMode, OnCheckedItemChangedListener listener,
            DropdownChipLayouter dropdownChipLayouter, StateListDrawable deleteDrawable,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        super(context,
                getCursorForConstruction(context, contactId, directoryId, lookupKey, queryMode,
                        permissionsCheckListener),
                0);
        mCurrentId = currentId;
        mDirectoryId = directoryId;
        mCheckedItemChangedListener = listener;

        mDropdownChipLayouter = dropdownChipLayouter;
        mDeleteDrawable = deleteDrawable;
    }

    private static Cursor getCursorForConstruction(Context context, long contactId,
            Long directoryId, String lookupKey, int queryType,
            ChipsUtil.PermissionsCheckListener permissionsCheckListener) {
        final Uri uri;
        final String desiredMimeType;
        final String[] projection;

        if (queryType == QUERY_TYPE_EMAIL) {
            projection = Queries.EMAIL.getProjection();

            if (directoryId == null || lookupKey == null) {
                uri = Queries.EMAIL.getContentUri();
                desiredMimeType = null;
            } else {
                uri = Contacts.getLookupUri(contactId, lookupKey)
                        .buildUpon()
                        .appendPath(Contacts.Entity.CONTENT_DIRECTORY)
                        .appendQueryParameter(
                                ContactsContract.DIRECTORY_PARAM_KEY, String.valueOf(directoryId))
                        .build();
                desiredMimeType = ContactsContract.CommonDataKinds.Email.CONTENT_ITEM_TYPE;
            }
        } else {
            projection = Queries.PHONE.getProjection();

            if (lookupKey == null) {
                uri = Queries.PHONE.getContentUri();
                desiredMimeType = null;
            } else {
                uri = Contacts.getLookupUri(contactId, lookupKey)
                        .buildUpon()
                        .appendPath(Contacts.Entity.CONTENT_DIRECTORY)
                        .appendQueryParameter(
                                ContactsContract.DIRECTORY_PARAM_KEY, String.valueOf(directoryId))
                        .build();
                desiredMimeType = ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE;
            }
        }

        final String selection = new StringBuilder()
                .append(projection[Queries.Query.CONTACT_ID])
                .append(" = ?")
                .toString();
        final Cursor cursor;
        if (ChipsUtil.hasPermissions(context, permissionsCheckListener)) {
            cursor = context.getContentResolver().query(
                    uri, projection, selection, new String[] {String.valueOf(contactId)}, null);
        } else {
            cursor = new MatrixCursor(projection);
        }

        final Cursor resultCursor = removeUndesiredDestinations(cursor, desiredMimeType, lookupKey);
        cursor.close();

        return resultCursor;
    }

    /**
     * @return a new cursor based on the given cursor with all duplicate destinations removed.
     *
     * It's only intended to use for the alternate list, so...
     * - This method ignores all other fields and dedupe solely on the destination.  Normally,
     * if a cursor contains multiple contacts and they have the same destination, we'd still want
     * to show both.
     * - This method creates a MatrixCursor, so all data will be kept in memory.  We wouldn't want
     * to do this if the original cursor is large, but it's okay here because the alternate list
     * won't be that big.
     *
     * @param desiredMimeType If this is non-<code>null</code>, only entries with this mime type
     *            will be added to the cursor
     * @param lookupKey The lookup key used for this contact if there isn't one in the cursor. This
     *            should be the same one used in the query that returned the cursor
     */
    // Visible for testing
    static Cursor removeUndesiredDestinations(final Cursor original, final String desiredMimeType,
            final String lookupKey) {
        final MatrixCursor result = new MatrixCursor(
                original.getColumnNames(), original.getCount());
        final HashSet<String> destinationsSeen = new HashSet<String>();

        String defaultDisplayName = null;
        String defaultPhotoThumbnailUri = null;
        int defaultDisplayNameSource = 0;

        // Find some nice defaults in case we need them
        original.moveToPosition(-1);
        while (original.moveToNext()) {
            final String mimeType = original.getString(Query.MIME_TYPE);

            if (ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE.equals(
                    mimeType)) {
                // Store this data
                defaultDisplayName = original.getString(Query.NAME);
                defaultPhotoThumbnailUri = original.getString(Query.PHOTO_THUMBNAIL_URI);
                defaultDisplayNameSource = original.getInt(Query.DISPLAY_NAME_SOURCE);
                break;
            }
        }

        original.moveToPosition(-1);
        while (original.moveToNext()) {
            if (desiredMimeType != null) {
                final String mimeType = original.getString(Query.MIME_TYPE);
                if (!desiredMimeType.equals(mimeType)) {
                    continue;
                }
            }
            final String destination = original.getString(Query.DESTINATION);
            if (destinationsSeen.contains(destination)) {
                continue;
            }
            destinationsSeen.add(destination);

            final Object[] row = new Object[] {
                    original.getString(Query.NAME),
                    original.getString(Query.DESTINATION),
                    original.getInt(Query.DESTINATION_TYPE),
                    original.getString(Query.DESTINATION_LABEL),
                    original.getLong(Query.CONTACT_ID),
                    original.getLong(Query.DATA_ID),
                    original.getString(Query.PHOTO_THUMBNAIL_URI),
                    original.getInt(Query.DISPLAY_NAME_SOURCE),
                    original.getString(Query.LOOKUP_KEY),
                    original.getString(Query.MIME_TYPE)
            };

            if (row[Query.NAME] == null) {
                row[Query.NAME] = defaultDisplayName;
            }
            if (row[Query.PHOTO_THUMBNAIL_URI] == null) {
                row[Query.PHOTO_THUMBNAIL_URI] = defaultPhotoThumbnailUri;
            }
            if ((Integer) row[Query.DISPLAY_NAME_SOURCE] == 0) {
                row[Query.DISPLAY_NAME_SOURCE] = defaultDisplayNameSource;
            }
            if (row[Query.LOOKUP_KEY] == null) {
                row[Query.LOOKUP_KEY] = lookupKey;
            }

            // Ensure we don't have two '?' like content://.../...?account_name=...?sz=...
            final String photoThumbnailUri = (String) row[Query.PHOTO_THUMBNAIL_URI];
            if (photoThumbnailUri != null) {
                if (sCorrectedPhotoUris.containsKey(photoThumbnailUri)) {
                    row[Query.PHOTO_THUMBNAIL_URI] = sCorrectedPhotoUris.get(photoThumbnailUri);
                } else if (photoThumbnailUri.indexOf('?') != photoThumbnailUri.lastIndexOf('?')) {
                    final String[] parts = photoThumbnailUri.split("\\?");
                    final StringBuilder correctedUriBuilder = new StringBuilder();
                    for (int i = 0; i < parts.length; i++) {
                        if (i == 1) {
                            correctedUriBuilder.append("?"); // We only want one of these
                        } else if (i > 1) {
                            correctedUriBuilder.append("&"); // And we want these elsewhere
                        }
                        correctedUriBuilder.append(parts[i]);
                    }

                    final String correctedUri = correctedUriBuilder.toString();
                    sCorrectedPhotoUris.put(photoThumbnailUri, correctedUri);
                    row[Query.PHOTO_THUMBNAIL_URI] = correctedUri;
                }
            }

            result.addRow(row);
        }

        return result;
    }

    @Override
    public long getItemId(int position) {
        Cursor c = getCursor();
        if (c.moveToPosition(position)) {
            c.getLong(Queries.Query.DATA_ID);
        }
        return -1;
    }

    public RecipientEntry getRecipientEntry(int position) {
        Cursor c = getCursor();
        c.moveToPosition(position);
        return RecipientEntry.constructTopLevelEntry(
                c.getString(Queries.Query.NAME),
                c.getInt(Queries.Query.DISPLAY_NAME_SOURCE),
                c.getString(Queries.Query.DESTINATION),
                c.getInt(Queries.Query.DESTINATION_TYPE),
                c.getString(Queries.Query.DESTINATION_LABEL),
                c.getLong(Queries.Query.CONTACT_ID),
                mDirectoryId,
                c.getLong(Queries.Query.DATA_ID),
                c.getString(Queries.Query.PHOTO_THUMBNAIL_URI),
                true,
                c.getString(Queries.Query.LOOKUP_KEY));
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        Cursor cursor = getCursor();
        cursor.moveToPosition(position);
        if (convertView == null) {
            convertView = mDropdownChipLayouter.newView(AdapterType.RECIPIENT_ALTERNATES);
        }
        if (cursor.getLong(Queries.Query.DATA_ID) == mCurrentId) {
            mCheckedItemPosition = position;
            if (mCheckedItemChangedListener != null) {
                mCheckedItemChangedListener.onCheckedItemChanged(mCheckedItemPosition);
            }
        }
        bindView(convertView, convertView.getContext(), cursor);
        return convertView;
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
        int position = cursor.getPosition();
        RecipientEntry entry = getRecipientEntry(position);

        mDropdownChipLayouter.bindView(view, null, entry, position,
                AdapterType.RECIPIENT_ALTERNATES, null, mDeleteDrawable);
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
        return mDropdownChipLayouter.newView(AdapterType.RECIPIENT_ALTERNATES);
    }

    /*package*/ static interface OnCheckedItemChangedListener {
        public void onCheckedItemChanged(int position);
    }
}
