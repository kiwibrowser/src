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

package com.google.typography.font.sfntly.table;

import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;
import com.google.typography.font.sfntly.table.bitmap.EbdtTable;
import com.google.typography.font.sfntly.table.bitmap.EblcTable;
import com.google.typography.font.sfntly.table.bitmap.EbscTable;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.core.FontHeaderTable;
import com.google.typography.font.sfntly.table.core.HorizontalDeviceMetricsTable;
import com.google.typography.font.sfntly.table.core.HorizontalHeaderTable;
import com.google.typography.font.sfntly.table.core.HorizontalMetricsTable;
import com.google.typography.font.sfntly.table.core.MaximumProfileTable;
import com.google.typography.font.sfntly.table.core.NameTable;
import com.google.typography.font.sfntly.table.core.OS2Table;
import com.google.typography.font.sfntly.table.core.PostScriptTable;
import com.google.typography.font.sfntly.table.opentype.GSubTable;
import com.google.typography.font.sfntly.table.truetype.ControlProgramTable;
import com.google.typography.font.sfntly.table.truetype.ControlValueTable;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;


/**
 * A concrete implementation of a root level table in the font. This is the base
 * class used for all specific table implementations and is used as the generic
 * table for all tables which have no specific implementations.
 *
 * @author Stuart Gill
 */
public class Table extends FontDataTable {

  private Header header;

  protected Table(Header header, ReadableFontData data) {
    super(data);
    this.header = header;
  }

  /**
   * Get the calculated checksum for the data in the table.
   *
   * @return the checksum
   */
  public long calculatedChecksum() {
    return this.data.checksum();
  }

  /**
   * Get the header for the table.
   *
   * @return the table header
   */
  public Header header() {
    return this.header;
  }

  /**
   * Get the tag for the table from the record header.
   *
   * @return the tag for the table
   * @see #header
   */
  public int headerTag() {
    return this.header().tag();
  }

  /**
   * Get the offset for the table from the record header.
   *
   * @return the offset for the table
   * @see #header
   */
  public int headerOffset() {
    return this.header().offset();
  }

  /**
   * Get the length of the table from the record header.
   *
   * @return the length of the table
   * @see #header
   */
  public int headerLength() {
    return this.header().length();
  }

  /**
   * Get the checksum for the table from the record header.
   *
   * @return the checksum for the table
   * @see #header
   */
  public long headerChecksum() {
    return this.header().checksum();
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder();
    sb.append("[");
    sb.append(Tag.stringValue(this.header.tag()));
    sb.append(", cs=0x");
    sb.append(Long.toHexString(this.header.checksum()));
    sb.append(", offset=0x");
    sb.append(Integer.toHexString(this.header.offset()));
    sb.append(", size=0x");
    sb.append(Integer.toHexString(this.header.length()));
    sb.append("]");
    return sb.toString();
  }

  public abstract static class Builder<T extends Table> extends FontDataTable.Builder<T> {
    private Header header;

    protected Builder(Header header, WritableFontData data) {
      super(data);
      this.header = header;
    }

    protected Builder(Header header, ReadableFontData data) {
      super(data);
      this.header = header;
    }

    protected Builder(Header header) {
      this(header, null);
    }

    @Override
    public String toString() {
      return "Table Builder for - " + this.header.toString();
    }

    /***********************************************************************************
     * 
     * Public Interface for Table Building
     * 
     ***********************************************************************************/

    public final Header header() {
      return this.header;
    }

    /***********************************************************************************
     * Internal Interface for Table Building
     ***********************************************************************************/

    @Override
    protected void notifyPostTableBuild(T table) {
      if (this.modelChanged() || this.dataChanged()) {
        Header header = new Header(this.header().tag(), table.dataLength());
        ((Table) table).header = header;
      }
    }

    // static interface for building

    /**
     * Get a builder for the table type specified by the data in the header.
     *
     * @param header the header for the table
     * @param tableData the data to be used to build the table from
     * @return builder for the table specified
     */
    public static Table.Builder<? extends Table> getBuilder(
        Header header, WritableFontData tableData) {

      int tag = header.tag();

      if (tag == Tag.cmap) {
        return CMapTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.head) {
        return FontHeaderTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.hhea) {
        return HorizontalHeaderTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.hmtx) {
        return HorizontalMetricsTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.maxp) {
        return MaximumProfileTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.name) {
        return NameTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.OS_2) {
        return OS2Table.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.post) {
        return PostScriptTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.cvt) {
        return ControlValueTable.Builder.createBuilder(header, tableData);
        // } else if (tag == Tag.fpgm) {
        // break;
      } else if (tag == Tag.glyf) {
        return GlyphTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.loca) {
        return LocaTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.prep) {
        return ControlProgramTable.Builder.createBuilder(header, tableData);
        // } else if (tag == CFF) {
        // break;
        // } else if (tag == VORG) {
        // break;
      } else if (tag == Tag.EBDT) {
        return EbdtTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.EBLC) {
        return EblcTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.EBSC) {
        return EbscTable.Builder.createBuilder(header, tableData);
        // } else if (tag == BASE) {
        // break;
        // } else if (tag == GDEF) {
        // break;
        // } else if (tag == GPOS) {
        // break;
      } else if (tag == Tag.GSUB) {
        return GSubTable.Builder.createBuilder(header, tableData);
        // break;
        // } else if (tag == JSTF) {
        // break;
        // } else if (tag == DSIG) {
        // break;
        // } else if (tag == gasp) {
        // break;
      } else if (tag == Tag.hdmx) {
        return HorizontalDeviceMetricsTable.Builder.createBuilder(header, tableData);
        // break;
        // } else if (tag == kern) {
        // break;
        // } else if (tag == LTSH) {
        // break;
        // } else if (tag == PCLT) {
        // break;
        // } else if (tag == VDMX) {
        // break;
        // } else if (tag == vhea) {
        // break;
        // } else if (tag == vmtx) {
        // break;
      } else if (tag == Tag.bhed) {
        return FontHeaderTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.bdat) {
        return EbdtTable.Builder.createBuilder(header, tableData);
      } else if (tag == Tag.bloc) {
        return EblcTable.Builder.createBuilder(header, tableData);
      }
      return GenericTableBuilder.createBuilder(header, tableData);
    }
  }
}
