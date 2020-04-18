// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.policy;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.os.Build;
import android.os.Bundle;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Robolectric test for AbstractAppRestrictionsProvider.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = Build.VERSION_CODES.LOLLIPOP)
public class PolicyConverterTest {
    /**
     * Test method for
     * {@link org.chromium.policy.PolicyConverter#setPolicy(java.lang.String, java.lang.Object)}.
     */
    @Test
    public void testSetPolicy() {
        // Stub out the native methods.
        PolicyConverter policyConverter = spy(PolicyConverter.create(1234));
        doNothing()
                .when(policyConverter)
                .nativeSetPolicyBoolean(anyLong(), anyString(), anyBoolean());
        doNothing().when(policyConverter).nativeSetPolicyInteger(anyLong(), anyString(), anyInt());
        doNothing()
                .when(policyConverter)
                .nativeSetPolicyString(anyLong(), anyString(), anyString());
        doNothing()
                .when(policyConverter)
                .nativeSetPolicyStringArray(anyLong(), anyString(), any(String[].class));

        policyConverter.setPolicy("p1", true);
        verify(policyConverter).nativeSetPolicyBoolean(1234, "p1", true);
        policyConverter.setPolicy("p1", 5678);
        verify(policyConverter).nativeSetPolicyInteger(1234, "p1", 5678);
        policyConverter.setPolicy("p1", "hello");
        verify(policyConverter).nativeSetPolicyString(1234, "p1", "hello");
        policyConverter.setPolicy("p1", new String[] {"hello", "goodbye"});
        verify(policyConverter)
                .nativeSetPolicyStringArray(1234, "p1", new String[] {"hello", "goodbye"});
        Bundle b1 = new Bundle();
        b1.putInt("i1", 23);
        b1.putString("s1", "a string");
        Bundle[] ba = new Bundle[1];
        ba[0] = new Bundle();
        ba[0].putBoolean("ba1b", true);
        ba[0].putString("ba1s", "another string");
        b1.putParcelableArray("b1b", ba);
        policyConverter.setPolicy("p1", b1);
        verify(policyConverter)
                .nativeSetPolicyString(1234, "p1", "{\"i1\":23,\"s1\":\"a string\","
                                + "\"b1b\":[{\"ba1b\":true,\"ba1s\":\"another string\"}]}");
        policyConverter.setPolicy("p1", ba);
        verify(policyConverter)
                .nativeSetPolicyString(1234, "p1", "[{\"ba1b\":true,\"ba1s\":\"another string\"}]");
    }
}
