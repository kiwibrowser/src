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

import java.util.HashSet;
import java.util.Set;

/**
 * Note: at the moment, this class just replicates the existing functionality in sfntly. It does
 * _not_ create a working font.
 * 
 * @author Raph Levien
 */
public class DumbSubsetter extends Subsetter {

  {
    Set<TableSubsetter> temp = new HashSet<TableSubsetter>();
    temp.add(new GlyphTableSubsetter());
    temp.add(new CMapTableSubsetter());
    tableSubsetters = temp;
  }

  public DumbSubsetter(Font font, FontFactory fontFactory) {
    super(font, fontFactory);
  }

  @Override
  protected void setUpTables(Font.Builder fontBuilder) {
    fontBuilder.newTableBuilder(Tag.maxp, font.getTable(Tag.maxp).readFontData());
  }
}
