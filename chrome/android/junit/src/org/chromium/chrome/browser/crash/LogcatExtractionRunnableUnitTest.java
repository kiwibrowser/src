// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.crash;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import static org.chromium.chrome.browser.crash.LogcatExtractionRunnable.BEGIN_MICRODUMP;
import static org.chromium.chrome.browser.crash.LogcatExtractionRunnable.END_MICRODUMP;
import static org.chromium.chrome.browser.crash.LogcatExtractionRunnable.SNIPPED_MICRODUMP;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 * junit tests for {@link LogcatExtractionRunnable}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class LogcatExtractionRunnableUnitTest {
    private static final int MAX_LINES = 5;

    @Test
    public void testElideEmail() {
        String original = "email me at someguy@mailservice.com";
        String expected = "email me at XXX@EMAIL.ELIDED";
        assertEquals(expected, LogcatExtractionRunnable.elideEmail(original));
    }

    @Test
    public void testElideUrl() {
        String original = "file bugs at crbug.com";
        String expected = "file bugs at HTTP://WEBADDRESS.ELIDED";
        assertEquals(expected, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl2() {
        String original =
                "exception at org.chromium.chrome.browser.crash.LogcatExtractionRunnableUnitTest";
        assertEquals(original, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl3() {
        String original = "file bugs at crbug.com or code.google.com";
        String expected = "file bugs at HTTP://WEBADDRESS.ELIDED or HTTP://WEBADDRESS.ELIDED";
        assertEquals(expected, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl4() {
        String original = "test shorturl.com !!!";
        String expected = "test HTTP://WEBADDRESS.ELIDED !!!";
        assertEquals(expected, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl5() {
        String original = "test just.the.perfect.len.url !!!";
        String expected = "test HTTP://WEBADDRESS.ELIDED !!!";
        assertEquals(expected, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl6() {
        String original = "test a.very.very.very.very.very.long.url !!!";
        String expected = "test HTTP://WEBADDRESS.ELIDED !!!";
        assertEquals(expected, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl7() {
        String original = " at android.content.Intent \n at java.util.ArrayList";
        assertEquals(original, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl8() {
        String original = "exception at org.chromium.chrome.browser.compositor.scene_layer."
                + "TabListSceneLayer.nativeUpdateLayer(Native Method)";
        assertEquals(original, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideUrl9() {
        String original = "I/dalvikvm( 5083): at org.chromium.chrome.browser.compositor."
                + "scene_layer.TabListSceneLayer.nativeUpdateLayer(Native Method)";
        assertEquals(original, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testDontElideFileSuffixes() {
        String original = "chromium_android_linker.so";
        assertEquals(original, LogcatExtractionRunnable.elideUrl(original));
    }

    @Test
    public void testElideIp() {
        String original = "traceroute 127.0.0.1";
        String expected = "traceroute 1.2.3.4";
        assertEquals(expected, LogcatExtractionRunnable.elideIp(original));
    }

    @Test
    public void testElideMac1() {
        String original = "MAC: AB-AB-AB-AB-AB-AB";
        String expected = "MAC: 01:23:45:67:89:AB";
        assertEquals(expected, LogcatExtractionRunnable.elideMac(original));
    }

    @Test
    public void testElideMac2() {
        String original = "MAC: AB:AB:AB:AB:AB:AB";
        String expected = "MAC: 01:23:45:67:89:AB";
        assertEquals(expected, LogcatExtractionRunnable.elideMac(original));
    }

    @Test
    public void testElideConsole() {
        String original = "I/chromium(123): [INFO:CONSOLE(2)] hello!";
        String expected = "I/chromium(123): [ELIDED:CONSOLE(0)] ELIDED CONSOLE MESSAGE";
        assertEquals(expected, LogcatExtractionRunnable.elideConsole(original));
    }

    @Test
    public void testLogTagNotElided() {
        List<String> original = Arrays.asList(new String[] {"I/cr_FooBar(123): Some message"});
        assertEquals(original, LogcatExtractionRunnable.elideLogcat(original));
    }

    @Test
    public void testLogcatEmpty() {
        final List<String> original = new LinkedList<>();
        assertLogcatLists(original, original);
    }

    @Test
    public void testLogcatWithoutBeginOrEnd_smallLogcat() {
        final List<String> original =
                Arrays.asList("Line 1", "Line 2", "Line 3", "Line 4", "Line 5");
        assertLogcatLists(original, original);
    }

    @Test
    public void testLogcatWithoutBeginOrEnd_largeLogcat() {
        final List<String> original = Arrays.asList("Trimmed Line 1", "Trimmed Line 2", "Line 3",
                "Line 4", "Line 5", "Line 6", "Line 7");
        final List<String> expected =
                Arrays.asList("Line 3", "Line 4", "Line 5", "Line 6", "Line 7");
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatBeginsWithBegin() {
        final List<String> original = Arrays.asList(BEGIN_MICRODUMP, "a", "b", "c", "d", "e");
        final List<String> expected = Arrays.asList(SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatWithBegin() {
        final List<String> original =
                Arrays.asList("Line 1", "Line 2", BEGIN_MICRODUMP, "a", "b", "c", "d", "e");
        final List<String> expected = Arrays.asList("Line 1", "Line 2", SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatWithEnd() {
        final List<String> original = Arrays.asList("Line 1", "Line 2", END_MICRODUMP);
        assertLogcatLists(original, original);
    }

    @Test
    public void testLogcatWithBeginAndEnd_smallLogcat() {
        final List<String> original = Arrays.asList(
                "Line 1", "Line 2", BEGIN_MICRODUMP, "a", "b", "c", "d", "e", END_MICRODUMP);
        final List<String> expected = Arrays.asList("Line 1", "Line 2", SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatWithBeginAndEnd_splitLogcat() {
        final List<String> original = Arrays.asList("Line 1", "Line 2", BEGIN_MICRODUMP, "a", "b",
                "c", "d", "e", END_MICRODUMP, "Trimmed Line 3", "Trimmed Line 4");
        final List<String> expected = Arrays.asList("Line 1", "Line 2", SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatWithBeginAndEnd_largeLogcat() {
        final List<String> original = Arrays.asList("Trimmed Line 1", "Trimmed Line 2", "Line 3",
                "Line 4", "Line 5", "Line 6", BEGIN_MICRODUMP, "a", "b", "c", "d", "e",
                END_MICRODUMP, "Trimmed Line 7", "Trimmed Line 8");
        final List<String> expected =
                Arrays.asList("Line 3", "Line 4", "Line 5", "Line 6", SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatWithEndAndBegin_smallLogcat() {
        final List<String> original = Arrays.asList(
                END_MICRODUMP, "Line 1", "Line 2", BEGIN_MICRODUMP, "a", "b", "c", "d", "e");
        final List<String> expected =
                Arrays.asList(END_MICRODUMP, "Line 1", "Line 2", SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    @Test
    public void testLogcatWithEndAndBegin_largeLogcat() {
        final List<String> original =
                Arrays.asList(END_MICRODUMP, "Line 1", "Line 2", BEGIN_MICRODUMP, "a", "b", "c",
                        "d", "e", END_MICRODUMP, "Trimmed Line 3", "Trimmed Line 4");
        final List<String> expected =
                Arrays.asList(END_MICRODUMP, "Line 1", "Line 2", SNIPPED_MICRODUMP);
        assertLogcatLists(expected, original);
    }

    private void assertLogcatLists(List<String> expected, List<String> original) {
        // trimLogcat() expects a modifiable list as input.
        LinkedList<String> rawLogcat = new LinkedList<String>(original);
        List<String> actualLogcat = LogcatExtractionRunnable.trimLogcat(rawLogcat, MAX_LINES);
        assertArrayEquals(expected.toArray(), actualLogcat.toArray());
    }
}
