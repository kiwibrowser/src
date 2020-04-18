// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync.notifier;

import android.util.Base64;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.MainDex;

import java.util.Random;

/**
 * Generates a fully random client ID.
 *
 * This ID will not persist across restarts.  Using this ID will break the invalidator's "reflection
 * blocking" feature.  That's unfortunate, but better than using a hard-coded ID.  A hard-coded ID
 * could prevent invalidations from being delivered.
 */
@MainDex
class RandomizedInvalidationClientNameGenerator implements InvalidationClientNameGenerator {
    private static final Random RANDOM = new Random();

    RandomizedInvalidationClientNameGenerator() {}

    /**
     * Generates a random ID prefixed with the string "BadID".
     *
     * The prefix is intended to grab attention.  We should never use it in real builds.  Hopefully,
     * it will induce someone to file a bug if they see it.
     *
     * However, as bad as it is, this ID is better than a hard-coded default or none at all.  See
     * the class description for more details.
     */
    @Override
    public byte[] generateInvalidatorClientName() {
        byte[] randomBytes = new byte[8];
        RANDOM.nextBytes(randomBytes);
        String encoded = Base64.encodeToString(randomBytes, 0, randomBytes.length, Base64.NO_WRAP);
        String idString = "BadID" + encoded;
        return ApiCompatibilityUtils.getBytesUtf8(idString);
    }
}
