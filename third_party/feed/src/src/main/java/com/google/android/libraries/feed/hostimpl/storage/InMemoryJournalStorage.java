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

package com.google.android.libraries.feed.hostimpl.storage;

import static com.google.android.libraries.feed.host.storage.JournalOperation.Type.APPEND;
import static com.google.android.libraries.feed.host.storage.JournalOperation.Type.COPY;
import static com.google.android.libraries.feed.host.storage.JournalOperation.Type.DELETE;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.host.storage.JournalMutation;
import com.google.android.libraries.feed.host.storage.JournalOperation;
import com.google.android.libraries.feed.host.storage.JournalOperation.Append;
import com.google.android.libraries.feed.host.storage.JournalOperation.Copy;
import com.google.android.libraries.feed.host.storage.JournalStorage;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/** A {@link JournalStorage} implementation that holds the data in memory. */
public class InMemoryJournalStorage implements JournalStorage, Dumpable {
  private static final String TAG = "InMemoryJournalStorage";

  private final Map<String, List<byte[]>> journals;
  private int readCount = 0;
  private int appendCount = 0;
  private int copyCount = 0;
  private int deleteCount = 0;
  private final ThreadUtils threadUtils;

  public InMemoryJournalStorage(ThreadUtils threadUtils) {
    this.threadUtils = threadUtils;
    journals = new HashMap<>();
  }

  @Override
  public void read(String journalName, Consumer<Result<List<byte[]>>> consumer) {
    threadUtils.checkMainThread();

    readCount++;
    List<byte[]> journal = journals.get(journalName);
    if (journal == null) {
      journal = new ArrayList<>();
    }
    consumer.accept(Result.success(journal));
  }

  @Override
  public void commit(JournalMutation mutation, Consumer<CommitResult> consumer) {
    threadUtils.checkMainThread();

    String journalName = mutation.getJournalName();

    for (JournalOperation operation : mutation.getOperations()) {
      if (operation.getType() == APPEND) {
        if (!append(((Append) operation).getValue(), journalName)) {
          consumer.accept(CommitResult.FAILURE);
          return;
        }
      } else if (operation.getType() == COPY) {
        if (!copy(journalName, ((Copy) operation).getToJournalName())) {
          consumer.accept(CommitResult.FAILURE);
          return;
        }
      } else if (operation.getType() == DELETE) {
        if (!delete(journalName)) {
          consumer.accept(CommitResult.FAILURE);
          return;
        }
      } else {
        Logger.w(TAG, "Unexpected JournalOperation type: %s", operation.getType());
        consumer.accept(CommitResult.FAILURE);
        return;
      }
    }
    consumer.accept(CommitResult.SUCCESS);
  }

  @Override
  public void exists(String journalName, Consumer<Result<Boolean>> consumer) {
    threadUtils.checkMainThread();
    consumer.accept(Result.success(journals.containsKey(journalName)));
  }

  @Override
  public void getAllJournals(Consumer<Result<List<String>>> consumer) {
    threadUtils.checkMainThread();
    consumer.accept(Result.success(new ArrayList<>(journals.keySet())));
  }

  private boolean append(byte[] value, String journalName) {
    List<byte[]> journal = journals.get(journalName);
    if (value == null) {
      Logger.e(TAG, "Journal not found: %s", journalName);
      return false;
    }
    if (journal == null) {
      journal = new ArrayList<>();
      journals.put(journalName, journal);
    }
    appendCount++;
    journal.add(value);
    return true;
  }

  private boolean copy(String fromJournalName, String toJournalName) {
    copyCount++;
    List<byte[]> toJournal = journals.get(toJournalName);
    if (toJournal != null) {
      Logger.e(TAG, "Copy destination journal already present: %s", toJournalName);
      return false;
    }
    List<byte[]> journal = journals.get(fromJournalName);
    if (journal != null) {
      // TODO: Compact before copying?
      journals.put(toJournalName, new ArrayList<>(journal));
    }
    return true;
  }

  private boolean delete(String journalName) {
    deleteCount++;
    journals.remove(journalName);
    return true;
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("readCount").value(readCount);
    dumper.forKey("appendCount").value(appendCount).compactPrevious();
    dumper.forKey("copyCount").value(copyCount).compactPrevious();
    dumper.forKey("deleteCount").value(deleteCount).compactPrevious();
    dumper.forKey("sessions").value(journals.size());
    for (Entry<String, List<byte[]>> entry : journals.entrySet()) {
      Dumper childDumper = dumper.getChildDumper();
      childDumper.title("Session");
      childDumper.forKey("name").value(entry.getKey());
      childDumper.forKey("operations").value(entry.getValue().size()).compactPrevious();
    }
  }
}
