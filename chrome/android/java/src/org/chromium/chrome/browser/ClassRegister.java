// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

/**
 * Base class for defining methods where different behavior is required by downstream targets.
 * The difference to AppHooks is we need to upstream changes here later.
 */
public abstract class ClassRegister {
    private static ClassRegisterImpl sInstance;

    public static ClassRegister get() {
        if (sInstance == null) {
            sInstance = new ClassRegisterImpl();
        }
        return sInstance;
    }

    /**
     * Register the {@link ContentClassFactory} so {@link SelectionInsertionHandleObserver} can be
     * set properly.
     */
    public void registerContentClassFactory() {
        /* no-op */
    }
}
