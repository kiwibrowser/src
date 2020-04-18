/*
 * Copyright (C) 2011 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.ex.photo.adapters;

import android.content.Context;
import android.database.Cursor;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.View;

import com.android.ex.photo.provider.PhotoContract;

import java.util.HashMap;

/**
 * Page adapter for use with an BaseCursorLoader. Unlike other cursor adapters, this has no
 * observers for automatic refresh. Instead, it depends upon external mechanisms to provide
 * the update signal.
 */
public abstract class BaseCursorPagerAdapter extends BaseFragmentPagerAdapter {
    private static final String TAG = "BaseCursorPagerAdapter";

    protected Context mContext;
    protected Cursor mCursor;
    protected int mRowIDColumn;
    /** Mapping of row ID to cursor position */
    protected SparseIntArray mItemPosition;
    /** Mapping of instantiated object to row ID */
    protected final HashMap<Object, Integer> mObjectRowMap = new HashMap<Object, Integer>();

    /**
     * Constructor that always enables auto-requery.
     *
     * @param c The cursor from which to get the data.
     * @param context The context
     */
    public BaseCursorPagerAdapter(Context context, FragmentManager fm, Cursor c) {
        super(fm);
        init(context, c);
    }

    /**
     * Makes a fragment for the data pointed to by the cursor
     *
     * @param context Interface to application's global information
     * @param cursor The cursor from which to get the data. The cursor is already
     * moved to the correct position.
     * @return the newly created fragment.
     */
    public abstract Fragment getItem(Context context, Cursor cursor, int position);

    // TODO: This shouldn't just return null - maybe it needs to wait for a cursor to be supplied?
    //       See b/7103023
    @Override
    public Fragment getItem(int position) {
        if (mCursor != null && moveCursorTo(position)) {
            return getItem(mContext, mCursor, position);
        }
        return null;
    }

    @Override
    public int getCount() {
        if (mCursor != null) {
            return mCursor.getCount();
        } else {
            return 0;
        }
    }

    @Override
    public Object instantiateItem(View container, int position) {
        if (mCursor == null) {
            throw new IllegalStateException("this should only be called when the cursor is valid");
        }

        final Integer rowId;
        if (moveCursorTo(position)) {
            rowId = mCursor.getString(mRowIDColumn).hashCode();
        } else {
            rowId = null;
        }

        // Create the fragment and bind cursor data
        final Object obj = super.instantiateItem(container, position);
        if (obj != null) {
            mObjectRowMap.put(obj, rowId);
        }
        return obj;
    }

    @Override
    public void destroyItem(View container, int position, Object object) {
        mObjectRowMap.remove(object);

        super.destroyItem(container, position, object);
    }

    @Override
    public int getItemPosition(Object object) {
        final Integer rowId = mObjectRowMap.get(object);
        if (rowId == null || mItemPosition == null) {
            return POSITION_NONE;
        }

        final int position = mItemPosition.get(rowId, POSITION_NONE);
        return position;
    }

    /**
     * @return true if data is valid
     */
    public boolean isDataValid() {
        return mCursor != null;
    }

    /**
     * Returns the cursor.
     */
    public Cursor getCursor() {
        return mCursor;
    }

    /**
     * Returns the data item associated with the specified position in the data set.
     */
    public Object getDataItem(int position) {
        if (mCursor != null && moveCursorTo(position)) {
            return mCursor;
        } else {
            return null;
        }
    }

    /**
     * Returns the row id associated with the specified position in the list.
     */
    public long getItemId(int position) {
        if (mCursor != null && moveCursorTo(position)) {
            return mCursor.getString(mRowIDColumn).hashCode();
        } else {
            return 0;
        }
    }

    /**
     * Swap in a new Cursor, returning the old Cursor.
     *
     * @param newCursor The new cursor to be used.
     * @return Returns the previously set Cursor, or null if there was not one.
     * If the given new Cursor is the same instance is the previously set
     * Cursor, null is also returned.
     */
    public Cursor swapCursor(Cursor newCursor) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "swapCursor old=" + (mCursor == null ? -1 : mCursor.getCount()) +
                    "; new=" + (newCursor == null ? -1 : newCursor.getCount()));
        }

        if (newCursor == mCursor) {
            return null;
        }
        Cursor oldCursor = mCursor;
        mCursor = newCursor;
        if (newCursor != null) {
            mRowIDColumn = newCursor.getColumnIndex(PhotoContract.PhotoViewColumns.URI);
        } else {
            mRowIDColumn = -1;
        }

        setItemPosition();
        notifyDataSetChanged();     // notify the observers about the new cursor
        return oldCursor;
    }

    /**
     * Converts the cursor into a CharSequence. Subclasses should override this
     * method to convert their results. The default implementation returns an
     * empty String for null values or the default String representation of
     * the value.
     *
     * @param cursor the cursor to convert to a CharSequence
     * @return a CharSequence representing the value
     */
    public CharSequence convertToString(Cursor cursor) {
        return cursor == null ? "" : cursor.toString();
    }

    @Override
    protected String makeFragmentName(int viewId, int index) {
        if (moveCursorTo(index)) {
            return "android:pager:" + viewId + ":" + mCursor.getString(mRowIDColumn).hashCode();
        } else {
            return super.makeFragmentName(viewId, index);
        }
    }

    /**
     * Moves the cursor to the given position
     *
     * @return {@code true} if the cursor's position was set. Otherwise, {@code false}.
     */
    private boolean moveCursorTo(int position) {
        if (mCursor != null && !mCursor.isClosed()) {
            return mCursor.moveToPosition(position);
        }
        return false;
    }

    /**
     * Initialize the adapter.
     */
    private void init(Context context, Cursor c) {
        boolean cursorPresent = c != null;
        mCursor = c;
        mContext = context;
        mRowIDColumn = cursorPresent
                ? mCursor.getColumnIndex(PhotoContract.PhotoViewColumns.URI) : -1;
    }

    /**
     * Sets the {@link #mItemPosition} instance variable with the current mapping of
     * row id to cursor position.
     */
    private void setItemPosition() {
        if (mCursor == null || mCursor.isClosed()) {
            mItemPosition = null;
            return;
        }

        SparseIntArray itemPosition = new SparseIntArray(mCursor.getCount());

        mCursor.moveToPosition(-1);
        while (mCursor.moveToNext()) {
            final int rowId = mCursor.getString(mRowIDColumn).hashCode();
            final int position = mCursor.getPosition();

            itemPosition.append(rowId, position);
        }
        mItemPosition = itemPosition;
    }
}
