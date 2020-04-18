/*
 * Copyright 2011 Google Inc. All Rights Reserved.
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

package com.google.typography.font.tools.subsetter;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.core.PostScriptTable;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.List;

/**
 * @author Raph Levien
 */
public class PostScriptTableBuilderTest extends TestCase {

  public void testPostTableBuilding() {
    FontFactory fontFactory = FontFactory.getInstance();
    Font.Builder fontBuilder = fontFactory.newFontBuilder();
    List<String> names = new ArrayList<String>();
    names.add(".notdef");
    names.add("numbersign");
    names.add("nonstandardglyph");
    names.add("dcroat");
    names.add("nonstandardglyph2");
    PostScriptTableBuilder postBuilder = new PostScriptTableBuilder();
    postBuilder.setNames(names);
    fontBuilder.newTableBuilder(Tag.post, postBuilder.build());
    
    Font font = fontBuilder.build();
    PostScriptTable post = font.getTable(Tag.post);
    assertEquals(0x20000, post.version());
    assertEquals(names.size(), post.numberOfGlyphs());
    for (int i = 0; i < names.size(); i++) {
      assertEquals(names.get(i), post.glyphName(i));
    }
    // Length depends on exact content of names array. The main point of checking
    // this is to make sure that standard names are encoded compactly.
    // the length is 79
    assertEquals(79, post.dataLength());
    assertEquals(79, post.headerLength());
  }
  
  // TODO: test initV1From()
}
