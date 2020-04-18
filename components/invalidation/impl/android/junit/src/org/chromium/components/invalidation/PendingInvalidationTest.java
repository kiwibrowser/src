// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.invalidation;

import static org.junit.Assert.assertEquals;

import android.os.Bundle;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Tests for {@link PendingInvalidation}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class PendingInvalidationTest {
    @Test
    public void testFullData() {
        String objectId = "ObjectId";
        int objectSource = 4;
        long version = 5;
        String payload = "Payload";
        doTestParseFromBundle(objectId, objectSource, version, payload);
        doTestParseToAndFromProtocolBuffer(objectId, objectSource, version, payload);
        doTestParseToAndFromProtocolBufferThroughBundle(objectId, objectSource, version, payload);
    }

    @Test
    public void testNoData() {
        String objectId = null;
        int objectSource = 4;
        long version = 0L;
        String payload = null;
        doTestParseFromBundle(objectId, objectSource, version, payload);
        doTestParseToAndFromProtocolBuffer(objectId, objectSource, version, payload);
        doTestParseToAndFromProtocolBufferThroughBundle(objectId, objectSource, version, payload);
    }

    public void doTestParseFromBundle(
            String objectId, int objectSource, long version, String payload) {
        PendingInvalidation invalidation =
                new PendingInvalidation(objectId, objectSource, version, payload);
        Bundle bundle = PendingInvalidation.createBundle(objectId, objectSource, version, payload);
        PendingInvalidation parsedInvalidation = new PendingInvalidation(bundle);
        assertEquals(objectId, parsedInvalidation.mObjectId);
        assertEquals(objectSource, parsedInvalidation.mObjectSource);
        assertEquals(version, parsedInvalidation.mVersion);
        assertEquals(payload, parsedInvalidation.mPayload);
        assertEquals(invalidation, parsedInvalidation);
    }

    public void doTestParseToAndFromProtocolBuffer(
            String objectId, int objectSource, long version, String payload) {
        PendingInvalidation invalidation =
                new PendingInvalidation(objectId, objectSource, version, payload);
        PendingInvalidation parsedInvalidation =
                PendingInvalidation.decodeToPendingInvalidation(invalidation.encodeToString());
        assertEquals(objectId, parsedInvalidation.mObjectId);
        assertEquals(objectSource, parsedInvalidation.mObjectSource);
        assertEquals(version, parsedInvalidation.mVersion);
        assertEquals(payload, parsedInvalidation.mPayload);
        assertEquals(invalidation, parsedInvalidation);
    }

    public void doTestParseToAndFromProtocolBufferThroughBundle(
            String objectId, int objectSource, long version, String payload) {
        PendingInvalidation invalidation =
                new PendingInvalidation(objectId, objectSource, version, payload);
        Bundle bundle = PendingInvalidation.decodeToBundle(invalidation.encodeToString());
        PendingInvalidation parsedInvalidation = new PendingInvalidation(bundle);
        assertEquals(objectId, parsedInvalidation.mObjectId);
        assertEquals(objectSource, parsedInvalidation.mObjectSource);
        assertEquals(version, parsedInvalidation.mVersion);
        assertEquals(payload, parsedInvalidation.mPayload);
        assertEquals(invalidation, parsedInvalidation);
    }
}
