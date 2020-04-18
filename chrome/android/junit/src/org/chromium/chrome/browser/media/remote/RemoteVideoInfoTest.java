// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Unit tests (run on host) for {@link RemoteVideoInfo}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class RemoteVideoInfoTest {
    /**
     * Test method for {@link RemoteVideoInfo#hashCode()}.
     */
    @Test
    public void testHashCode() {
        RemoteVideoInfo i1 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED,
                50, null);
        int h1 = i1.hashCode();

        RemoteVideoInfo i2 = new RemoteVideoInfo(i1);
        assertThat("Copying RemoteVideoInfo preserves the hash code", i2.hashCode(), equalTo(h1));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED,
                50, null);
        assertThat("Identical RemoteVideoInfos the same hash code", i2.hashCode(), equalTo(h1));

        int ho = new Object().hashCode();
        assertThat("Objects of different types are unequal to RemoteVideoInfos", i1.hashCode(),
                not(equalTo(ho)));

        i2 = new RemoteVideoInfo("Test1", 100, RemoteVideoInfo.PlayerState.STOPPED, 50, null);
        assertThat("Changing title changes hash code", i2.hashCode(), not(equalTo(h1)));

        i2 = new RemoteVideoInfo("Test", 200, RemoteVideoInfo.PlayerState.STOPPED, 50, null);
        assertThat("Changing duration changes hash code", i2.hashCode(), not(equalTo(h1)));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.INVALIDATED, 50, null);
        assertThat("Changing state changes hash code", i2.hashCode(), not(equalTo(h1)));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED, 70, null);
        assertThat("Changing current time changes hash code", i2.hashCode(), not(equalTo(h1)));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED, 50, "Error");
        assertThat("Changing error changes hash code", i2.hashCode(), not(equalTo(h1)));
    }

    /**
     * Test method for
     * {@link RemoteVideoInfo#equals(java.lang.Object)}.
     */
    @Test
    public void testEqualsObject() {
        RemoteVideoInfo i1 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED,
                50, null);
        assertThat("A RemoteVideoInfo is equal to itself", i1, equalTo(i1));

        RemoteVideoInfo i2 = new RemoteVideoInfo(i1);
        assertThat("A RemoteVideoInfo is equal to its copy", i2, equalTo(i1));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED,
                50, null);
        assertThat("identical RemoteVideoInfos are equal", i2, equalTo(i1));

        Object o = new Object();
        assertThat("Objects of different types are unequal to RemoteVideoInfos", i1,
                not(equalTo(o)));

        i2 = new RemoteVideoInfo("Test1", 100, RemoteVideoInfo.PlayerState.STOPPED, 50, null);
        assertThat("Changing title makes RemoteVideoInfos unequal", i2, not(equalTo(i1)));

        i2 = new RemoteVideoInfo("Test", 200, RemoteVideoInfo.PlayerState.STOPPED, 50, null);
        assertThat("Changing duration makes RemoteVideoInfos unequal", i2, not(equalTo(i1)));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.INVALIDATED, 50, null);
        assertThat("Changing state makes RemoteVideoInfos unequal", i2, not(equalTo(i1)));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED, 70, null);
        assertThat("Changing current time makes RemoteVideoInfos unequal", i2, not(equalTo(i1)));

        i2 = new RemoteVideoInfo("Test", 100, RemoteVideoInfo.PlayerState.STOPPED, 50, "Error");
        assertThat("Changing error makes RemoteVideoInfos unequal", i2, not(equalTo(i1)));
    }

}
