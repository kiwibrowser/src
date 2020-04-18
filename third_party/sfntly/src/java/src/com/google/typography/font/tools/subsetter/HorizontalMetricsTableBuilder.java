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
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.core.HorizontalHeaderTable;

import java.util.List;

/**
 * A builder method for the hmtx (horizontal metrics) table. The goal is for this
 * logic to go into the HorizontalMetricsTable.Builder class, but for now is separate.
 * 
 * Note that this class also computes the advanceWidthMax value, which goes into the
 * hhea table, leading to somewhat awkward plumbing.
 * 
 * @author Raph Levien
 */
public class HorizontalMetricsTableBuilder {

  public static class LongHorMetric {
    public int advanceWidth;
    public int lsb;
    
    public LongHorMetric(int advanceWidth, int lsb) {
      this.advanceWidth = advanceWidth;
      this.lsb = lsb;
    }
  }
  
  private final Font.Builder fontBuilder;
  private final List<LongHorMetric> metrics;

  public HorizontalMetricsTableBuilder(Font.Builder fontBuilder, List<LongHorMetric> metrics) {
    this.fontBuilder = fontBuilder;
    this.metrics = metrics;
  }
  
  public void build() {
    int nMetrics = metrics.size();
    if (nMetrics <= 0) {
      throw new IllegalArgumentException("nMetrics must be positive");
    }
    int lastWidth = metrics.get(nMetrics - 1).advanceWidth;
    int numberOfHMetrics = nMetrics;
    while (numberOfHMetrics > 1 && metrics.get(numberOfHMetrics - 2).advanceWidth == lastWidth) {
      numberOfHMetrics--;
    }
    int size = 4 * numberOfHMetrics + 2 * (nMetrics - numberOfHMetrics);
    WritableFontData data = WritableFontData.createWritableFontData(size);
    int index = 0;
    int advanceWidthMax = 0;
    for (int i = 0; i < numberOfHMetrics; i++) {
      int advanceWidth = metrics.get(i).advanceWidth;
      advanceWidthMax = Math.max(advanceWidth, advanceWidthMax);
      index += data.writeUShort(index, advanceWidth);
      index += data.writeShort(index, metrics.get(i).lsb);
    }
    for (int i = numberOfHMetrics; i < nMetrics; i++) {
      index += data.writeShort(index, metrics.get(i).lsb);
    }
    fontBuilder.newTableBuilder(Tag.hmtx, data);
    HorizontalHeaderTable.Builder hheaBuilder =
        (HorizontalHeaderTable.Builder) fontBuilder.getTableBuilder(Tag.hhea);
    hheaBuilder.setNumberOfHMetrics(numberOfHMetrics);
    hheaBuilder.setAdvanceWidthMax(advanceWidthMax);
  }
}
