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
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Directory;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.text.util.Rfc822Token;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AutoCompleteTextView;
import android.widget.BaseAdapter;
import android.widget.Filter;
import android.widget.Filterable;

import com.android.ex.chips.ChipsUtil.PermissionsCheckListener;
import com.android.ex.chips.DropdownChipLayouter.AdapterType;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Adapter for showing a recipient list.
 *
 * <p>It checks whether all permissions are granted before doing
 * query. If not all permissions in {@link ChipsUtil#REQUIRED_PERMISSIONS} are granted and
 * {@link #mShowRequestPermissionsItem} is true it will return single entry that asks user to grant
 * permissions to the app. Any app that uses this library should set this when it wants us to
 * display that entry but then it should set
 * {@link RecipientEditTextView.PermissionsRequestItemClickedListener} on
 * {@link RecipientEditTextView} as well.
 */
public class BaseRecipientAdapter extends BaseAdapter implements Filterable, AccountSpecifier,
        PhotoManager.PhotoManagerCallback {
    private static final String TAG = "BaseRecipientAdapter";

    private static final boolean DEBUG = false;

    /**
     * The preferred number of results to be retrieved. This number may be
     * exceeded if there are several directories configured, because we will use
     * the same limit for all directories.
     */
    private static final int DEFAULT_PREFERRED_MAX_RESULT_COUNT = 10;

    /**
     * The number of extra entries requested to allow for duplicates. Duplicates
     * are removed from the overall result.
     */
    static final int ALLOWANCE_FOR_DUPLICATES = 5;

    // This is ContactsContract.PRIMARY_ACCOUNT_NAME. Available from ICS as hidden
    static final String PRIMARY_ACCOUNT_NAME = "name_for_primary_account";
    // This is ContactsContract.PRIMARY_ACCOUNT_TYPE. Available from ICS as hidden
    static final String PRIMARY_ACCOUNT_TYPE = "type_for_primary_account";

    /**
     * The "Waiting for more contacts" message will be displayed if search is not complete
     * within this many milliseconds.
     */
    private static final int MESSAGE_SEARCH_PENDING_DELAY = 1000;
    /** Used to prepare "Waiting for more contacts" message. */
    private static final int MESSAGE_SEARCH_PENDING = 1;

    public static final int QUERY_TYPE_EMAIL = 0;
    public static final int QUERY_TYPE_PHONE = 1;

    private final Queries.Query mQueryMode;
    private final int mQueryType;

    /**
     * Model object for a {@link Directory} row.
     */
    public final static class DirectorySearchParams {
        public long directoryId;
        public String directoryType;
        public String displayName;
        public String accountName;
        public String accountType;
        public CharSequence constraint;
        public DirectoryFilter filter;
    }

    protected static class DirectoryListQuery {

        public static final Uri URI =
                Uri.withAppendedPath(ContactsContract.AUTHORITY_URI, "directories");
        public static final String[] PROJECTION = {
            Directory._ID,              // 0
            Directory.ACCOUNT_NAME,     // 1
            Directory.ACCOUNT_TYPE,     // 2
            Directory.DISPLAY_NAME,     // 3
            Directory.PACKAGE_NAME,     // 4
            Directory.TYPE_RESOURCE_ID, // 5
        };

        public static final int ID = 0;
        public static final int ACCOUNT_NAME = 1;
        public static final int ACCOUNT_TYPE = 2;
        public static final int DISPLAY_NAME = 3;
        public static final int PACKAGE_NAME = 4;
        public static final int TYPE_RESOURCE_ID = 5;
    }

    /** Used to temporarily hold results in Cursor objects. */
    protected static class TemporaryEntry {
        public final String displayName;
        public final String destination;
        public final int destinationType;
        public final String destinationLabel;
        public final long contactId;
        public final Long directoryId;
        public final long dataId;
        public final String thumbnailUriString;
        public final int displayNameSource;
        public final String lookupKey;

        public TemporaryEntry(
                String displayName,
                String destination,
                int destinationType,
                String destinationLabel,
                long contactId,
                Long directoryId,
                long dataId,
                String thumbnailUriString,
                int displayNameSource,
                String lookupKey) {
            this.displayName = displayName;
            this.destination = destination;
            this.destinationType = destinationType;
            this.destinationLabel = destinationLabel;
            this.contactId = contactId;
            this.directoryId = directoryId;
            this.dataId = dataId;
            this.thumbnailUriString = thumbnailUriString;
            this.displayNameSource = displayNameSource;
            this.lookupKey = lookupKey;
        }

        public TemporaryEntry(Cursor cursor, Long directoryId) {
            this.displayName = cursor.getString(Queries.Query.NAME);
            this.destination = cursor.getString(Queries.Query.DESTINATION);
            this.destinationType = cursor.getInt(Queries.Query.DESTINATION_TYPE);
            this.destinationLabel = cursor.getString(Queries.Query.DESTINATION_LABEL);
            this.contactId = cursor.getLong(Queries.Query.CONTACT_ID);
            this.directoryId = directoryId;
            this.dataId = cursor.getLong(Queries.Query.DATA_ID);
            this.thumbnailUriString = cursor.getString(Queries.Query.PHOTO_THUMBNAIL_URI);
            this.displayNameSource = cursor.getInt(Queries.Query.DISPLAY_NAME_SOURCE);
            this.lookupKey = cursor.getString(Queries.Query.LOOKUP_KEY);
        }
    }

    /**
     * Used to pass results from {@link DefaultFilter#performFiltering(CharSequence)} to
     * {@link DefaultFilter#publishResults(CharSequence, android.widget.Filter.FilterResults)}
     */
    private static class DefaultFilterResult {
        public final List<RecipientEntry> entries;
        public final LinkedHashMap<Long, List<RecipientEntry>> entryMap;
        public final List<RecipientEntry> nonAggregatedEntries;
        public final Set<String> existingDestinations;
        public final List<DirectorySearchParams> paramsList;

        public DefaultFilterResult(List<RecipientEntry> entries,
                LinkedHashMap<Long, List<RecipientEntry>> entryMap,
                List<RecipientEntry> nonAggregatedEntries,
                Set<String> existingDestinations,
                List<DirectorySearchParams> paramsList) {
            this.entries = entries;
            this.entryMap = entryMap;
            this.nonAggregatedEntries = nonAggregatedEntries;
            this.existingDestinations = existingDestinations;
            this.paramsList = paramsList;
        }

        private static DefaultFilterResult createResultWithNonAggregatedEntry(
                RecipientEntry entry) {
            return new DefaultFilterResult(
                    Collections.singletonList(entry),
                    new LinkedHashMap<Long, List<RecipientEntry>>() /* entryMap */,
                    Collections.singletonList(entry) /* nonAggregatedEntries */,
                    Collections.<String>emptySet() /* existingDestinations */,
                    null /* paramsList */);
        }
    }

    /**
     * An asynchronous filter used for loading two data sets: email rows from the local
     * contact provider and the list of {@link Directory}'s.
     */
    private final class DefaultFilter extends Filter {

        @Override
        protected FilterResults performFiltering(CharSequence constraint) {
            if (DEBUG) {
                Log.d(TAG, "start filtering. constraint: " + constraint + ", thread:"
                        + Thread.currentThread());
            }

            final FilterResults results = new FilterResults();

            if (TextUtils.isEmpty(constraint)) {
                clearTempEntries();
                // Return empty results.
                return results;
            }

            if (!ChipsUtil.hasPermissions(mContext, mPermissionsCheckListener)) {
                if (DEBUG) {
                    Log.d(TAG, "No Contacts permission. mShowRequestPermissionsItem: "
                            + mShowRequestPermissionsItem);
                }
                clearTempEntries();
                if (!mShowRequestPermissionsItem) {
                    // App doesn't want to show request permission entry. Returning empty results.
                    return results;
                }

                // Return result with only permission request entry.
                results.values = DefaultFilterResult.createResultWithNonAggregatedEntry(
                        RecipientEntry.constructPermissionEntry(ChipsUtil.REQUIRED_PERMISSIONS));
                results.count = 1;
                return results;
            }

            Cursor defaultDirectoryCursor = null;

            try {
                defaultDirectoryCursor = doQuery(constraint, mPreferredMaxResultCount,
                        null /* directoryId */);

                if (defaultDirectoryCursor == null) {
                    if (DEBUG) {
                        Log.w(TAG, "null cursor returned for default Email filter query.");
                    }
                } else {
                    // These variables will become mEntries, mEntryMap, mNonAggregatedEntries, and
                    // mExistingDestinations. Here we shouldn't use those member variables directly
                    // since this method is run outside the UI thread.
                    final LinkedHashMap<Long, List<RecipientEntry>> entryMap =
                            new LinkedHashMap<Long, List<RecipientEntry>>();
                    final List<RecipientEntry> nonAggregatedEntries =
                            new ArrayList<RecipientEntry>();
                    final Set<String> existingDestinations = new HashSet<String>();

                    while (defaultDirectoryCursor.moveToNext()) {
                        // Note: At this point each entry doesn't contain any photo
                        // (thus getPhotoBytes() returns null).
                        putOneEntry(new TemporaryEntry(defaultDirectoryCursor,
                                null /* directoryId */),
                                true, entryMap, nonAggregatedEntries, existingDestinations);
                    }

                    // We'll copy this result to mEntry in publicResults() (run in the UX thread).
                    final List<RecipientEntry> entries = constructEntryList(
                            entryMap, nonAggregatedEntries);

                    final List<DirectorySearchParams> paramsList =
                            searchOtherDirectories(existingDestinations);

                    results.values = new DefaultFilterResult(
                            entries, entryMap, nonAggregatedEntries,
                            existingDestinations, paramsList);
                    results.count = entries.size();
                }
            } finally {
                if (defaultDirectoryCursor != null) {
                    defaultDirectoryCursor.close();
                }
            }
            return results;
        }

        @Override
        protected void publishResults(final CharSequence constraint, FilterResults results) {
            mCurrentConstraint = constraint;

            clearTempEntries();

            if (results.values != null) {
                DefaultFilterResult defaultFilterResult = (DefaultFilterResult) results.values;
                mEntryMap = defaultFilterResult.entryMap;
                mNonAggregatedEntries = defaultFilterResult.nonAggregatedEntries;
                mExistingDestinations = defaultFilterResult.existingDestinations;

                cacheCurrentEntriesIfNeeded(defaultFilterResult.entries.size(),
                        defaultFilterResult.paramsList == null ? 0 :
                                defaultFilterResult.paramsList.size());

                updateEntries(defaultFilterResult.entries);

                // We need to search other remote directories, doing other Filter requests.
                if (defaultFilterResult.paramsList != null) {
                    final int limit = mPreferredMaxResultCount -
                            defaultFilterResult.existingDestinations.size();
                    startSearchOtherDirectories(constraint, defaultFilterResult.paramsList, limit);
                }
            } else {
                updateEntries(Collections.<RecipientEntry>emptyList());
            }
        }

        @Override
        public CharSequence convertResultToString(Object resultValue) {
            final RecipientEntry entry = (RecipientEntry)resultValue;
            final String displayName = entry.getDisplayName();
            final String emailAddress = entry.getDestination();
            if (TextUtils.isEmpty(displayName) || TextUtils.equals(displayName, emailAddress)) {
                 return emailAddress;
            } else {
                return new Rfc822Token(displayName, emailAddress, null).toString();
            }
        }
    }

    /**
     * Returns the list of models for directory search  (using {@link DirectoryFilter}) or
     * {@code null} when we don't need or can't search other directories.
     */
    protected List<DirectorySearchParams> searchOtherDirectories(Set<String> existingDestinations) {
        if (!ChipsUtil.hasPermissions(mContext, mPermissionsCheckListener)) {
            // If we don't have permissions we can't search other directories.
            if (DEBUG) {
                Log.d(TAG, "Not searching other directories because we don't have required "
                        + "permissions.");
            }
            return null;
        }

        // After having local results, check the size of results. If the results are
        // not enough, we search remote directories, which will take longer time.
        final int limit = mPreferredMaxResultCount - existingDestinations.size();
        if (limit > 0) {
            if (DEBUG) {
                Log.d(TAG, "More entries should be needed (current: "
                        + existingDestinations.size()
                        + ", remaining limit: " + limit + ") ");
            }
            Cursor directoryCursor = null;
            try {
                directoryCursor = mContentResolver.query(
                        DirectoryListQuery.URI, DirectoryListQuery.PROJECTION,
                        null, null, null);
                return setupOtherDirectories(mContext, directoryCursor, mAccount);
            } finally {
                if (directoryCursor != null) {
                    directoryCursor.close();
                }
            }
        } else {
            // We don't need to search other directories.
            return null;
        }
    }

    /**
     * An asynchronous filter that performs search in a particular directory.
     */
    protected class DirectoryFilter extends Filter {
        private final DirectorySearchParams mParams;
        private int mLimit;

        public DirectoryFilter(DirectorySearchParams params) {
            mParams = params;
        }

        public synchronized void setLimit(int limit) {
            this.mLimit = limit;
        }

        public synchronized int getLimit() {
            return this.mLimit;
        }

        @Override
        protected FilterResults performFiltering(CharSequence constraint) {
            if (DEBUG) {
                Log.d(TAG, "DirectoryFilter#performFiltering. directoryId: " + mParams.directoryId
                        + ", constraint: " + constraint + ", thread: " + Thread.currentThread());
            }
            final FilterResults results = new FilterResults();
            results.values = null;
            results.count = 0;

            if (!TextUtils.isEmpty(constraint)) {
                final ArrayList<TemporaryEntry> tempEntries = new ArrayList<TemporaryEntry>();

                Cursor cursor = null;
                try {
                    // We don't want to pass this Cursor object to UI thread (b/5017608).
                    // Assuming the result should contain fairly small results (at most ~10),
                    // We just copy everything to local structure.
                    cursor = doQuery(constraint, getLimit(), mParams.directoryId);

                    if (cursor != null) {
                        while (cursor.moveToNext()) {
                            tempEntries.add(new TemporaryEntry(cursor, mParams.directoryId));
                        }
                    }
                } finally {
                    if (cursor != null) {
                        cursor.close();
                    }
                }
                if (!tempEntries.isEmpty()) {
                    results.values = tempEntries;
                    results.count = tempEntries.size();
                }
            }

            if (DEBUG) {
                Log.v(TAG, "finished loading directory \"" + mParams.displayName + "\"" +
                        " with query " + constraint);
            }

            return results;
        }

        @Override
        protected void publishResults(final CharSequence constraint, FilterResults results) {
            if (DEBUG) {
                Log.d(TAG, "DirectoryFilter#publishResult. constraint: " + constraint
                        + ", mCurrentConstraint: " + mCurrentConstraint);
            }
            mDelayedMessageHandler.removeDelayedLoadMessage();
            // Check if the received result matches the current constraint
            // If not - the user must have continued typing after the request was issued, which
            // means several member variables (like mRemainingDirectoryLoad) are already
            // overwritten so shouldn't be touched here anymore.
            if (TextUtils.equals(constraint, mCurrentConstraint)) {
                if (results.count > 0) {
                    @SuppressWarnings("unchecked")
                    final ArrayList<TemporaryEntry> tempEntries =
                            (ArrayList<TemporaryEntry>) results.values;

                    for (TemporaryEntry tempEntry : tempEntries) {
                        putOneEntry(tempEntry, mParams.directoryId == Directory.DEFAULT);
                    }
                }

                // If there are remaining directories, set up delayed message again.
                mRemainingDirectoryCount--;
                if (mRemainingDirectoryCount > 0) {
                    if (DEBUG) {
                        Log.d(TAG, "Resend delayed load message. Current mRemainingDirectoryLoad: "
                                + mRemainingDirectoryCount);
                    }
                    mDelayedMessageHandler.sendDelayedLoadMessage();
                }

                // If this directory result has some items, or there are no more directories that
                // we are waiting for, clear the temp results
                if (results.count > 0 || mRemainingDirectoryCount == 0) {
                    // Clear the temp entries
                    clearTempEntries();
                }
            }

            // Show the list again without "waiting" message.
            updateEntries(constructEntryList());
        }
    }

    private final Context mContext;
    private final ContentResolver mContentResolver;
    private Account mAccount;
    protected final int mPreferredMaxResultCount;
    private DropdownChipLayouter mDropdownChipLayouter;

    /**
     * {@link #mEntries} is responsible for showing every result for this Adapter. To
     * construct it, we use {@link #mEntryMap}, {@link #mNonAggregatedEntries}, and
     * {@link #mExistingDestinations}.
     *
     * First, each destination (an email address or a phone number) with a valid contactId is
     * inserted into {@link #mEntryMap} and grouped by the contactId. Destinations without valid
     * contactId (possible if they aren't in local storage) are stored in
     * {@link #mNonAggregatedEntries}.
     * Duplicates are removed using {@link #mExistingDestinations}.
     *
     * After having all results from Cursor objects, all destinations in mEntryMap are copied to
     * {@link #mEntries}. If the number of destinations is not enough (i.e. less than
     * {@link #mPreferredMaxResultCount}), destinations in mNonAggregatedEntries are also used.
     *
     * These variables are only used in UI thread, thus should not be touched in
     * performFiltering() methods.
     */
    private LinkedHashMap<Long, List<RecipientEntry>> mEntryMap;
    private List<RecipientEntry> mNonAggregatedEntries;
    private Set<String> mExistingDestinations;
    /** Note: use {@link #updateEntries(List)} to update this variable. */
    private List<RecipientEntry> mEntries;
    private List<RecipientEntry> mTempEntries;

    /** The number of directories this adapter is waiting for results. */
    private int mRemainingDirectoryCount;

    /**
     * Used to ignore asynchronous queries with a different constraint, which may happen when
     * users type characters quickly.
     */
    protected CharSequence mCurrentConstraint;

    /**
     * Performs all photo querying as well as caching for repeated lookups.
     */
    private PhotoManager mPhotoManager;

    protected boolean mShowRequestPermissionsItem;

    private PermissionsCheckListener mPermissionsCheckListener;

    /**
     * Handler specific for maintaining "Waiting for more contacts" message, which will be shown
     * when:
     * - there are directories to be searched
     * - results from directories are slow to come
     */
    private final class DelayedMessageHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (mRemainingDirectoryCount > 0) {
                updateEntries(constructEntryList());
            }
        }

        public void sendDelayedLoadMessage() {
            sendMessageDelayed(obtainMessage(MESSAGE_SEARCH_PENDING, 0, 0, null),
                    MESSAGE_SEARCH_PENDING_DELAY);
        }

        public void removeDelayedLoadMessage() {
            removeMessages(MESSAGE_SEARCH_PENDING);
        }
    }

    private final DelayedMessageHandler mDelayedMessageHandler = new DelayedMessageHandler();

    private EntriesUpdatedObserver mEntriesUpdatedObserver;

    /**
     * Constructor for email queries.
     */
    public BaseRecipientAdapter(Context context) {
        this(context, DEFAULT_PREFERRED_MAX_RESULT_COUNT, QUERY_TYPE_EMAIL);
    }

    public BaseRecipientAdapter(Context context, int preferredMaxResultCount) {
        this(context, preferredMaxResultCount, QUERY_TYPE_EMAIL);
    }

    public BaseRecipientAdapter(int queryMode, Context context) {
        this(context, DEFAULT_PREFERRED_MAX_RESULT_COUNT, queryMode);
    }

    public BaseRecipientAdapter(int queryMode, Context context, int preferredMaxResultCount) {
        this(context, preferredMaxResultCount, queryMode);
    }

    public BaseRecipientAdapter(Context context, int preferredMaxResultCount, int queryMode) {
        mContext = context;
        mContentResolver = context.getContentResolver();
        mPreferredMaxResultCount = preferredMaxResultCount;
        mPhotoManager = new DefaultPhotoManager(mContentResolver);
        mQueryType = queryMode;

        if (queryMode == QUERY_TYPE_EMAIL) {
            mQueryMode = Queries.EMAIL;
        } else if (queryMode == QUERY_TYPE_PHONE) {
            mQueryMode = Queries.PHONE;
        } else {
            mQueryMode = Queries.EMAIL;
            Log.e(TAG, "Unsupported query type: " + queryMode);
        }
    }

    public Context getContext() {
        return mContext;
    }

    public int getQueryType() {
        return mQueryType;
    }

    public void setDropdownChipLayouter(DropdownChipLayouter dropdownChipLayouter) {
        mDropdownChipLayouter = dropdownChipLayouter;
        mDropdownChipLayouter.setQuery(mQueryMode);
    }

    public DropdownChipLayouter getDropdownChipLayouter() {
        return mDropdownChipLayouter;
    }

    public void setPermissionsCheckListener(PermissionsCheckListener permissionsCheckListener) {
        mPermissionsCheckListener = permissionsCheckListener;
    }

    @Nullable
    public PermissionsCheckListener getPermissionsCheckListener() {
        return mPermissionsCheckListener;
    }

    /**
     * Enables overriding the default photo manager that is used.
     */
    public void setPhotoManager(PhotoManager photoManager) {
        mPhotoManager = photoManager;
    }

    public PhotoManager getPhotoManager() {
        return mPhotoManager;
    }

    /**
     * If true, forces using the {@link com.android.ex.chips.SingleRecipientArrayAdapter}
     * instead of {@link com.android.ex.chips.RecipientAlternatesAdapter} when
     * clicking on a chip. Default implementation returns {@code false}.
     */
    public boolean forceShowAddress() {
        return false;
    }

    /**
     * Used to replace email addresses with chips. Default behavior
     * queries the ContactsProvider for contact information about the contact.
     * Derived classes should override this method if they wish to use a
     * new data source.
     * @param inAddresses addresses to query
     * @param callback callback to return results in case of success or failure
     */
    public void getMatchingRecipients(ArrayList<String> inAddresses,
            RecipientAlternatesAdapter.RecipientMatchCallback callback) {
        RecipientAlternatesAdapter.getMatchingRecipients(
                getContext(), this, inAddresses, getAccount(), callback, mPermissionsCheckListener);
    }

    /**
     * Set the account when known. Causes the search to prioritize contacts from that account.
     */
    @Override
    public void setAccount(Account account) {
        mAccount = account;
    }

    /**
     * Returns permissions that this adapter needs in order to provide results.
     */
    public String[] getRequiredPermissions() {
        return ChipsUtil.REQUIRED_PERMISSIONS;
    }

    /**
     * Sets whether to ask user to grant permission if they are missing.
     */
    public void setShowRequestPermissionsItem(boolean show) {
        mShowRequestPermissionsItem = show;
    }

    /** Will be called from {@link AutoCompleteTextView} to prepare auto-complete list. */
    @Override
    public Filter getFilter() {
        return new DefaultFilter();
    }

    /**
     * An extension to {@link RecipientAlternatesAdapter#getMatchingRecipients} that allows
     * additional sources of contacts to be considered as matching recipients.
     * @param addresses A set of addresses to be matched
     * @return A list of matches or null if none found
     */
    public Map<String, RecipientEntry> getMatchingRecipients(Set<String> addresses) {
        return null;
    }

    public static List<DirectorySearchParams> setupOtherDirectories(Context context,
            Cursor directoryCursor, Account account) {
        final PackageManager packageManager = context.getPackageManager();
        final List<DirectorySearchParams> paramsList = new ArrayList<DirectorySearchParams>();
        DirectorySearchParams preferredDirectory = null;
        while (directoryCursor.moveToNext()) {
            final long id = directoryCursor.getLong(DirectoryListQuery.ID);

            // Skip the local invisible directory, because the default directory already includes
            // all local results.
            if (id == Directory.LOCAL_INVISIBLE) {
                continue;
            }

            final DirectorySearchParams params = new DirectorySearchParams();
            final String packageName = directoryCursor.getString(DirectoryListQuery.PACKAGE_NAME);
            final int resourceId = directoryCursor.getInt(DirectoryListQuery.TYPE_RESOURCE_ID);
            params.directoryId = id;
            params.displayName = directoryCursor.getString(DirectoryListQuery.DISPLAY_NAME);
            params.accountName = directoryCursor.getString(DirectoryListQuery.ACCOUNT_NAME);
            params.accountType = directoryCursor.getString(DirectoryListQuery.ACCOUNT_TYPE);
            if (packageName != null && resourceId != 0) {
                try {
                    final Resources resources =
                            packageManager.getResourcesForApplication(packageName);
                    params.directoryType = resources.getString(resourceId);
                    if (params.directoryType == null) {
                        Log.e(TAG, "Cannot resolve directory name: "
                                + resourceId + "@" + packageName);
                    }
                } catch (NameNotFoundException e) {
                    Log.e(TAG, "Cannot resolve directory name: "
                            + resourceId + "@" + packageName, e);
                }
            }

            // If an account has been provided and we found a directory that
            // corresponds to that account, place that directory second, directly
            // underneath the local contacts.
            if (preferredDirectory == null && account != null
                    && account.name.equals(params.accountName)
                    && account.type.equals(params.accountType)) {
                preferredDirectory = params;
            } else {
                paramsList.add(params);
            }
        }

        if (preferredDirectory != null) {
            paramsList.add(1, preferredDirectory);
        }

        return paramsList;
    }

    /**
     * Starts search in other directories using {@link Filter}. Results will be handled in
     * {@link DirectoryFilter}.
     */
    protected void startSearchOtherDirectories(
            CharSequence constraint, List<DirectorySearchParams> paramsList, int limit) {
        final int count = paramsList.size();
        // Note: skipping the default partition (index 0), which has already been loaded
        for (int i = 1; i < count; i++) {
            final DirectorySearchParams params = paramsList.get(i);
            params.constraint = constraint;
            if (params.filter == null) {
                params.filter = new DirectoryFilter(params);
            }
            params.filter.setLimit(limit);
            params.filter.filter(constraint);
        }

        // Directory search started. We may show "waiting" message if directory results are slow
        // enough.
        mRemainingDirectoryCount = count - 1;
        mDelayedMessageHandler.sendDelayedLoadMessage();
    }

    /**
     * Called whenever {@link com.android.ex.chips.BaseRecipientAdapter.DirectoryFilter}
     * wants to add an additional entry to the results. Derived classes should override
     * this method if they are not using the default data structures provided by
     * {@link com.android.ex.chips.BaseRecipientAdapter} and are instead using their
     * own data structures to store and collate data.
     * @param entry the entry being added
     * @param isAggregatedEntry
     */
    protected void putOneEntry(TemporaryEntry entry, boolean isAggregatedEntry) {
        putOneEntry(entry, isAggregatedEntry,
                mEntryMap, mNonAggregatedEntries, mExistingDestinations);
    }

    private static void putOneEntry(TemporaryEntry entry, boolean isAggregatedEntry,
            LinkedHashMap<Long, List<RecipientEntry>> entryMap,
            List<RecipientEntry> nonAggregatedEntries,
            Set<String> existingDestinations) {
        if (existingDestinations.contains(entry.destination)) {
            return;
        }

        existingDestinations.add(entry.destination);

        if (!isAggregatedEntry) {
            nonAggregatedEntries.add(RecipientEntry.constructTopLevelEntry(
                    entry.displayName,
                    entry.displayNameSource,
                    entry.destination, entry.destinationType, entry.destinationLabel,
                    entry.contactId, entry.directoryId, entry.dataId, entry.thumbnailUriString,
                    true, entry.lookupKey));
        } else if (entryMap.containsKey(entry.contactId)) {
            // We already have a section for the person.
            final List<RecipientEntry> entryList = entryMap.get(entry.contactId);
            entryList.add(RecipientEntry.constructSecondLevelEntry(
                    entry.displayName,
                    entry.displayNameSource,
                    entry.destination, entry.destinationType, entry.destinationLabel,
                    entry.contactId, entry.directoryId, entry.dataId, entry.thumbnailUriString,
                    true, entry.lookupKey));
        } else {
            final List<RecipientEntry> entryList = new ArrayList<RecipientEntry>();
            entryList.add(RecipientEntry.constructTopLevelEntry(
                    entry.displayName,
                    entry.displayNameSource,
                    entry.destination, entry.destinationType, entry.destinationLabel,
                    entry.contactId, entry.directoryId, entry.dataId, entry.thumbnailUriString,
                    true, entry.lookupKey));
            entryMap.put(entry.contactId, entryList);
        }
    }

    /**
     * Returns the actual list to use for this Adapter. Derived classes
     * should override this method if overriding how the adapter stores and collates
     * data.
     */
    protected List<RecipientEntry> constructEntryList() {
        return constructEntryList(mEntryMap, mNonAggregatedEntries);
    }

    /**
     * Constructs an actual list for this Adapter using {@link #mEntryMap}. Also tries to
     * fetch a cached photo for each contact entry (other than separators), or request another
     * thread to get one from directories.
     */
    private List<RecipientEntry> constructEntryList(
            LinkedHashMap<Long, List<RecipientEntry>> entryMap,
            List<RecipientEntry> nonAggregatedEntries) {
        final List<RecipientEntry> entries = new ArrayList<RecipientEntry>();
        int validEntryCount = 0;
        for (Map.Entry<Long, List<RecipientEntry>> mapEntry : entryMap.entrySet()) {
            final List<RecipientEntry> entryList = mapEntry.getValue();
            final int size = entryList.size();
            for (int i = 0; i < size; i++) {
                RecipientEntry entry = entryList.get(i);
                entries.add(entry);
                mPhotoManager.populatePhotoBytesAsync(entry, this);
                validEntryCount++;
            }
            if (validEntryCount > mPreferredMaxResultCount) {
                break;
            }
        }
        if (validEntryCount <= mPreferredMaxResultCount) {
            for (RecipientEntry entry : nonAggregatedEntries) {
                if (validEntryCount > mPreferredMaxResultCount) {
                    break;
                }
                entries.add(entry);
                mPhotoManager.populatePhotoBytesAsync(entry, this);
                validEntryCount++;
            }
        }

        return entries;
    }


    public interface EntriesUpdatedObserver {
        public void onChanged(List<RecipientEntry> entries);
    }

    public void registerUpdateObserver(EntriesUpdatedObserver observer) {
        mEntriesUpdatedObserver = observer;
    }

    /** Resets {@link #mEntries} and notify the event to its parent ListView. */
    protected void updateEntries(List<RecipientEntry> newEntries) {
        mEntries = newEntries;
        mEntriesUpdatedObserver.onChanged(newEntries);
        notifyDataSetChanged();
    }

    /**
     * If there are no local results and we are searching alternate results,
     * in the new result set, cache off what had been shown to the user for use until
     * the first directory result is returned
     * @param newEntryCount number of newly loaded entries
     * @param paramListCount number of alternate filters it will search (including the current one).
     */
    protected void cacheCurrentEntriesIfNeeded(int newEntryCount, int paramListCount) {
        if (newEntryCount == 0 && paramListCount > 1) {
            cacheCurrentEntries();
        }
    }

    protected void cacheCurrentEntries() {
        mTempEntries = mEntries;
    }

    protected void clearTempEntries() {
        mTempEntries = null;
    }

    protected List<RecipientEntry> getEntries() {
        return mTempEntries != null ? mTempEntries : mEntries;
    }

    protected void fetchPhoto(final RecipientEntry entry, PhotoManager.PhotoManagerCallback cb) {
        mPhotoManager.populatePhotoBytesAsync(entry, cb);
    }

    private Cursor doQuery(CharSequence constraint, int limit, Long directoryId) {
        if (!ChipsUtil.hasPermissions(mContext, mPermissionsCheckListener)) {
            if (DEBUG) {
                Log.d(TAG, "Not doing query because we don't have required permissions.");
            }
            return null;
        }

        final Uri.Builder builder = mQueryMode.getContentFilterUri().buildUpon()
                .appendPath(constraint.toString())
                .appendQueryParameter(ContactsContract.LIMIT_PARAM_KEY,
                        String.valueOf(limit + ALLOWANCE_FOR_DUPLICATES));
        if (directoryId != null) {
            builder.appendQueryParameter(ContactsContract.DIRECTORY_PARAM_KEY,
                    String.valueOf(directoryId));
        }
        if (mAccount != null) {
            builder.appendQueryParameter(PRIMARY_ACCOUNT_NAME, mAccount.name);
            builder.appendQueryParameter(PRIMARY_ACCOUNT_TYPE, mAccount.type);
        }
        final long start = System.currentTimeMillis();
        final Cursor cursor = mContentResolver.query(
                builder.build(), mQueryMode.getProjection(), null, null, null);
        final long end = System.currentTimeMillis();
        if (DEBUG) {
            Log.d(TAG, "Time for autocomplete (query: " + constraint
                    + ", directoryId: " + directoryId + ", num_of_results: "
                    + (cursor != null ? cursor.getCount() : "null") + "): "
                    + (end - start) + " ms");
        }
        return cursor;
    }

    // TODO: This won't be used at all. We should find better way to quit the thread..
    /*public void close() {
        mEntries = null;
        mPhotoCacheMap.evictAll();
        if (!sPhotoHandlerThread.quit()) {
            Log.w(TAG, "Failed to quit photo handler thread, ignoring it.");
        }
    }*/

    @Override
    public int getCount() {
        final List<RecipientEntry> entries = getEntries();
        return entries != null ? entries.size() : 0;
    }

    @Override
    public RecipientEntry getItem(int position) {
        return getEntries().get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public int getViewTypeCount() {
        return RecipientEntry.ENTRY_TYPE_SIZE;
    }

    @Override
    public int getItemViewType(int position) {
        return getEntries().get(position).getEntryType();
    }

    @Override
    public boolean isEnabled(int position) {
        return getEntries().get(position).isSelectable();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final RecipientEntry entry = getEntries().get(position);

        final String constraint = mCurrentConstraint == null ? null :
                mCurrentConstraint.toString();

        return mDropdownChipLayouter.bindView(convertView, parent, entry, position,
                AdapterType.BASE_RECIPIENT, constraint);
    }

    public Account getAccount() {
        return mAccount;
    }

    @Override
    public void onPhotoBytesPopulated() {
        // Default implementation does nothing
    }

    @Override
    public void onPhotoBytesAsynchronouslyPopulated() {
        notifyDataSetChanged();
    }

    @Override
    public void onPhotoBytesAsyncLoadFailed() {
        // Default implementation does nothing
    }
}
