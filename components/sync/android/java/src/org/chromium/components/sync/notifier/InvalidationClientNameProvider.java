// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync.notifier;

/**
 * An injectable singleton that provides an invalidation client with an appropriate unique name.
 *
 * This singleton will always provide a somewhat reasonable name.  With proper support from outside
 * components, it will be able to provide a name that is consistent across restarts.
 */
public class InvalidationClientNameProvider {
    private static final Object LOCK = new Object();

    private static InvalidationClientNameProvider sInstance;

    private final Object mLock;

    private InvalidationClientNameGenerator mGenerator;

    private byte[] mUniqueId;

    public static InvalidationClientNameProvider get() {
        synchronized (LOCK) {
            if (sInstance == null) {
                sInstance = new InvalidationClientNameProvider();
            }
            return sInstance;
        }
    }

    InvalidationClientNameProvider() {
        mLock = new Object();
        mGenerator = new RandomizedInvalidationClientNameGenerator();
    }

    /** Returns a consistent unique string of bytes for use as an invalidator client ID. */
    public byte[] getInvalidatorClientName() {
        synchronized (mLock) {
            if (mUniqueId == null) {
                mUniqueId = mGenerator.generateInvalidatorClientName();
            }
            return mUniqueId;
        }
    }

    public void setPreferredClientNameGenerator(InvalidationClientNameGenerator generator) {
        mGenerator = generator;
    }
}
