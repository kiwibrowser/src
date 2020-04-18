// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;

/**
 * Robolectric tests for {@link CastMediaSource} class.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastMediaSourceTest {
    @Test
    @Feature({"MediaRouter"})
    public void testCorrectSourceId() {
        final String sourceId = "https://example.com/path?query"
                + "#__castAppId__=ABCD1234(video_out,audio_out)"
                + "/__castClientId__=1234567890"
                + "/__castAutoJoinPolicy__=tab_and_origin_scoped";

        CastMediaSource source = CastMediaSource.from(sourceId);
        assertNotNull(source);
        assertEquals("ABCD1234", source.getApplicationId());

        assertNotNull(source.getCapabilities());
        assertEquals(2, source.getCapabilities().length);
        assertEquals("video_out", source.getCapabilities()[0]);
        assertEquals("audio_out", source.getCapabilities()[1]);

        assertEquals("1234567890", source.getClientId());
        assertEquals("tab_and_origin_scoped", source.getAutoJoinPolicy());

        assertEquals(sourceId, source.getSourceId());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNoFragment() {
        assertNull(CastMediaSource.from("https://example.com/path?query"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testEmptyFragment() {
        assertNull(CastMediaSource.from("https://example.com/path?query#"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNoAppId() {
        assertNull(CastMediaSource.from("https://example.com/path?query#fragment"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNoValidAppId() {
        // Invalid app id needs to indicate no availability so {@link CastMediaSource} needs to be
        // created.
        CastMediaSource empty =
                CastMediaSource.from("https://example.com/path?query#__castAppId__=");
        assertNotNull(empty);
        assertEquals("", empty.getApplicationId());

        CastMediaSource invalid =
                CastMediaSource.from("https://example.com/path?query#__castAppId__=INVALID-APP-ID");
        assertNotNull(invalid);
        assertEquals("INVALID-APP-ID", invalid.getApplicationId());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNoCapabilitiesSuffix() {
        assertNull(CastMediaSource.from("https://example.com/path?query#__castAppId__=A("));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCapabilitiesEmpty() {
        assertNull(CastMediaSource.from("https://example.com/path?query#__castAppId__=A()"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testInvalidCapability() {
        assertNull(CastMediaSource.from("https://example.com/path?query#__castAppId__=A(a)"));
        assertNull(
                CastMediaSource.from("https://example.com/path?query#__castAppId__=A(video_in,b)"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testInvalidAutoJoinPolicy() {
        assertNull(CastMediaSource.from("https://example.com/path?query#__castAppId__=A"
                + "/__castAutoJoinPolicy__=invalidPolicy"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOptionalParameters() {
        CastMediaSource source =
                CastMediaSource.from("https://example.com/path?query#__castAppId__=A");
        assertNotNull(source);
        assertEquals("A", source.getApplicationId());

        assertNull(source.getCapabilities());
        assertNull(source.getClientId());
        assertEquals("tab_and_origin_scoped", source.getAutoJoinPolicy());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testBasicCastPresentationUrl() {
        CastMediaSource source = CastMediaSource.from("cast:ABCD1234");
        assertNotNull(source);
        assertEquals("ABCD1234", source.getApplicationId());
        assertNull(source.getCapabilities());
        assertNull(source.getClientId());
        assertEquals("tab_and_origin_scoped", source.getAutoJoinPolicy());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastPresentationUrlWithParameters() {
        CastMediaSource source = CastMediaSource.from("cast:ABCD1234?clientId=1234"
                + "&capabilities=video_out,audio_out"
                + "&autoJoinPolicy=tab_and_origin_scoped");
        assertNotNull(source);
        assertEquals("ABCD1234", source.getApplicationId());
        assertNotNull(source.getCapabilities());
        assertEquals(2, source.getCapabilities().length);
        assertEquals("video_out", source.getCapabilities()[0]);
        assertEquals("audio_out", source.getCapabilities()[1]);
        assertEquals("1234", source.getClientId());
        assertEquals("tab_and_origin_scoped", source.getAutoJoinPolicy());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastPresentationUrlInvalidCapability() {
        assertNull(CastMediaSource.from("cast:ABCD1234?clientId=1234"
                + "&capabilities=invalidCapability"));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCastPresentationUrlInvalidAutoJoinPolicy() {
        assertNull(CastMediaSource.from("cast:ABCD1234?clientId=1234"
                + "&autoJoinPolicy=invalidPolicy"));
    }
}
