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
import com.google.typography.font.sfntly.table.core.HorizontalHeaderTable;
import com.google.typography.font.sfntly.table.core.HorizontalMetricsTable;
import com.google.typography.font.sfntly.table.core.MaximumProfileTable;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.List;

/**
 * @author Raph Levien
 */
public class HorizontalMetricsTableBuilderTest extends TestCase {

  public void testHorizontalMetricsTableBuilder() {
    FontFactory fontFactory = FontFactory.getInstance();
    Font.Builder fontBuilder = fontFactory.newFontBuilder();
    HorizontalHeaderTable.Builder hheaBuilder =
      (HorizontalHeaderTable.Builder) fontBuilder.newTableBuilder(Tag.hhea);

    List<HorizontalMetricsTableBuilder.LongHorMetric> metrics =
        new ArrayList<HorizontalMetricsTableBuilder.LongHorMetric>();
    metrics.add(new HorizontalMetricsTableBuilder.LongHorMetric(123, 42));
    metrics.add(new HorizontalMetricsTableBuilder.LongHorMetric(123, 43));
    metrics.add(new HorizontalMetricsTableBuilder.LongHorMetric(789, 44));
    metrics.add(new HorizontalMetricsTableBuilder.LongHorMetric(789, 45));
    new HorizontalMetricsTableBuilder(fontBuilder, metrics).build();

    MaximumProfileTable.Builder maxpBuilder =
        (MaximumProfileTable.Builder) fontBuilder.newTableBuilder(Tag.maxp);
    maxpBuilder.setNumGlyphs(4);

    Font font = fontBuilder.build();

    HorizontalMetricsTable hmtxTable = font.getTable(Tag.hmtx);
    assertEquals(3, hmtxTable.numberOfHMetrics());
    assertEquals(1, hmtxTable.numberOfLSBs());
    assertEquals(123, hmtxTable.advanceWidth(0));
    assertEquals(42, hmtxTable.leftSideBearing(0));
    assertEquals(123, hmtxTable.advanceWidth(1));
    assertEquals(43, hmtxTable.leftSideBearing(1));
    assertEquals(789, hmtxTable.advanceWidth(2));
    assertEquals(44, hmtxTable.leftSideBearing(2));
    assertEquals(789, hmtxTable.advanceWidth(3));
    assertEquals(45, hmtxTable.leftSideBearing(3));
  }

}
