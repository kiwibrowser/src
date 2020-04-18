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

import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentResolver;
import android.content.OperationApplicationException;
import android.net.Uri;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.util.Log;

import java.util.ArrayList;

/**
 * <P>
 * {@link VCardEntryHandler} implementation which commits the entry to ContentResolver.
 * </P>
 * <P>
 * Note:<BR />
 * Each vCard may contain big photo images encoded by BASE64,
 * If we store all vCard entries in memory, OutOfMemoryError may be thrown.
 * Thus, this class push each VCard entry into ContentResolver immediately.
 * </P>
 */
public class VCardEntryCommitter implements VCardEntryHandler {
    public static String LOG_TAG = VCardConstants.LOG_TAG;

    private final ContentResolver mContentResolver;
    private long mTimeToCommit;
    private int mCounter;
    private ArrayList<ContentProviderOperation> mOperationList;
    private final ArrayList<Uri> mCreatedUris = new ArrayList<Uri>();

    public VCardEntryCommitter(ContentResolver resolver) {
        mContentResolver = resolver;
    }

    @Override
    public void onStart() {
    }

    @Override
    public void onEnd() {
        if (mOperationList != null) {
            mCreatedUris.add(pushIntoContentResolver(mOperationList));
        }

        if (VCardConfig.showPerformanceLog()) {
            Log.d(LOG_TAG, String.format("time to commit entries: %d ms", mTimeToCommit));
        }
    }

    @Override
    public void onEntryCreated(final VCardEntry vcardEntry) {
        final long start = System.currentTimeMillis();
        mOperationList = vcardEntry.constructInsertOperations(mContentResolver, mOperationList);
        mCounter++;
        if (mCounter >= 20) {
            mCreatedUris.add(pushIntoContentResolver(mOperationList));
            mCounter = 0;
            mOperationList = null;
        }
        mTimeToCommit += System.currentTimeMillis() - start;
    }

    private Uri pushIntoContentResolver(ArrayList<ContentProviderOperation> operationList) {
        try {
            final ContentProviderResult[] results = mContentResolver.applyBatch(
                    ContactsContract.AUTHORITY, operationList);

            // the first result is always the raw_contact. return it's uri so
            // that it can be found later. do null checking for badly behaving
            // ContentResolvers
            return ((results == null || results.length == 0 || results[0] == null)
                            ? null : results[0].uri);
        } catch (RemoteException e) {
            Log.e(LOG_TAG, String.format("%s: %s", e.toString(), e.getMessage()));
            return null;
        } catch (OperationApplicationException e) {
            Log.e(LOG_TAG, String.format("%s: %s", e.toString(), e.getMessage()));
            return null;
        }
    }

    /**
     * Returns the list of created Uris. This list should not be modified by the caller as it is
     * not a clone.
     */
   public ArrayList<Uri> getCreatedUris() {
        return mCreatedUris;
    }
}