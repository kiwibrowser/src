// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.invalidation;

import android.accounts.Account;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;

import com.google.ipc.invalidation.external.client.types.ObjectId;

import org.chromium.base.CollectionUtil;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Subclass of {@link InvalidationClientService} that captures events and allows controlling
 * whether or not Chrome is in the foreground and sync is enabled.
 *
 * @author dsmyers@google.com (Daniel Myers)
 */
public class TestableInvalidationClientService extends InvalidationClientService {
    /** Object ids given to {@link #register}, one list element per call. */
    final List<List<ObjectId>> mRegistrations = new ArrayList<List<ObjectId>>();

    /** Object ids given to {@link #unregister}, one list element per call. */
    final List<List<ObjectId>> mUnregistrations = new ArrayList<List<ObjectId>>();

    /**
     * Current registered based on the cumulative calls to {@link #register} and
     * {@link #unregister}.
     */
    final Set<ObjectId> mCurrentRegistrations = new HashSet<ObjectId>();

    /** Intents given to {@link #startService}. */
    final List<Intent> mStartedServices = new ArrayList<Intent>();

    /** Bundles given to {@link #requestSyncFromContentResolver}. */
    final List<Bundle> mRequestedSyncs = new ArrayList<Bundle>();

    final List<byte[]> mAcknowledgements = new ArrayList<byte[]>();

    /** Whether Chrome is in the foreground. */
    private boolean mIsChromeInForeground;

    /** Whether sync is enabled. */
    private boolean mIsSyncEnabled;

    @Override
    public void acknowledge(byte[] ackHandle) {
        mAcknowledgements.add(ackHandle);
    }

    @Override
    public void register(byte[] clientId, Iterable<ObjectId> objectIds) {
        List<ObjectId> objectIdList = CollectionUtil.newArrayList(objectIds);
        mRegistrations.add(objectIdList);
        mCurrentRegistrations.addAll(objectIdList);
        super.register(clientId, objectIds);
    }

    @Override
    public void unregister(byte[] clientId, Iterable<ObjectId> objectIds) {
        List<ObjectId> objectIdList = CollectionUtil.newArrayList(objectIds);
        mUnregistrations.add(objectIdList);
        mCurrentRegistrations.removeAll(objectIdList);
        super.unregister(clientId, objectIds);
    }

    @Override
    public ComponentName startService(Intent intent) {
        mStartedServices.add(intent);
        return super.startService(intent);
    }

    @Override
    public void requestSyncFromContentResolver(Bundle bundle, Account account,
            String contractAuthority) {
        mRequestedSyncs.add(bundle);
        super.requestSyncFromContentResolver(bundle, account, contractAuthority);
    }

    @Override
    boolean isChromeInForeground() {
        return mIsChromeInForeground;
    }

    @Override
    boolean isSyncEnabled() {
        return mIsSyncEnabled;
    }

    /**
     * Sets the variables used to control whether or not a notification client should be running.
     * @param isChromeInForeground whether Chrome is in the foreground
     * @param isSyncEnabled whether sync is enabled
     */
    void setShouldRunStates(boolean isChromeInForeground, boolean isSyncEnabled) {
        this.mIsChromeInForeground = isChromeInForeground;
        this.mIsSyncEnabled = isSyncEnabled;
    }
}
