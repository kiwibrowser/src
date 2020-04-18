package com.google.typography.font.sfntly.issue_tests;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.core.NameTable;
import com.google.typography.font.sfntly.testutils.TestFont;

import junit.framework.TestCase;

import java.io.File;
import java.io.FileInputStream;

/*
 * Test for Issue 27.
 * Adding a zero length name that sorts at the end of the name table.
 * http://code.google.com/p/sfntly/issues/detail?id=27
 */
public class Issue27Tests extends TestCase {

  private static final File fontFile = TestFont.TestFontNames.OPENSANS.getFile();

  public void testIssue27() throws Exception {
    FontFactory fontFactory = FontFactory.getInstance();
    Font.Builder fontBuilder = fontFactory.loadFontsForBuilding(new FileInputStream(fontFile))[0];
    NameTable.Builder nameTableBuilder = (NameTable.Builder) fontBuilder.getTableBuilder(Tag.name);
    // add a name that will sort after all the other names in the table
    nameTableBuilder.nameBuilder(Font.PlatformId.Windows.value(), 
        Font.WindowsEncodingId.UnicodeUCS4.value(), 
        NameTable.WindowsLanguageId.Spanish_UnitedStates.value(), 
        NameTable.NameId.WWSSubfamilyName.value()).setName("");
    Font font = fontBuilder.build();
    assertNotNull(font);
  }
}
