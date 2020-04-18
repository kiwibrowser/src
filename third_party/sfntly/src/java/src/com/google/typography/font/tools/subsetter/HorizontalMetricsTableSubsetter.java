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
import com.google.typography.font.sfntly.Font.Builder;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.core.HorizontalMetricsTable;

import java.util.ArrayList;
import java.util.List;

/**
 * @author Raph Levien
 */
public class HorizontalMetricsTableSubsetter extends TableSubsetterImpl {

  protected HorizontalMetricsTableSubsetter() {
    // Note: doesn't actually create the hhea table, that should be done in the
    // setUpTables method of the invoking subsetter.
    super(Tag.hmtx, Tag.hhea);
  }
  
  @Override
  public boolean subset(Subsetter subsetter, Font font, Builder fontBuilder) {
    List<Integer> permutationTable = subsetter.glyphMappingTable();
    if (permutationTable == null) {
      return false;
    }
    HorizontalMetricsTable origMetrics = font.getTable(Tag.hmtx);
    List<HorizontalMetricsTableBuilder.LongHorMetric> metrics =
        new ArrayList<HorizontalMetricsTableBuilder.LongHorMetric>();
    for (int i = 0; i < permutationTable.size(); i++) {
      int origGlyphId = permutationTable.get(i);
      int advanceWidth = origMetrics.advanceWidth(origGlyphId);
      int lsb = origMetrics.leftSideBearing(origGlyphId);
      metrics.add(new HorizontalMetricsTableBuilder.LongHorMetric(advanceWidth, lsb));
    }
    new HorizontalMetricsTableBuilder(fontBuilder, metrics).build();
    return true;
  }
}
