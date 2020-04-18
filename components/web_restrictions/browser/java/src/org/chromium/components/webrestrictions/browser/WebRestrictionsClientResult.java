// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.webrestrictions.browser;

import android.database.Cursor;

import org.chromium.base.annotations.CalledByNative;

/**
 * Result of a WebRestrictions should proceed request on the client size
 */
public class WebRestrictionsClientResult {
    private final Cursor mCursor;

    public WebRestrictionsClientResult(Cursor cursor) {
        mCursor = cursor;
        if (cursor == null) return;
        cursor.moveToFirst();
    }

    /**
     * @return true if it is ok to access the URL
     */
    @CalledByNative
    public boolean shouldProceed() {
        if (mCursor == null) return false;
        return mCursor.getInt(0) > 0;
    }

    /**
     * Get an integer element of the custom error data
     * @param column the column number
     * @return the value
     */
    @CalledByNative
    public int getInt(int column) {
        if (mCursor == null) return 0;
        return mCursor.getInt(column);
    }

    /**
     * Get a string element of the custom error data
     * @param column column number
     * @return the value
     */
    @CalledByNative
    public String getString(int column) {
        if (mCursor == null) return null;
        return mCursor.getString(column);
    }

    /**
     * Get the name of a column
     * @param column column number
     * @return the value
     */
    @CalledByNative
    public String getColumnName(int column) {
        if (mCursor == null) return null;
        return mCursor.getColumnName(column);
    }

    /**
     * @return the number of columns
     */
    @CalledByNative
    public int getColumnCount() {
        if (mCursor == null) return 0;
        return mCursor.getColumnCount();
    }

    /**
     * @param column column number
     * @return whether the column is a string column
     * @note all columns are either strings or ints.
     */
    @CalledByNative
    public boolean isString(int column) {
        if (mCursor == null) return false;
        return mCursor.getType(column) == Cursor.FIELD_TYPE_STRING;
    }
}