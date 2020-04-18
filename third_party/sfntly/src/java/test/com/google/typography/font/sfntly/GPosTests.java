// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly;

import com.google.typography.font.sfntly.table.Header;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.testutils.TestFont.TestFontNames;
import com.google.typography.font.sfntly.testutils.TestFontUtils;

import junit.framework.TestCase;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @author dougfelt@google.com (Doug Felt)
 */
public class GPosTests extends TestCase {
  public void testGposFiles() {
    List<Font> gposFontList = new ArrayList<Font>();
    for (TestFontNames name : TestFontNames.values()) {
      Font[] fonts;
      try {
        fonts = TestFontUtils.loadFont(name.getFile());
        assertNotNull(fonts);
      } catch (IOException e) {
        System.out.format("caught exception (%s) when loading font %s\n", e.getMessage(), name);
        continue;
      }
      for (int i = 0; i < fonts.length; ++i) {
        Font font = fonts[i];
        if (font.hasTable(Tag.GPOS)) {
          System.out.format("Font %s(%d) has GPOS\n", name, i);
          gposFontList.add(font);

          Table gpos = font.getTable(Tag.GPOS);
          Header gposHeader = gpos.header();
          System.out.println(gposHeader);
        }
      }
    }
    assertTrue("have test gpos file", gposFontList.size() > 0);
    
    for (Font font : gposFontList) {
    }
  }
}
