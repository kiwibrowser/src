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

package com.google.android.libraries.feed.host.storage;

import android.support.annotation.IntDef;

/** A mutation to the underlying {@link JournalStorage}. */
public abstract class JournalOperation {
  /** The types of operations. */
  @IntDef({Type.APPEND, Type.COPY, Type.DELETE})
  public @interface Type {
    int APPEND = 0;
    int COPY = 1;
    int DELETE = 2;
  }

  public @Type int getType() {
    return type;
  }

  private final @Type int type;

  // Only the following classes may extend JournalOperation
  private JournalOperation(@Type int type) {
    this.type = type;
  }

  /**
   * A {@link JournalOperation} created by calling {@link JournalMutation.Builder#append(byte[])}.
   */
  public static class Append extends JournalOperation {
    private final byte[] value;

    Append(byte[] value) {
      super(Type.APPEND);
      this.value = value;
    }

    public byte[] getValue() {
      return value;
    }
  }

  /** A {@link JournalOperation} created by calling {@link JournalMutation.Builder#copy(String)}. */
  public static class Copy extends JournalOperation {
    private final String toJournalName;

    Copy(String toJournalName) {
      super(Type.COPY);
      this.toJournalName = toJournalName;
    }

    public String getToJournalName() {
      return toJournalName;
    }
  }

  /** A {@link JournalOperation} created by calling {@link JournalMutation.Builder#delete()}. */
  public static class Delete extends JournalOperation {
    Delete() {
      super(Type.DELETE);
    }
  }
}
