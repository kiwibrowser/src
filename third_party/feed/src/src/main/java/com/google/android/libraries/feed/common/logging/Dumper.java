// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common.logging;

import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * Simple Dumper modelled after the AGSA Dumper. This will dump a tree of objects which implement
 * the {@link Dumpable} interface.
 */
public class Dumper {
  private static final String TAG = "Dumper";
  private static final String SINGLE_INDENT = "  ";

  private final int indentLevel;

  // Walk up the chain of parents to
  // ensure none are currently dumping a given dumpable, to prevent cycles. The WeakReference
  // itself is always non-null, but the Dumpable may be null.
  private final WeakReference<Dumpable> currentDumpable;
  /*@Nullable*/ private final Dumper parent;

  // The root Dumper will create the values used by all children Dumper instances.  This flattens
  // the output into one list.
  @VisibleForTesting final List<DumperValue> values;

  /** Returns the default Dumper */
  public static Dumper newDefaultDumper() {
    // TODO: We need to support a factory which creates either Redacted Dumpers or
    // full Dumpers.
    return new Dumper(1, null, new WeakReference<Dumpable>(null), new ArrayList<>());
  }

  // Private constructor, these are created through the static newDefaultDumper} method.
  private Dumper(
      int indentLevel,
      /*@Nullable*/ Dumper parent,
      WeakReference<Dumpable> currentDumpable,
      List<DumperValue> values) {
    this.indentLevel = indentLevel;
    this.parent = parent;
    this.currentDumpable = currentDumpable;
    this.values = values;
  }

  private boolean isDescendentOf(Dumpable dumpable) {
    return (currentDumpable.get() == dumpable)
        || (parent != null && parent.isDescendentOf(dumpable));
  }

  /** Creates a new Dumper with a indent level one greater than the current indent level. */
  public Dumper getChildDumper() {
    return getChildDumper(null);
  }

  private Dumper getChildDumper(/*@Nullable*/ Dumpable dumpable) {
    return new Dumper(indentLevel + 1, this, new WeakReference<>(dumpable), values);
  }

  /** Set the title of the section. This is output at the previous indent level. */
  public void title(String title) {
    values.add(new DumperValue(indentLevel - 1, title));
  }

  /** Adds a String as an indented line to the dump */
  public void format(String format, Object... args) {
    addLine("", String.format(Locale.US, format, args));
  }

  /** Create a Dumper value with the specified name. */
  public DumperValue forKey(String name) {
    return addLine(indentLevel, name);
  }

  /** Allow the indent level to be adjusted. */
  public DumperValue forKey(String name, int indentAdjustment) {
    return addLine(indentLevel + indentAdjustment, name);
  }

  /** Dump a Dumpable as a child of the current Dumper */
  public void dump(/*@Nullable*/ Dumpable dumpable) {
    if (dumpable == null) {
      return;
    }
    if (isDescendentOf(dumpable)) {
      format("[cycle detected]");
      return;
    }
    Dumper child = getChildDumper(dumpable);
    try {
      dumpable.dump(child);
    } catch (Exception e) {
      Logger.e(TAG, e, "Dump Failed");
    }
  }

  /** Write the Dumpable to an {@link Appendable}. */
  public void write(Appendable writer) throws IOException {
    boolean newLine = true;
    for (DumperValue value : values) {
      String stringValue = value.toString(false);
      if (!newLine) {
        if (value.compactPrevious) {
          writer.append(" | ");
        } else {
          String indent = getIndent(value.indentLevel);
          writer.append("\n").append(indent);
        }
      }
      writer.append(stringValue);
      newLine = false;
    }
    writer.append("\n");
  }

  private DumperValue addLine(int indentLevel, String title) {
    DumperValue dumperValue = new DumperValue(indentLevel, title);
    values.add(dumperValue);
    return dumperValue;
  }

  private DumperValue addLine(String name, String value) {
    return forKey(name).value(value);
  }

  /**
   * Class which represents a name value pair within the dump. It is used for both titles and for
   * the name value pair content within the dump. The indent level is used to specify the indent
   * level for content appearing on a new line. Multiple DumperValues may be compacted into a single
   * line in the output.
   */
  public static class DumperValue {
    private static final String REDACTED = "[REDACTED]";
    @VisibleForTesting final String name;
    @VisibleForTesting final StringBuilder content;
    @VisibleForTesting final int indentLevel;
    private boolean compactPrevious = false;
    // TODO: What should the default be?
    private boolean sensitive = false;

    // create a DumpValue with just a name, this is not public, it will is called by Dumper
    DumperValue(int indentLevel, String name) {
      this.indentLevel = indentLevel;
      this.name = name;
      this.content = new StringBuilder();
    }

    /** Append an int to the current content of this value */
    public DumperValue value(int value) {
      this.content.append(Integer.toString(value));
      return this;
    }

    /** Append an String to the current content of this value */
    public DumperValue value(String value) {
      this.content.append(value);
      return this;
    }

    /** Append an boolean to the current content of this value */
    public DumperValue value(boolean value) {
      this.content.append(Boolean.toString(value));
      return this;
    }

    /** Add a Date value. It will be formatted as Logcat Dates are formatted */
    public DumperValue value(Date value) {
      this.content.append(StringFormattingUtils.formatLogDate(value));
      return this;
    }

    /** Add a value specified as an object. */
    public DumperValue valueObject(Object value) {
      if (value instanceof Integer) {
        return value((Integer) value);
      }
      if (value instanceof Boolean) {
        return value((Boolean) value);
      }
      return value(value.toString());
    }

    /**
     * When output, this DumperValue will be compacted to the same output line as the previous
     * value.
     */
    public DumperValue compactPrevious() {
      compactPrevious = true;
      return this;
    }

    /** Mark the Value as containing sensitive data */
    public DumperValue sensitive() {
      sensitive = true;
      return this;
    }

    /**
     * Output the value as a Name/Value pair. This will convert sensitive data to "REDACTED" if we
     * are not dumping sensitive data.
     */
    String toString(boolean suppressSensitive) {
      String value = "";
      if (!TextUtils.isEmpty(content)) {
        value = (suppressSensitive && sensitive) ? REDACTED : content.toString();
      }
      if (!TextUtils.isEmpty(name) && !TextUtils.isEmpty(value)) {
        return name + ": " + value;
      } else if (!TextUtils.isEmpty(name)) {
        return name + ":";
      } else if (!TextUtils.isEmpty(value)) {
        return String.valueOf(value);
      } else {
        return "";
      }
    }
  }

  private static final String[] INDENTATIONS =
      new String[] {
        "", createIndent(1), createIndent(2), createIndent(3), createIndent(4), createIndent(5),
      };

  private static String createIndent(int size) {
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < size; i++) {
      sb.append(SINGLE_INDENT);
    }
    return sb.toString();
  }

  private static String getIndent(int indentLevel) {
    if (indentLevel < 0) {
      return INDENTATIONS[0];
    } else if (indentLevel < INDENTATIONS.length) {
      return INDENTATIONS[indentLevel];
    } else {
      return createIndent(indentLevel);
    }
  }
}
