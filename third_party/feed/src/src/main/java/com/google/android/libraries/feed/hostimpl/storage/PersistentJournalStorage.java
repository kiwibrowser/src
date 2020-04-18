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

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.host.storage.JournalMutation;
import com.google.android.libraries.feed.host.storage.JournalOperation;
import com.google.android.libraries.feed.host.storage.JournalOperation.Append;
import com.google.android.libraries.feed.host.storage.JournalOperation.Copy;
import com.google.android.libraries.feed.host.storage.JournalStorage;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;

/** Implementation of {@link JournalStorage} that persists data to disk. */
public class PersistentJournalStorage implements JournalStorage {

  private static final String TAG = "PersistentJournal";
  private static final String JOURNAL_DIR = "journal";
  public static final String ENCODING = "UTF-8";
  public static final Charset CHARSET = Charset.forName(ENCODING);
  public static final String ASTERISK = "_ATK_";

  private final Context context;
  private final ThreadUtils threadUtils;
  private final ExecutorService executor;
  private File journalDir;

  public PersistentJournalStorage(
      Context context, ExecutorService executorService, ThreadUtils threadUtils) {
    this.context = context;
    this.executor = executorService;
    this.threadUtils = threadUtils;
  }

  @Override
  public void read(String journalName, Consumer<Result<List<byte[]>>> consumer) {
    threadUtils.checkMainThread();

    executor.execute(
        () -> {
          initializeJournalDir();

          String sanitizedJournalName = sanitize(journalName);
          if (!sanitizedJournalName.isEmpty()) {
            File journal = new File(journalDir, sanitizedJournalName);
            try {
              consumer.accept(Result.success(getJournalContents(journal)));
            } catch (IOException e) {
              Logger.e(TAG, "Error occured reading journal %s", journalName);
              consumer.accept(Result.failure());
            }
          }
        });
  }

  private List<byte[]> getJournalContents(File journal) throws IOException {
    threadUtils.checkNotMainThread();

    List<byte[]> journalContents = new ArrayList<>();
    if (journal.exists()) {
      try (BufferedReader reader = new BufferedReader(new FileReader(journal))) {
        String line;
        while ((line = reader.readLine()) != null) {
          byte[] bytes = line.getBytes(CHARSET);
          journalContents.add(bytes);
        }
      } catch (IOException e) {
        Logger.e(TAG, "Error reading file", e);
        throw new IOException("Error reading journal file", e);
      }
    }
    return journalContents;
  }

  @Override
  public void commit(JournalMutation mutation, Consumer<CommitResult> consumer) {
    threadUtils.checkMainThread();

    executor.execute(
        () -> {
          initializeJournalDir();

          String sanitizedJournalName = sanitize(mutation.getJournalName());
          if (!sanitizedJournalName.isEmpty()) {
            File journal = new File(journalDir, sanitizedJournalName);

            for (JournalOperation operation : mutation.getOperations()) {
              if (operation.getType() == APPEND) {
                if (!append((Append) operation, journal)) {
                  consumer.accept(CommitResult.FAILURE);
                  return;
                }
              } else if (operation.getType() == COPY) {
                if (!copy((Copy) operation, journal)) {
                  consumer.accept(CommitResult.FAILURE);
                  return;
                }
              } else if (operation.getType() == DELETE) {
                if (!delete(journal)) {
                  consumer.accept(CommitResult.FAILURE);
                  return;
                }
              } else {
                Logger.e(TAG, "Unrecognized journal operation type %s", operation.getType());
              }
            }

            consumer.accept(CommitResult.SUCCESS);
          }
          // TODO: error handling
        });
  }

  private boolean delete(File journal) {
    threadUtils.checkNotMainThread();

    if (!journal.exists()) {
      // If the file doesn't exist, let's call it deleted.
      return true;
    }
    boolean result = journal.delete();
    if (!result) {
      Logger.e(TAG, "Error deleting journal %s", journal.getName());
    }
    return result;
  }

  private boolean copy(Copy operation, File journal) {
    threadUtils.checkNotMainThread();

    try {
      String sanitizedDestJournalName = sanitize(operation.getToJournalName());
      if (!sanitizedDestJournalName.isEmpty()) {
        Files.copy(journal.toPath(), new File(journalDir, sanitizedDestJournalName).toPath());
        return true;
      }
      // TODO: error handling
    } catch (IOException e) {
      Logger.e(
          TAG,
          e,
          "Error copying journal %s to %s",
          journal.getName(),
          operation.getToJournalName());
    }
    return false;
  }

  private boolean append(Append operation, File journal) {
    threadUtils.checkNotMainThread();

    try (BufferedWriter writer = new BufferedWriter(new FileWriter(journal, /* append= */ true))) {
      writer.write(new String(operation.getValue(), CHARSET));
      writer.newLine();
      return true;
    } catch (IOException e) {
      Logger.e(
          TAG, "Error appending byte[] %s for journal %s", operation.getValue(), journal.getName());
      return false;
    }
  }

  @Override
  public void exists(String journalName, Consumer<Result<Boolean>> consumer) {
    threadUtils.checkMainThread();

    executor.execute(
        () -> {
          initializeJournalDir();

          String sanitizedJournalName = sanitize(journalName);
          if (!sanitizedJournalName.isEmpty()) {
            File journal = new File(journalDir, sanitizedJournalName);
            consumer.accept(Result.success(journal.exists()));
          }
          // TODO: error handling
        });
  }

  @Override
  public void getAllJournals(Consumer<Result<List<String>>> consumer) {
    threadUtils.checkMainThread();

    executor.execute(
        () -> {
          initializeJournalDir();

          File[] files = journalDir.listFiles();
          List<String> journals = new ArrayList<>();
          if (files != null) {
            for (File file : files) {
              String desanitizedFileName = desanitize(file.getName());
              if (!desanitizedFileName.isEmpty()) {
                journals.add(desanitizedFileName);
              }
            }
          }
          consumer.accept(Result.success(journals));
        });
  }

  private void initializeJournalDir() {
    threadUtils.checkNotMainThread();

    if (journalDir == null) {
      journalDir = context.getDir(JOURNAL_DIR, Context.MODE_PRIVATE);
    }
  }

  @VisibleForTesting
  String sanitize(String journalName) {
    try {
      // * is not replaced by URL encoder
      String sanitized = URLEncoder.encode(journalName, ENCODING);
      return sanitized.replace("*", ASTERISK);
    } catch (UnsupportedEncodingException e) {
      // Should not happen
      Logger.e(TAG, "Error sanitizing journal name %s", journalName);
      return "";
    }
  }

  @VisibleForTesting
  String desanitize(String sanitizedJournalName) {
    try {
      // * is not replaced by URL encoder
      String desanitized = URLDecoder.decode(sanitizedJournalName, ENCODING);
      return desanitized.replace(ASTERISK, "*");
    } catch (UnsupportedEncodingException e) {
      // Should not happen
      Logger.e(TAG, "Error desanitizing journal name %s", sanitizedJournalName);
      return "";
    }
  }
}
