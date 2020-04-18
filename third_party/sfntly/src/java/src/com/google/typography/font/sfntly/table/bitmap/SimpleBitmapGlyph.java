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

package com.google.typography.font.sfntly.table.bitmap;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

/**
 * @author Stuart Gill
 *
 */
public final class SimpleBitmapGlyph extends BitmapGlyph {

  protected SimpleBitmapGlyph(ReadableFontData data, int format) {
    super(data, format);
  }

  public static class Builder extends BitmapGlyph.Builder<BitmapGlyph> {

    protected Builder(WritableFontData data, int format) {
      super(data, format);
    }

    protected Builder(ReadableFontData data, int format) {
      super(data, format);
    }

    @Override
    protected SimpleBitmapGlyph subBuildTable(ReadableFontData data) {
      return new SimpleBitmapGlyph(data, this.format());
    }
  }
}
