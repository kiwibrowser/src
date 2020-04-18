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

import com.google.android.libraries.feed.host.storage.JournalOperation.Append;
import com.google.android.libraries.feed.host.storage.JournalOperation.Copy;
import com.google.android.libraries.feed.host.storage.JournalOperation.Delete;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * A mutation described by a journal name and a sequence of {@link JournalOperation} instances. This
 * class is used to commit changes to a journal in {@link JournalStorage}.
 */
public class JournalMutation {
  private final String journalName;
  private final List<JournalOperation> operations;

  private JournalMutation(String journalName, List<JournalOperation> operations) {
    this.journalName = journalName;
    this.operations = Collections.unmodifiableList(operations);
  }

  /** An unmodifiable list of operations to be committed by {@link JournalStorage}. */
  public List<JournalOperation> getOperations() {
    return operations;
  }

  /** The name of the journal this mutation should be applied to. */
  public String getJournalName() {
    return journalName;
  }

  /**
   * Creates a {@link JournalMutation}, which can be used to apply mutations to a journal in the
   * underlying {@link JournalStorage}.
   */
  public static class Builder {
    private final ArrayList<JournalOperation> operations = new ArrayList<>();
    private final String journalName;

    public Builder(String journalName) {
      this.journalName = journalName;
    }

    /**
     * Appends {@code value} to the journal in {@link JournalStorage}.
     *
     * <p>This method can be called repeatedly to append multiple times.
     */
    public Builder append(byte[] value) {
      operations.add(new Append(value));
      return this;
    }

    /** Copies the journal to {@code toJournalName}. */
    public Builder copy(String toJournalName) {
      operations.add(new Copy(toJournalName));
      return this;
    }

    /** Deletes the journal. */
    public Builder delete() {
      operations.add(new Delete());
      return this;
    }

    /** Creates a {@link JournalMutation} based on the operations added in this builder. */
    public JournalMutation build() {
      return new JournalMutation(journalName, new ArrayList<>(operations));
    }
  }
}
