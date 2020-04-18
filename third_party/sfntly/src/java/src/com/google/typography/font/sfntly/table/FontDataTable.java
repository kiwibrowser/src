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

package com.google.typography.font.sfntly.table;

import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

import java.io.IOException;
import java.io.OutputStream;

/**
 * An abstract base for any table that contains a FontData. This is the root of
 * the table class hierarchy.
 *
 * @author Stuart Gill
 *
 */
public abstract class FontDataTable {
  protected ReadableFontData data;
  /**
   * Constructor.
   *
   * @param data the data to back this table
   */
  FontDataTable(ReadableFontData data) {
    this.data = data;
  }

  /**
   * Gets the readable font data for this table.
   *
   * @return the read only data
   */
  public ReadableFontData readFontData() {
    return this.data;
  }
  
  @Override
  public String toString() {
    return this.data.toString();
  }

  /**
   * Gets the length of the data for this table in bytes. This is the full
   * allocated length of the data underlying the table and may or may not
   * include any padding.
   *
   * @return the data length in bytes
   */
  public final int dataLength() {
    return this.data.length();
  }

  public int serialize(OutputStream os) throws IOException {
    return this.data.copyTo(os);
  }

  protected int serialize(WritableFontData data) {
    return this.data.copyTo(data);
  }

  public static abstract class Builder<T extends FontDataTable> {
    private WritableFontData wData;
    private ReadableFontData rData;
    private boolean modelChanged;
    private boolean containedModelChanged; // may expand to list of submodel states
    private boolean dataChanged;

    /**
     * Constructor.
     * 
     * Construct a FontDataTable.Builder with a WritableFontData backing store
     * of size given. A positive size will create a fixed size backing store and
     * a 0 or less size is an estimate for a growable backing store with the
     * estimate being the absolute of the size.
     * 
     * @param dataSize if positive then a fixed size; if 0 or less then an
     *        estimate for a growable size
     */
    protected Builder(int dataSize) {
      this.wData = WritableFontData.createWritableFontData(dataSize);
    }

    protected Builder(WritableFontData data) {
      this.wData = data;
    }

    protected Builder(ReadableFontData data) {
      this.rData = data;
    }

    /**
     * Gets a snapshot copy of the internal data of the builder.
     *
     *  This causes any internal data structures to be serialized to a new data
     * object. This data object belongs to the caller and must be properly
     * disposed of. No changes are made to the builder and any changes to the
     * data directly do not affect the internal state. To do that a subsequent
     * call must be made to {@link #setData(WritableFontData)}.
     *
     * @return a copy of the internal data of the builder
     * @see FontDataTable.Builder#setData(WritableFontData)
     */ 
    public WritableFontData data() {
      WritableFontData newData;
      if (this.modelChanged) {
        if (!this.subReadyToSerialize()) {
          throw new RuntimeException("Table not ready to build.");
        }
        int size = subDataSizeToSerialize();
        newData = WritableFontData.createWritableFontData(size);
        this.subSerialize(newData);
      } else {
        ReadableFontData data = internalReadData();
        newData = WritableFontData.createWritableFontData(data != null ? data.length() : 0);
        if (data != null) {
          data.copyTo(newData);
        }
      }
      return newData;
    }

    public void setData(WritableFontData data) {
      this.internalSetData(data, true);
    }

    /**
     * @param data
     */
    public void setData(ReadableFontData data) {
      this.internalSetData(data, true);
    }

    private void internalSetData(WritableFontData data, boolean dataChanged) {
      this.wData = data;
      this.rData = null;
      if (dataChanged) {
        this.dataChanged = true;
        this.subDataSet();
      }
    }

    private void internalSetData(ReadableFontData data, boolean dataChanged) {
      this.rData = data;
      this.wData = null;
      if (dataChanged) {
        this.dataChanged = true;
        this.subDataSet();
      }
    }

    public T build() {
      T table = null;

      ReadableFontData data = this.internalReadData();
      if (this.modelChanged) {
        // let subclass serialize from model
        if (!this.subReadyToSerialize()) {
          return null;
        }
        int size = subDataSizeToSerialize();
        WritableFontData newData = WritableFontData.createWritableFontData(size);
        this.subSerialize(newData);
        data = newData;
      }
      
      if (data != null) {
        table = this.subBuildTable(data);
        this.notifyPostTableBuild(table);
      }
      this.rData = null;
      this.wData = null;
      
      return table;
    }

    public boolean readyToBuild() {
      return true;
    }    

    protected ReadableFontData internalReadData() {
      if (this.rData != null) {
        return this.rData;
      }
      return this.wData;
    }

    protected WritableFontData internalWriteData() {
      if (this.wData == null) {
        WritableFontData newData =
            WritableFontData.createWritableFontData(this.rData == null ? 0 : this.rData.length());
        if (this.rData != null) {
          this.rData.copyTo(newData);
        }
        this.internalSetData(newData, false);
      }
      return this.wData;
    }
    
    /**
     * Determines whether the state of this builder has changed - either the data or the internal 
     * model representing the data.
     * 
     * @return true if the builder has changed
     */
    public boolean changed() {
      return this.dataChanged() || this.modelChanged();
    }

    protected boolean dataChanged() {
      return this.dataChanged;
    }

    protected boolean modelChanged() {
      return this.currentModelChanged() || this.containedModelChanged();
    }

    protected boolean currentModelChanged() {
      return this.modelChanged;
    }

    protected boolean containedModelChanged() {
      return this.containedModelChanged;
    }
    
    protected boolean setModelChanged() {
      return this.setModelChanged(true);
    }

    protected boolean setModelChanged(boolean changed) {
      boolean old = this.modelChanged;
      this.modelChanged = changed;
      return old;
    }

    // subclass API

    /**
     * Notification to subclasses that a table was built.
     */
    protected void notifyPostTableBuild(T table) {
      // NOP -
    }

    /**
     * Serialize the table to the data provided.
     *
     * @param newData the data object to serialize to
     * @return the number of bytes written
     */
    protected abstract int subSerialize(WritableFontData newData);

    /**
     * @return true if the subclass is ready to serialize it's structure into
     *         data
     */
    protected abstract boolean subReadyToSerialize();

    /**
     * Query if the subclass needs to serialize and how much data is required.
     *
     * @return positive bytes needed to serialize if a fixed size; and zero or
     *         negative bytes as an estimate if growable data is needed
     */
    protected abstract int subDataSizeToSerialize();

    /**
     * Tell the subclass that the data has been changed and any structures must
     * be discarded.
     */
    protected abstract void subDataSet();

    /**
     * Build a table with the data provided.
     *
     * @param data the data to use to build the table
     * @return a table
     */
    protected abstract T subBuildTable(ReadableFontData data);
  }
}
