/*
 * Copyright 2010 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.typography.font.sfntly;

import com.google.typography.font.sfntly.Font.PlatformId;
import com.google.typography.font.sfntly.Font.WindowsEncodingId;
import com.google.typography.font.sfntly.table.core.NameTable;
import com.google.typography.font.sfntly.table.core.NameTable.NameEntryBuilder;
import com.google.typography.font.sfntly.table.core.NameTable.NameId;
import com.google.typography.font.sfntly.table.core.NameTable.WindowsLanguageId;
import com.google.typography.font.sfntly.testutils.TestFont;
import com.google.typography.font.sfntly.testutils.TestFontUtils;
import com.google.typography.font.sfntly.testutils.TestUtils;

import junit.framework.TestCase;

import java.io.File;

/**
 * @author Stuart Gill
 */
public class NameEditingTests extends TestCase {

  private static final File fontFile = TestFont.TestFontNames.OPENSANS.getFile();

  public NameEditingTests(String name) {
    super(name);
  }

  public void testChangeOneName() throws Exception {
    String newName = "Timothy";

    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(fontFile);
    NameTable.Builder nameBuilder = (NameTable.Builder) fontBuilder.getTableBuilder(Tag.name);

    // change the font name
    NameEntryBuilder neb =
        nameBuilder.nameBuilder(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
            WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    neb.setName(newName);

    // build the font
    Font font = fontBuilder.build();

    // serialize and then load the serialized font
    File serializedFontFile = TestFontUtils.serializeFont(font, TestUtils.extension(fontFile));
    Font[] serializedFontArray = TestFontUtils.loadFont(serializedFontFile);
    font = serializedFontArray[0];

    // check the font name
    NameTable nameTable = font.getTable(Tag.name);
    String name = nameTable.name(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
        WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    assertTrue(name != null);
    assertEquals(newName, name);
  }

  public void testModifyNameTableAndRevert() throws Exception {
    String newName = "Timothy";
    String originalName;

    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(fontFile);
    NameTable.Builder nameBuilder = (NameTable.Builder) fontBuilder.getTableBuilder(Tag.name);

    // change the font name
    NameEntryBuilder neb =
        nameBuilder.nameBuilder(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
            WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    originalName = neb.name();
    neb.setName(newName);

    nameBuilder.revertNames();
    
    // build the font
    Font font = fontBuilder.build();

    // serialize and then load the serialized font
    File serializedFontFile = TestFontUtils.serializeFont(font, TestUtils.extension(fontFile));
    Font[] serializedFontArray = TestFontUtils.loadFont(serializedFontFile);
    font = serializedFontArray[0];

    // check the font name
    NameTable nameTable = font.getTable(Tag.name);
    String name = nameTable.name(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
        WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    assertTrue(name != null);
    assertEquals(originalName, name);
  }
  
  public void testRemoveOneName() throws Exception {
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(fontFile);
    NameTable.Builder nameBuilder = (NameTable.Builder) fontBuilder.getTableBuilder(Tag.name);

    // change the font name
    assertTrue(nameBuilder.has(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
        WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value()));

    assertTrue(nameBuilder.remove(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
        WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value()));

    // build the font
    Font font = fontBuilder.build();

    // serialize and then load the serialized font
    File serializedFontFile = TestFontUtils.serializeFont(font, TestUtils.extension(fontFile));
    Font[] serializedFontArray = TestFontUtils.loadFont(serializedFontFile);
    font = serializedFontArray[0];

    // check the font name
    NameTable nameTable = font.getTable(Tag.name);
    String name = nameTable.name(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
        WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    assertTrue(name == null);
  }
  
  public void testClearAllNamesAndSetOne() throws Exception {
    String newName = "Fred";
    
    Font.Builder fontBuilder = TestFontUtils.builderForFontFile(fontFile);
    NameTable.Builder nameBuilder = (NameTable.Builder) fontBuilder.getTableBuilder(Tag.name);

    assertFalse(0 == nameBuilder.builderCount());
    nameBuilder.clear();
    assertEquals(0, nameBuilder.builderCount());

    // change the font name
    NameEntryBuilder neb =
        nameBuilder.nameBuilder(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
            WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    neb.setName(newName);
    
    // build the font
    Font font = fontBuilder.build();

    // serialize and then load the serialized font
    File serializedFontFile = TestFontUtils.serializeFont(font, TestUtils.extension(fontFile));
    Font[] serializedFontArray = TestFontUtils.loadFont(serializedFontFile);
    font = serializedFontArray[0];

    // check the font name
    NameTable nameTable = font.getTable(Tag.name);
    nameTable.nameCount();
    String name = nameTable.name(PlatformId.Windows.value(), WindowsEncodingId.UnicodeUCS2.value(),
        WindowsLanguageId.English_UnitedStates.value(), NameId.FontFamilyName.value());
    assertTrue(name != null);
    assertEquals(newName, name);
    assertEquals(1, nameTable.nameCount());
  }
}
