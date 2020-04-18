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

import static com.google.android.libraries.feed.host.storage.ContentOperation.Type.DELETE;
import static com.google.android.libraries.feed.host.storage.ContentOperation.Type.DELETE_BY_PREFIX;
import static com.google.android.libraries.feed.host.storage.ContentOperation.Type.UPSERT;

import android.content.Context;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.host.storage.ContentMutation;
import com.google.android.libraries.feed.host.storage.ContentOperation;
import com.google.android.libraries.feed.host.storage.ContentOperation.Delete;
import com.google.android.libraries.feed.host.storage.ContentOperation.DeleteByPrefix;
import com.google.android.libraries.feed.host.storage.ContentOperation.Upsert;
import com.google.android.libraries.feed.host.storage.ContentStorage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;

/** Implementation of {@link ContentStorage} that persists data to disk. */
public class PersistentContentStorage implements ContentStorage {


  private static final String TAG = "PersistentContent";
  private static final String CONTENT_DIR = "content";

  private final Context context;
  private final ThreadUtils threadUtils;
  private final ExecutorService executor;

  private File contentDir;

  public PersistentContentStorage(
      Context context, ExecutorService executor, ThreadUtils threadUtils) {
    this.context = context;
    this.executor = executor;
    this.threadUtils = threadUtils;
  }

  @Override
  public void get(List<String> keys, Consumer<Result<Map<String, byte[]>>> consumer) {
    threadUtils.checkMainThread();

    executor.execute(
        () -> {
          initializeContentDir();

          Map<String, byte[]> valueMap = new HashMap<>(keys.size());
          for (String key : keys) {
            File keyFile = new File(contentDir, key);
            if (keyFile.exists()) {
              try {
                valueMap.put(key, getFileContents(keyFile));
              } catch (IOException e) {
                Logger.e(TAG, e, "Error getting file contents");
                consumer.accept(Result.failure());
              }
            }
          }
          consumer.accept(Result.success(valueMap));
        });
  }

  @Override
  public void getAll(String prefix, Consumer<Result<Map<String, byte[]>>> consumer) {
    threadUtils.checkMainThread();

    executor.execute(
        () -> {
          initializeContentDir();

          File[] files = contentDir.listFiles();

          Map<String, byte[]> allFilesMap = new HashMap<>();

          try {
            if (files != null) {
              for (File file : files) {
                if (file.getName().startsWith(prefix)) {
                  allFilesMap.put(file.getName(), getFileContents(file));
                }
              }
            }

            consumer.accept(Result.success(allFilesMap));
          } catch (IOException e) {
            Logger.e(TAG, e, "Error getting file contents");
            consumer.accept(Result.failure());
          }
        });
  }

  private byte[] getFileContents(File keyFile) throws IOException {
    threadUtils.checkNotMainThread();

    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

    if (keyFile.exists()) {
      try (FileInputStream fileInputStream = new FileInputStream(keyFile)) {
        while (fileInputStream.available() > 0) {
          outputStream.write(fileInputStream.read());
        }
        return outputStream.toByteArray();
      } catch (IOException e) {
        Logger.e(TAG, "Error reading file", e);
        throw new IOException("Error reading file", e);
      }
    }
    return new byte[0];
  }

  @Override
  public void commit(ContentMutation mutation, Consumer<CommitResult> consumer) {
    threadUtils.checkMainThread();
    executor.execute(
        () -> {
          initializeContentDir();

          for (ContentOperation operation : mutation.getOperations()) {
            if (operation.getType() == UPSERT) {
              if (!upsert((Upsert) operation)) {
                consumer.accept(CommitResult.FAILURE);
                return;
              }
            } else if (operation.getType() == DELETE) {
              if (!delete((Delete) operation)) {
                consumer.accept(CommitResult.FAILURE);
                return;
              }
            } else if (operation.getType() == DELETE_BY_PREFIX) {
              if (!deleteByPrefix((DeleteByPrefix) operation)) {
                consumer.accept(CommitResult.FAILURE);
                return;
              }
            } else {
              Logger.e(TAG, "Unrecognized ContentOperation type: %s", operation.getType());
            }
          }

          consumer.accept(CommitResult.SUCCESS);
        });
  }

  private boolean deleteByPrefix(DeleteByPrefix operation) {
    threadUtils.checkNotMainThread();

    File[] files = contentDir.listFiles();

    if (files != null) {
      for (File file : files) {
        if (file.getName().startsWith(operation.getPrefix())) {
          // Try to delete
          if (!file.delete()) {
            Logger.e(TAG, "Error deleting by prefix %s", operation.getPrefix());
            return false;
          }
        }
      }
      return true;
    }

    return false;
  }

  private boolean delete(Delete operation) {
    threadUtils.checkNotMainThread();

    File file = new File(contentDir, operation.getKey());
    boolean result = file.delete();
    if (!result) {
      Logger.e(TAG, "Error deleting key %s", operation.getKey());
    }
    return result;
  }

  private boolean upsert(Upsert operation) {
    threadUtils.checkNotMainThread();

    File file = new File(contentDir, operation.getKey());
    try (FileOutputStream fos = new FileOutputStream(file)) {
      fos.write(operation.getValue());
      return true;
    } catch (IOException e) {
      Logger.e(
          TAG, "Error upserting key %s with value %s", operation.getKey(), operation.getValue());
      return false;
    }
  }

  private void initializeContentDir() {
    threadUtils.checkNotMainThread();

    if (contentDir == null) {
      contentDir = context.getDir(CONTENT_DIR, Context.MODE_PRIVATE);
    }
  }
}

