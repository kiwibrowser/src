// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.webrestrictions.browser;

import android.annotation.TargetApi;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.content.pm.ProviderInfo;
import android.database.AbstractCursor;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Abstract content provider for providing web restrictions, i.e. for providing a filter for URLs so
 * that they can be blocked or permitted, and a means of requesting permission for new URLs. It
 * provides two (virtual) tables; an 'authorized' table listing the the status of every URL, and a
 * 'requested' table containing the requests for access to new URLs. The 'authorized' table is read
 * only and the 'requested' table is write only.
 */
public abstract class WebRestrictionsContentProvider extends ContentProvider {
    public static final int BLOCKED = 0;
    public static final int PROCEED = 1;
    private static final int WEB_RESTRICTIONS = 1;
    private static final int AUTHORIZED = 2;
    private static final int REQUESTED = 3;

    private final Pattern mSelectionPattern;
    private UriMatcher mContentUriMatcher;
    private Uri mContentUri;

    /**
     * Structure for returning result including the custom error data.
     */
    public static class WebRestrictionsResult {
        private final boolean mShouldProceed;
        private final int mErrorInts[];
        private final String mErrorStrings[];

        public WebRestrictionsResult(
                boolean shouldProceed, final int[] errorInts, final String[] errorStrings) {
            assert !shouldProceed || errorInts == null;
            assert !shouldProceed || errorStrings == null;
            mShouldProceed = shouldProceed;
            mErrorInts = errorInts == null ? null : errorInts.clone();
            mErrorStrings = errorStrings == null ? null : errorStrings.clone();
        }

        public int getErrorInt(int i) {
            if (mErrorInts == null || i >= mErrorInts.length) return 0;
            return mErrorInts[i];
        }

        public String getErrorString(int i) {
            if (mErrorStrings == null || i >= mErrorStrings.length) return null;
            return mErrorStrings[i];
        }

        public boolean shouldProceed() {
            return mShouldProceed;
        }

        public int errorIntCount() {
            if (mErrorInts == null) return 0;
            return mErrorInts.length;
        }

        public int errorStringCount() {
            if (mErrorStrings == null) return 0;
            return mErrorStrings.length;
        }
    }

    protected WebRestrictionsContentProvider() {
        // Pattern to extract the URL from the selection.
        // Matches patterns of the form "url = '<url>'" with arbitrary spacing
        // around the "=" etc.
        mSelectionPattern = Pattern.compile("\\s*url\\s*=\\s*'([^']*)'");
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public void attachInfo(Context context, ProviderInfo info) {
        super.attachInfo(context, info);
        mContentUri = new Uri.Builder().scheme("content").authority(info.authority).build();
        mContentUriMatcher = new UriMatcher(WEB_RESTRICTIONS);
        mContentUriMatcher.addURI(info.authority, "authorized", AUTHORIZED);
        mContentUriMatcher.addURI(info.authority, "requested", REQUESTED);
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        if (!contentProviderEnabled()) return null;
        // Check that this is the a query on the 'authorized' table
        // TODO(aberent): Provide useful queries on the 'requested' table.
        if (mContentUriMatcher.match(uri) != AUTHORIZED) return null;
        // If the selection is of the right form get the url we are querying.
        Matcher matcher = mSelectionPattern.matcher(selection);
        if (!matcher.find()) return null;
        final String url = matcher.group(1);
        final WebRestrictionsResult result = shouldProceed(maybeGetCallingPackage(), url);
        if (result == null) return null;

        return new AbstractCursor() {

            @Override
            public int getCount() {
                return 1;
            }

            @Override
            public String[] getColumnNames() {
                String errorNames[] = getErrorColumnNames();
                // The cursor in the client gets the column count from the number of column names
                // so it is important to limit this array to the actual number of columns.
                String names[] = new String[getColumnCount()];
                names[0] = "Should Proceed";
                for (int i = 0; i < getColumnCount() - 1; i++) {
                    names[i + 1] = errorNames[i];
                }
                return names;
            }

            @Override
            public String getString(int column) {
                // The column order is:
                //    result,
                //    integer error parameters,
                //    string error parameters
                // so offset the string error parameters by the number of integer parameters + 1
                int errorStringNumber = column - result.errorIntCount() - 1;
                if (errorStringNumber >= 0 && errorStringNumber < result.errorStringCount()) {
                    return result.getErrorString(errorStringNumber);
                }
                return null;
            }

            @Override
            public short getShort(int column) {
                return (short) getLong(column);
            }

            @Override
            public int getInt(int column) {
                return (int) getLong(column);
            }

            @Override
            public long getLong(int column) {
                if (column == 0) return result.shouldProceed() ? PROCEED : BLOCKED;
                // The column order is:
                //    result,
                //    integer error parameters,
                //    string error parameters
                // so offset the integer error parameters by 1
                int errorIntNumber = column - 1;
                if (errorIntNumber < result.errorIntCount()) {
                    return result.getErrorInt(errorIntNumber);
                }
                return 0;
            }

            @Override
            public float getFloat(int column) {
                return 0;
            }

            @Override
            public double getDouble(int column) {
                return 0;
            }

            @Override
            public boolean isNull(int column) {
                return false;
            }

            @Override
            public int getType(int column) {
                if (column < result.errorIntCount() + 1) return FIELD_TYPE_INTEGER;
                if (column < result.errorIntCount() + result.errorStringCount() + 1) {
                    return FIELD_TYPE_STRING;
                }
                return FIELD_TYPE_NULL;
            }

            @Override
            public int getColumnCount() {
                return result.errorIntCount() + result.errorStringCount() + 1;
            }
        };
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    private String maybeGetCallingPackage() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) return null;

        return getCallingPackage();
    }

    @Override
    public String getType(Uri uri) {
        // Abused to return whether we can insert
        if (!contentProviderEnabled()) return null;
        if (mContentUriMatcher.match(uri) != REQUESTED) return null;
        return canInsert() ? "text/plain" : null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        if (!contentProviderEnabled()) return null;
        if (mContentUriMatcher.match(uri) != REQUESTED) return null;
        String url = values.getAsString("url");
        if (requestInsert(url)) {
            // TODO(aberent): If we ever make the 'requested' table readable then we might want to
            // change this to a more conventional content URI (with a row number).
            return uri.buildUpon().appendPath(url).build();
        } else {
            return null;
        }
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        return 0;
    }

    /**
     * @param the package calling the content provider, or null if the package is not available.
     * @param url the URL that is wanted.
     * @return a pair containing the Result and the HTML Error Message. result is true if safe to
     *         proceed, false otherwise. error message is only meaningful if result is false, a null
     *         error message means use application default.
     */
    protected abstract WebRestrictionsResult shouldProceed(String callingPackage, String url);

    /**
     * @return whether the content provider allows insertions.
     */
    protected abstract boolean canInsert();

    /**
     * @return the names of the custom error columns, integer valued columns must proceed string
     * valued columns.
     */
    protected abstract String[] getErrorColumnNames();

    /**
     * Start a request that a URL should be permitted
     *
     * @param url the URL that is wanted.
     */
    protected abstract boolean requestInsert(final String url);

    /**
     * @return true if the content provider is enabled, false if not
     */
    protected abstract boolean contentProviderEnabled();

    /**
     * Call to tell observers that the filter has changed.
     */
    protected void onFilterChanged() {
        getContext().getContentResolver().notifyChange(
                mContentUri.buildUpon().appendPath("authorized").build(), null);
    }
}
