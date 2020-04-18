// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.accessibility.captioning;

import android.graphics.Color;
import android.graphics.Typeface;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.content.browser.accessibility.captioning.CaptioningChangeDelegate.ClosedCaptionEdgeAttribute;
import org.chromium.content.browser.accessibility.captioning.CaptioningChangeDelegate.ClosedCaptionFont;

/**
  * Test suite to ensure that platform settings are translated to CSS appropriately
  */
@RunWith(BaseJUnit4ClassRunner.class)
public class CaptioningChangeDelegateTest {
    private static final String DEFAULT_CAPTIONING_PREF_VALUE =
            CaptioningChangeDelegate.DEFAULT_CAPTIONING_PREF_VALUE;

    @Test
    @SmallTest
    public void testFontScaleToPercentage() {
        String result = CaptioningChangeDelegate.androidFontScaleToPercentage(0f);
        Assert.assertEquals("0%", result);

        result = CaptioningChangeDelegate.androidFontScaleToPercentage(0.000f);
        Assert.assertEquals("0%", result);

        result = CaptioningChangeDelegate.androidFontScaleToPercentage(0.25f);
        Assert.assertEquals("25%", result);

        result = CaptioningChangeDelegate.androidFontScaleToPercentage(1f);
        Assert.assertEquals("100%", result);

        result = CaptioningChangeDelegate.androidFontScaleToPercentage(1.5f);
        Assert.assertEquals("150%", result);

        result = CaptioningChangeDelegate.androidFontScaleToPercentage(0.50125f);
        Assert.assertEquals("50%", result);

        result = CaptioningChangeDelegate.androidFontScaleToPercentage(0.50925f);
        Assert.assertEquals("51%", result);
    }

    @Test
    @SmallTest
    public void testAndroidColorToCssColor() {
        String result = CaptioningChangeDelegate.androidColorToCssColor(null);
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, result);

        result = CaptioningChangeDelegate.androidColorToCssColor(Color.BLACK);
        Assert.assertEquals("rgba(0, 0, 0, 1)", result);

        result = CaptioningChangeDelegate.androidColorToCssColor(Color.WHITE);
        Assert.assertEquals("rgba(255, 255, 255, 1)", result);

        result = CaptioningChangeDelegate.androidColorToCssColor(Color.BLUE);
        Assert.assertEquals("rgba(0, 0, 255, 1)", result);

        // Transparent-black
        result = CaptioningChangeDelegate.androidColorToCssColor(0x00000000);
        Assert.assertEquals("rgba(0, 0, 0, 0)", result);

        // Transparent-white
        result = CaptioningChangeDelegate.androidColorToCssColor(0x00FFFFFF);
        Assert.assertEquals("rgba(255, 255, 255, 0)", result);

        // 50% opaque blue
        result = CaptioningChangeDelegate.androidColorToCssColor(0x7f0000ff);
        Assert.assertEquals("rgba(0, 0, 255, 0.5)", result);

        // No alpha information
        result = CaptioningChangeDelegate.androidColorToCssColor(0xFFFFFF);
        Assert.assertEquals("rgba(255, 255, 255, 0)", result);
    }

    @Test
    @SmallTest
    public void testClosedCaptionEdgeAttributeWithDefaults() {
        ClosedCaptionEdgeAttribute edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(
                null, null);
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(null, "red");
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(0, "red");
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(2, null);
        Assert.assertEquals("silver 0.05em 0.05em 0.1em", edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(2, "");
        Assert.assertEquals("silver 0.05em 0.05em 0.1em", edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(2, "red");
        Assert.assertEquals("red 0.05em 0.05em 0.1em", edge.getTextShadow());
    }

    @Test
    @SmallTest
    public void testClosedCaptionEdgeAttributeWithCustomDefaults() {
        ClosedCaptionEdgeAttribute.setShadowOffset("0.00em");
        ClosedCaptionEdgeAttribute.setDefaultEdgeColor("red");
        ClosedCaptionEdgeAttribute edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(
                null, null);
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(null, "red");
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(0, "red");
        Assert.assertEquals(DEFAULT_CAPTIONING_PREF_VALUE, edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(2, null);
        Assert.assertEquals("red 0.00em 0.00em 0.1em", edge.getTextShadow());

        edge = ClosedCaptionEdgeAttribute.fromSystemEdgeAttribute(2, "silver");
        Assert.assertEquals("silver 0.00em 0.00em 0.1em", edge.getTextShadow());
    }

    /**
     * Verifies that certain system fonts always correspond to the default captioning font.
     */
    @Test
    @SmallTest
    public void testClosedCaptionDefaultFonts() {
        final ClosedCaptionFont nullFont = ClosedCaptionFont.fromSystemFont(null);
        Assert.assertEquals("Null typeface should return the default font family.",
                DEFAULT_CAPTIONING_PREF_VALUE, nullFont.getFontFamily());

        final ClosedCaptionFont defaultFont = ClosedCaptionFont.fromSystemFont(Typeface.DEFAULT);
        Assert.assertEquals("Typeface.DEFAULT should return the default font family.",
                DEFAULT_CAPTIONING_PREF_VALUE, defaultFont.getFontFamily());

        final ClosedCaptionFont defaultBoldFont = ClosedCaptionFont.fromSystemFont(
                Typeface.DEFAULT_BOLD);
        Assert.assertEquals("Typeface.BOLD should return the default font family.",
                DEFAULT_CAPTIONING_PREF_VALUE, defaultBoldFont.getFontFamily());
    }

    /**
     * Typeface.DEFAULT may be equivalent to another Typeface such as Typeface.SANS_SERIF
     * so this test ensures that each typeface returns DEFAULT_CAPTIONING_PREF_VALUE if it is
     * equal to Typeface.DEFAULT or returns an explicit font family otherwise.
     */
    @Test
    @SmallTest
    public void testClosedCaptionNonDefaultFonts() {
        final ClosedCaptionFont monospaceFont = ClosedCaptionFont.fromSystemFont(
                Typeface.MONOSPACE);
        if (Typeface.MONOSPACE.equals(Typeface.DEFAULT)) {
            Assert.assertEquals(
                    "Since the default font is monospace, the default family should be returned.",
                    DEFAULT_CAPTIONING_PREF_VALUE, monospaceFont.getFontFamily());
        } else {
            Assert.assertTrue("Typeface.MONOSPACE should return a monospace font family.",
                    monospaceFont.mFlags.contains(ClosedCaptionFont.Flags.MONOSPACE));
        }

        final ClosedCaptionFont sansSerifFont = ClosedCaptionFont.fromSystemFont(
                Typeface.SANS_SERIF);
        if (Typeface.SANS_SERIF.equals(Typeface.DEFAULT)) {
            Assert.assertEquals(
                    "Since the default font is sans-serif, the default family should be returned.",
                    DEFAULT_CAPTIONING_PREF_VALUE, sansSerifFont.getFontFamily());
        } else {
            Assert.assertTrue("Typeface.SANS_SERIF should return a sans-serif font family.",
                    sansSerifFont.mFlags.contains(ClosedCaptionFont.Flags.SANS_SERIF));
        }

        final ClosedCaptionFont serifFont = ClosedCaptionFont.fromSystemFont(Typeface.SERIF);
        if (Typeface.SERIF.equals(Typeface.DEFAULT)) {
            Assert.assertEquals(
                    "Since the default font is serif, the default font family should be returned.",
                    DEFAULT_CAPTIONING_PREF_VALUE, serifFont.getFontFamily());
        } else {
            Assert.assertTrue("Typeface.SERIF should return a serif font family.",
                    serifFont.mFlags.contains(ClosedCaptionFont.Flags.SERIF));
        }
    }
}
